#!/usr/bin/env python3
"""Rebuild icon sheet from pokemondb.net (XY sprites Gen1-8, HOME sprites Gen9)."""
import json, os, time, re
from concurrent.futures import ThreadPoolExecutor, as_completed
from urllib.request import urlopen, Request
from PIL import Image
from io import BytesIO

DATA = os.path.join(os.path.dirname(__file__), '..', '..', 'data')
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'frontend', 'public', 'sprites')
UTILS = os.path.join(os.path.dirname(__file__), '..', '..', 'frontend', 'src', 'utils')
ICON_CELL, COLS = 96, 20

with open(os.path.join(DATA, 'showdown_data.json'), 'r', encoding='utf-8') as f:
    dump = json.load(f)

def pokemondb_name(name):
    """Convert Showdown name → pokemondb URL name."""
    n = name.lower().strip()
    n = n.replace('♀', '-f').replace('♂', '-m')
    n = n.replace('é', 'e').replace('è', 'e').replace('ê', 'e')
    # Replace non-alphanumeric chars with hyphen, collapse multiples
    n = re.sub(r'[^a-z0-9]+', '-', n)
    return n.strip('-')

def fetch(url):
    for _ in range(2):
        try:
            req = Request(url, headers={'User-Agent':'Mozilla/5.0 (compatible; PokemonSim/1.0)'})
            img = Image.open(BytesIO(urlopen(req, timeout=15).read())).convert("RGBA")
            # Resize to fit cell
            w, h = img.size
            if w != ICON_CELL or h != ICON_CELL:
                scale = ICON_CELL / max(w, h) * 0.85
                new_w, new_h = int(w * scale), int(h * scale)
                img = img.resize((new_w, new_h), Image.LANCZOS)
                # Center in ICON_CELL x ICON_CELL canvas
                canvas = Image.new('RGBA', (ICON_CELL, ICON_CELL), (0,0,0,0))
                ox, oy = (ICON_CELL - new_w) // 2, (ICON_CELL - new_h) // 2
                canvas.paste(img, (ox, oy), img)
                return canvas
            return img
        except:
            time.sleep(0.3)
    return None

# ── Collect species: unique by NatDex ID (base form only for sheet) ──
species_list = sorted(
    [s for s in dump['species'] if 0 < s['id'] <= 1025],
    key=lambda s: s['id'])
# Deduplicate by ID — keep first (base form)
seen_ids = set()
unique_species = []
for s in species_list:
    if s['id'] not in seen_ids:
        seen_ids.add(s['id'])
        unique_species.append(s)

print(f"Icon sheet: {len(unique_species)} unique NatDex species")

# Determine Gen for each species (Gen1-8 = 1-905, Gen9 = 906-1025)
def sprite_url(s):
    n = pokemondb_name(s['name'])
    return f"https://img.pokemondb.net/sprites/home/normal/{n}.png"

print("Downloading (16 threads)...")
icons = [None] * len(unique_species)
ok = 0
with ThreadPoolExecutor(max_workers=16) as pool:
    futs = {pool.submit(fetch, sprite_url(s)): i for i, s in enumerate(unique_species)}
    for f in as_completed(futs):
        i = futs[f]; img = f.result()
        icons[i] = img if img else Image.new('RGBA', (ICON_CELL, ICON_CELL), (0,0,0,0))
        if img: ok += 1
        if ok % 200 == 0: print(f"  icons: {ok}/{len(unique_species)}")

rows = (len(icons) + COLS - 1) // COLS
sheet = Image.new('RGBA', (COLS * ICON_CELL, rows * ICON_CELL), (0,0,0,0))
for i, icon in enumerate(icons):
    sheet.paste(icon, ((i % COLS) * ICON_CELL, (i // COLS) * ICON_CELL))
sheet.save(os.path.join(OUT, 'icons-sheet.png'), optimize=True)
with open(os.path.join(UTILS, 'iconSheet.js'), 'w') as f:
    f.write(f"// pokemondb.net sprites — {len(icons)} species\nexport const ICON_SHEET = {{ url:'/sprites/icons-sheet.png', cellW:{ICON_CELL}, cellH:{ICON_CELL}, cols:{COLS} }};\n")

print(f"Icon sheet: {ok}/{len(icons)} done → icons-sheet.png")
print("Done!")

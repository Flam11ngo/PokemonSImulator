#!/usr/bin/env python3
"""Rebuild item sprite sheet from 52poke wiki (30x30 bag sprites, all gens incl. Gen9).
Falls back to PokeSprite (32x32 pixel art) for items missing from 52poke."""
import json, os, re, hashlib, time, urllib.parse
from concurrent.futures import ThreadPoolExecutor, as_completed
from urllib.request import urlopen, Request
from PIL import Image
from io import BytesIO

DATA = os.path.join(os.path.dirname(__file__), '..', '..', 'data')
PS_ROOT = os.path.join(os.path.dirname(__file__), '..', '..', 'node_modules', 'pokesprite-images', 'items')
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'frontend', 'public', 'sprites')
UTILS = os.path.join(os.path.dirname(__file__), '..', '..', 'frontend', 'src', 'utils')

CELL, COLS = 24, 20
WIKI_BASE = "https://s1.52poke.com/wiki"

# ═══════════════════════════════════════════════════════
# Step 1: Parse 52poke wiki for Chinese→English mapping
# ═══════════════════════════════════════════════════════
print("=== Step 1: Parse 52poke wiki for CN→EN mapping ===")

# Try reading cached wiki text, otherwise fetch
wiki_path = os.path.join(os.path.dirname(__file__), '..', '..', 'tmp_wiki_items.txt')
if os.path.exists(wiki_path):
    with open(wiki_path, 'r', encoding='utf-8') as f:
        wiki_text = f.read()
    print(f"Read cached wiki ({len(wiki_text)} chars)")
else:
    raise FileNotFoundError("Need wiki text cache at tmp_wiki_items.txt")

item_rows = []
for match in re.finditer(
    r'\|\s*\{\{Bag/Latest\|([^}|]+)(?:\|[^}]*)?\}\}\s*\n'
    r'\|\s*\{\{I\|([^}|]+)(?:\|[^}]*)?\}\}\s*\n'
    r'\|\s*([^\n]+)\s*\n'
    r'\|\s*([^\n]+?)\s*(?:\n|$)',
    wiki_text
):
    chinese = match.group(1).strip()
    english = match.group(4).strip()
    english = re.sub(r"'''?", '', english)
    english = re.sub(r'\[\[.*?\|', '', english)
    english = re.sub(r'[\[\]]', '', english)
    english = english.strip()
    if english and chinese:
        item_rows.append({'chinese': chinese, 'english': english.lower()})

en_to_cn = {}
for row in item_rows:
    en = row['english']
    cn = row['chinese']
    if en not in en_to_cn:
        en_to_cn[en] = cn

print(f"Parsed {len(item_rows)} items, {len(en_to_cn)} unique EN→CN pairs")

# ═══════════════════════════════════════════════════════
# Step 2: Build PokeSprite fallback index
# ═══════════════════════════════════════════════════════
print("\n=== Step 2: Build PokeSprite fallback index ===")

ps_files = set()
if os.path.exists(PS_ROOT):
    for root, dirs, files in os.walk(PS_ROOT):
        for fn in files:
            if fn.endswith('.png'):
                rel = os.path.relpath(os.path.join(root, fn), PS_ROOT).replace('\\', '/')
                ps_files.add(rel)

def hyphenate(s):
    n = s.lower().strip()
    n = re.sub(r"['.():]", '', n)
    n = re.sub(r'[^a-z0-9]+', '-', n).strip('-')
    return n

def find_ps_path(sd_name):
    n = sd_name.lower().strip()
    hn = hyphenate(sd_name)
    rules = [
        lambda: f"z-crystals/{hyphenate(n.replace(' z', ''))}-z--held.png",
        lambda: f"berry/{hyphenate(re.sub(r'\s*berry$', '', n))}.png",
        lambda: f"fossil/{hyphenate(re.sub(r'\s*fossil$', '', n))}.png",
        lambda: f"plate/{hyphenate(re.sub(r'\s*plate$', '', n))}.png",
        lambda: f"gem/{hyphenate(re.sub(r'\s*gem$', '', n))}.png",
        lambda: f"memory/{hyphenate(re.sub(r'\s*memory$', '', n))}.png",
        lambda: f"incense/{hyphenate(re.sub(r'\s*incense$', '', n))}.png",
        lambda: f"evo-item/{hn}.png",
        lambda: f"mega-stone/{hn}.png",
        lambda: f"battle-item/{hn}.png",
        lambda: f"hold-item/{hn}.png",
    ]
    for rule in rules:
        path = rule()
        if path in ps_files:
            return path
    for f in ps_files:
        basename = os.path.splitext(os.path.basename(f))[0]
        if basename == hn or basename == f"{hn}--held":
            return f
    return None

# ═══════════════════════════════════════════════════════
# Step 3: Match Showdown items & filter battle items
# ═══════════════════════════════════════════════════════
print("\n=== Step 3: Match items ===")

with open(os.path.join(DATA, 'showdown_data.json'), 'r', encoding='utf-8') as f:
    dump = json.load(f)

def is_non_battle(name):
    if re.match(r'^tr\d+$', name) or re.match(r'^tm\d+$', name):
        return True
    if re.search(r'\bball$', name) and name not in ('snowball', 'smoke ball'):
        return True
    if re.search(r'\bmail\b', name):
        return True
    non_battle = [
        r'\bpotion\b', r'\bether\b', r'\belixir\b', r'\brevive\b', r'\brepel\b',
        r'rare.candy', r'rare.bone', r'\bnugget\b', r'\bpearl\b',
        r'stardust', r'star.piece', r'comet.shard',
        r'big.mushroom', r'balm.mushroom', r'tiny.mushroom',
        r'shoal.salt', r'shoal.shell', r'heart.scale', r'\bhoney\b',
        r'pp.up', r'pp.max', r'\bmulch\b', r'odd.keystone',
    ]
    for pat in non_battle:
        if re.search(pat, name):
            return True
    return False

def match_cn(sd_name):
    en_lower = sd_name.lower()
    if en_lower in en_to_cn:
        return en_to_cn[en_lower]
    en_clean = re.sub(r"[’'.\- ]", '', en_lower)
    for en, cn in en_to_cn.items():
        if re.sub(r"[’'.\- ]", '', en) == en_clean:
            return cn
    for en, cn in en_to_cn.items():
        if en_lower in en or en in en_lower:
            return cn
    return None

# Build item entries: (sd_name, source, source_path)
# source = 'wiki' or 'ps' or None
item_entries = []

for it in dump['items']:
    sd_name = it['name']
    name = sd_name.lower()
    if is_non_battle(name):
        continue

    # Try 52poke wiki first
    cn = match_cn(sd_name)
    if cn:
        filename = f'Bag_{cn}_Sprite.png'
        md5 = hashlib.md5(filename.encode('utf-8')).hexdigest()
        wiki_path = f'{md5[0]}/{md5[0:2]}/{urllib.parse.quote(filename)}'
        item_entries.append((sd_name, 'wiki', wiki_path))
    else:
        # Try PokeSprite fallback
        ps_path = find_ps_path(name)
        if ps_path:
            item_entries.append((sd_name, 'ps', ps_path))
        else:
            item_entries.append((sd_name, None, None))

wiki_count = sum(1 for _, s, _ in item_entries if s == 'wiki')
ps_count = sum(1 for _, s, _ in item_entries if s == 'ps')
none_count = sum(1 for _, s, _ in item_entries if s is None)
print(f"Items: {len(item_entries)} total ({wiki_count} wiki, {ps_count} PS, {none_count} missing)")

# ═══════════════════════════════════════════════════════
# Step 4: Download from 52poke wiki
# ═══════════════════════════════════════════════════════
print(f"\n=== Step 4: Download {wiki_count} wiki sprites ===")

# Progress tracking
download_results = {}  # sd_name -> (success, img_or_None)

def download_wiki(sd_name, wiki_path):
    url = f"{WIKI_BASE}/{wiki_path}"
    for attempt in range(3):
        try:
            req = Request(url, headers={'User-Agent': 'PokemonSim/1.0'})
            data = urlopen(req, timeout=10).read()
            img = Image.open(BytesIO(data)).convert("RGBA")
            img = img.resize((CELL, CELL), Image.NEAREST)
            return (sd_name, True, img)
        except Exception:
            if attempt < 2:
                time.sleep(0.3)
    return (sd_name, False, None)

with ThreadPoolExecutor(max_workers=12) as pool:
    futs = {}
    for sd_name, src, path in item_entries:
        if src == 'wiki':
            futs[pool.submit(download_wiki, sd_name, path)] = sd_name

    for f in as_completed(futs):
        sd_name, success, img = f.result()
        download_results[sd_name] = (success, img)

wiki_ok = sum(1 for _, (s, _) in download_results.items() if s)
wiki_fail = sum(1 for _, (s, _) in download_results.items() if not s)
print(f"Wiki downloads: {wiki_ok} OK, {wiki_fail} failed → falling back to PokeSprite")

# ═══════════════════════════════════════════════════════
# Step 5: Build final sprite sheet
# ═══════════════════════════════════════════════════════
print("\n=== Step 5: Build sprite sheet ===")

icons = []
imap = {}
ps_fallback_count = 0
missing_count = 0

for idx, (sd_name, src, path) in enumerate(item_entries):
    img = None
    used_src = src

    if src == 'wiki':
        success, img = download_results.get(sd_name, (False, None))
        if not success:
            # Fallback to PokeSprite
            ps_path = find_ps_path(sd_name.lower())
            if ps_path:
                try:
                    full_path = os.path.join(PS_ROOT, ps_path)
                    img = Image.open(full_path).convert("RGBA")
                    img = img.resize((CELL, CELL), Image.NEAREST)
                    used_src = 'ps-fallback'
                    ps_fallback_count += 1
                except Exception:
                    img = None
            if img is None:
                used_src = None

    elif src == 'ps':
        try:
            full_path = os.path.join(PS_ROOT, path)
            img = Image.open(full_path).convert("RGBA")
            img = img.resize((CELL, CELL), Image.NEAREST)
        except Exception:
            img = None
            used_src = None

    if img is None:
        img = Image.new('RGBA', (CELL, CELL), (0, 0, 0, 0))
        missing_count += 1

    icons.append(img)
    imap[sd_name.lower()] = f"-{(idx % COLS) * CELL}px -{(idx // COLS) * CELL}px"

rows = (len(icons) + COLS - 1) // COLS
sheet = Image.new('RGBA', (COLS * CELL, rows * CELL), (0, 0, 0, 0))
for i, icon in enumerate(icons):
    sheet.paste(icon, ((i % COLS) * CELL, (i // COLS) * CELL))

out_path = os.path.join(OUT, 'items-sheet.png')
sheet.save(out_path, optimize=True)
print(f"Sheet: {out_path} ({COLS}x{rows}, {len(icons)} items)")
print(f"Sources: wiki={wiki_ok}, PS-fallback={ps_fallback_count}, PS-direct={ps_count}, missing={missing_count}")

# Generate JS
with open(os.path.join(UTILS, 'itemSheet.js'), 'w', encoding='utf-8') as f:
    f.write(f"// 52poke wiki + PokeSprite fallback — {len(icons)-missing_count}/{len(icons)} items\n")
    f.write(f"export const ITEM_SHEET = {{\"url\":\"/sprites/items-sheet.png\",\"size\":{CELL},\"cols\":{COLS},\"mapping\":{json.dumps(imap, ensure_ascii=False)} }};\n")

print(f"itemSheet.js: {len(imap)} entries")
print("Done!")

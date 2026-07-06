#!/usr/bin/env python3
"""Download back sprites from Showdown CDN to local public/sprites/back/ folder.
Downloads both GIF (animated) and PNG (static fallback) for all base species."""
import json, os, re, time
from concurrent.futures import ThreadPoolExecutor, as_completed
from urllib.request import urlopen, Request
from pathlib import Path

DATA = Path(__file__).resolve().parent.parent.parent / "data"
OUT = Path(__file__).resolve().parent.parent.parent / "frontend" / "public" / "sprites" / "back"
OUT.mkdir(parents=True, exist_ok=True)

CDN = "https://play.pokemonshowdown.com/sprites"

def showdown_id(name):
    s = name.lower().strip()
    s = s.replace("'", "").replace(".", "").replace("-", "").replace(" ", "")
    s = s.replace("é", "e").replace("è", "e").replace("ê", "e")
    s = re.sub(r"[^a-z0-9]", "", s)
    return s

# Load base-form species only
with open(DATA / "showdown_data.json", "r", encoding="utf-8") as f:
    dump = json.load(f)

base_species = []
seen = set()
for s in dump["species"]:
    sid = s["id"]
    if sid <= 0 or sid in seen:
        continue
    seen.add(sid)
    base_species.append((sid, s["name"]))

print(f"Species to download: {len(base_species)}")
print(f"Output: {OUT}")

def download_one(sid, name):
    sid_str = showdown_id(name)
    results = []

    # GIF first (animated)
    gif_url = f"{CDN}/ani-back/{sid_str}.gif"
    gif_path = OUT / f"{sid}.gif"
    if not gif_path.exists():
        try:
            req = Request(gif_url, headers={"User-Agent": "PokemonSim/1.0"})
            data = urlopen(req, timeout=10).read()
            if len(data) > 100:  # valid image
                with open(gif_path, "wb") as f:
                    f.write(data)
                results.append("gif")
        except Exception:
            pass

    # PNG fallback
    png_url = f"{CDN}/ani-back/{sid_str}.png"
    png_path = OUT / f"{sid}.png"
    if not png_path.exists():
        try:
            req = Request(png_url, headers={"User-Agent": "PokemonSim/1.0"})
            data = urlopen(req, timeout=10).read()
            if len(data) > 100:
                with open(png_path, "wb") as f:
                    f.write(data)
                results.append("png")
        except Exception:
            pass

    return sid, name, results

gif_ok = 0
png_ok = 0
failed = []

with ThreadPoolExecutor(max_workers=16) as pool:
    futs = {pool.submit(download_one, sid, name): sid for sid, name in base_species}
    for f in as_completed(futs):
        sid, name, results = f.result()
        if "gif" in results:
            gif_ok += 1
        if "png" in results:
            png_ok += 1
        if not results:
            failed.append((sid, name))
        if (gif_ok + png_ok) % 200 == 0:
            print(f"  progress: {gif_ok} gif + {png_ok} png / {len(base_species)}")

print(f"\nDone!")
print(f"  GIF: {gif_ok}/{len(base_species)}")
print(f"  PNG: {png_ok}/{len(base_species)}")
print(f"  Failed: {len(failed)}")

if failed:
    print(f"\nFailed species (first 20):")
    for sid, name in failed[:20]:
        print(f"  #{sid} {name}  →  {showdown_id(name)}")

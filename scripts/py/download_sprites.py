#!/usr/bin/env python3
"""Download Pokemon animated sprites from Pokemon Showdown CDN to local directory."""
import json, urllib.request, sys, os, time
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_DIR / "data"
SPRITE_DIR = PROJECT_DIR / "frontend" / "public" / "sprites"
SPRITE_DIR.mkdir(parents=True, exist_ok=True)

def load_species():
    with open(DATA_DIR / "species.json") as f:
        data = json.load(f)
    arr = data.get("species", data) if isinstance(data, dict) else data
    return arr

def species_to_showdown_name(sp):
    """Convert species name to Pokemon Showdown sprite name format."""
    name = sp.get("name", "").lower()
    # Remove special characters, keep letters only
    import re
    name = re.sub(r"[^a-z0-9]", "", name)
    return name

def download_sprite(species_id, name):
    """Download animated GIF from Pokemon Showdown CDN."""
    url = f"https://play.pokemonshowdown.com/sprites/ani/{name}.gif"
    out_path = SPRITE_DIR / f"{species_id}.gif"

    if out_path.exists():
        return True  # already downloaded

    try:
        req = urllib.request.Request(url, headers={"User-Agent": "PokemonSimulator/1.0"})
        with urllib.request.urlopen(req, timeout=10) as resp:
            if resp.status == 200:
                out_path.write_bytes(resp.read())
                return True
    except Exception as e:
        pass
    return False

def main():
    species = load_species()
    print(f"{len(species)} species to check")

    downloaded = 0
    failed = 0
    skipped = 0

    for i, sp in enumerate(species):
        sid = sp["id"]
        name = species_to_showdown_name(sp)
        if not name:
            failed += 1
            continue

        out_path = SPRITE_DIR / f"{sid}.gif"
        if out_path.exists():
            skipped += 1
            if (i + 1) % 100 == 0:
                print(f"  {i+1}/{len(species)} ({downloaded} new, {skipped} cached, {failed} failed)")
            continue

        if download_sprite(sid, name):
            downloaded += 1
        else:
            failed += 1

        if (i + 1) % 50 == 0:
            print(f"  {i+1}/{len(species)} ({downloaded} new, {skipped} cached, {failed} failed)")

        time.sleep(0.02)  # minimal delay

    print(f"\nDone! {downloaded} downloaded, {skipped} cached, {failed} failed")
    print(f"Sprites: {SPRITE_DIR} ({len(list(SPRITE_DIR.glob('*.gif')))} files)")

if __name__ == "__main__":
    main()

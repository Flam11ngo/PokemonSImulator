#!/usr/bin/env python3
"""Fetch learnable moves from PokeAPI for all species and save to data/learnsets.json."""
import json, time, urllib.request, sys, os
from pathlib import Path

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
SPECIES_FILE = DATA_DIR / "species.json"
LEARNSETS_FILE = DATA_DIR / "learnsets.json"
BATCH_SIZE = 50
DELAY = 0.05  # seconds between requests (be nice to PokeAPI)

def load_species():
    with open(SPECIES_FILE) as f:
        data = json.load(f)
    arr = data.get("species", data) if isinstance(data, dict) else data
    return {s["id"]: s for s in arr}

def fetch_pokemon(species_id):
    url = f"https://pokeapi.co/api/v2/pokemon/{species_id}"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "PokemonSimulator/1.0"})
        with urllib.request.urlopen(req, timeout=15) as resp:
            return json.loads(resp.read())
    except Exception as e:
        print(f"  ❌ species {species_id}: {e}")
        return None

def extract_moves(payload):
    """Extract move IDs from PokeAPI pokemon response."""
    moves = []
    for entry in payload.get("moves", []):
        move_url = entry.get("move", {}).get("url", "")
        # URL format: https://pokeapi.co/api/v2/move/{id}/
        try:
            move_id = int(move_url.rstrip("/").split("/")[-1])
            moves.append(move_id)
        except (ValueError, IndexError):
            pass
    return sorted(moves)

def main():
    species = load_species()
    print(f"Loaded {len(species)} species from {SPECIES_FILE}")

    # Load existing learnsets
    existing = {}
    if LEARNSETS_FILE.exists():
        with open(LEARNSETS_FILE) as f:
            existing = json.load(f)
        print(f"Existing learnsets: {len(existing)} entries")

    # Fetch learnsets
    total = len(species)
    fetched = 0
    for i, (sid, sp) in enumerate(species.items()):
        if str(sid) in existing:
            continue

        payload = fetch_pokemon(sid)
        if payload:
            moves = extract_moves(payload)
            existing[str(sid)] = moves
            fetched += 1

        if (i + 1) % 10 == 0:
            print(f"  Progress: {i+1}/{total} ({fetched} fetched this run)")

        time.sleep(DELAY)

    # Save
    with open(LEARNSETS_FILE, "w") as f:
        json.dump(existing, f, indent=2)

    # Update species.json with learnableMoves
    updated = 0
    for sid_str, moves in existing.items():
        sid = int(sid_str)
        if sid in species and not species[sid].get("learnableMoves"):
            species[sid]["learnableMoves"] = moves
            updated += 1

    # Write updated species.json
    species_list = list(species.values())
    with open(SPECIES_FILE, "w") as f:
        json.dump({"species": species_list}, f, ensure_ascii=False, indent=2)

    print(f"\n✅ Done! {len(existing)} species with learnsets, {updated} updated in species.json")
    print(f"   Learnset file: {LEARNSETS_FILE}")
    total_moves = sum(len(v) for v in existing.values())
    print(f"   Total move entries: {total_moves}")

if __name__ == "__main__":
    main()

"""
Export Smogon meta data as JSON for the data daemon.
Output: data/smogon_meta.json — species rankings + per-species top moves/abilities/items/spreads
Usage: py scripts/py/export_smogon_meta.py
"""
import json, os, sys, re
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'smogon_stats'))

from queries import get_pokemon_ranking, get_pokemon_detail

SMOGON_DB = os.path.join(os.path.dirname(__file__), '..', '..', 'smogon_stats', 'gen91v1_stats.sqlite')
OUT_FILE = os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'smogon_meta.json')

def norm(n):
    return re.sub(r'[^a-z0-9]', '', n.lower())

def main():
    # Top 100 species
    ranking = get_pokemon_ranking(SMOGON_DB, "smogon", "2026-05", 1500, limit=100)
    total_usage = sum(r["usage"] for r in ranking)

    species = []
    for r in ranking:
        name = r["name"]
        weight = r["usage"] / total_usage if total_usage > 0 else 0
        entry = {
            "name": name,
            "weight": round(weight, 6),
            "usage_pct": round(r["usage"] * 100, 2),
            "chinese_name": r.get("chinese_name", ""),
        }
        try:
            detail = get_pokemon_detail(SMOGON_DB, name, "smogon", "2026-05", 1500)
            if detail:
                entry["moves"] = [{"name": m["name"], "usage": m["usage"]} for m in detail.get("moves", [])[:10]]
                entry["abilities"] = [{"name": a["name"], "usage": a["usage"]} for a in detail.get("abilities", [])[:10]]
                entry["items"] = [{"name": i["name"], "usage": i["usage"]} for i in detail.get("items", [])[:10]]
                entry["spreads"] = [{"nature": s["nature"], "evs": s["evs"], "usage": s["usage"]} for s in detail.get("spreads", [])[:10]]
        except Exception as e:
            print(f"  WARN {name}: {e}")
        species.append(entry)

    with open(OUT_FILE, 'w', encoding='utf-8') as f:
        json.dump({"species": species}, f, ensure_ascii=False)
    print(f"Exported {len(species)} species to {OUT_FILE}")
    print(f"Total usage sum: {total_usage:.4f}")

if __name__ == "__main__":
    main()

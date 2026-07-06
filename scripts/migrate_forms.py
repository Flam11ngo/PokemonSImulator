"""
Add form mapping table to pokemon.db and populate it with known Smogon forms.
Run once: python scripts/migrate_forms.py
"""
import sqlite3
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DB_PATH = PROJECT_ROOT / "data" / "pokemon.db"
BACKUP_PATH = PROJECT_ROOT / "data" / "pokemon.db.forms_backup"

# Form mappings: Showdown form name → (base species name, sprite filename)
# Sprite filename is what Showdown CDN uses (lowercase, hyphens preserved)
FORM_MAPPINGS = {
    # Regional forms
    "Arcanine-Hisui":        ("Arcanine",        "arcanine-hisui"),
    "Articuno-Galar":        ("Articuno",        "articuno-galar"),
    "Avalugg-Hisui":         ("Avalugg",         "avalugg-hisui"),
    "Goodra-Hisui":          ("Goodra",          "goodra-hisui"),
    "Lilligant-Hisui":       ("Lilligant",       "lilligant-hisui"),
    "Moltres-Galar":         ("Moltres",         "moltres-galar"),
    "Ninetales-Alola":       ("Ninetales",       "ninetales-alola"),
    "Samurott-Hisui":        ("Samurott",        "samurott-hisui"),
    "Slowking-Galar":        ("Slowking",        "slowking-galar"),
    "Typhlosion-Hisui":      ("Typhlosion",      "typhlosion-hisui"),
    "Zapdos-Galar":          ("Zapdos",          "zapdos-galar"),
    "Zoroark-Hisui":         ("Zoroark",         "zoroark-hisui"),
    "Golem-Alola":           ("Golem",           "golem-alola"),
    # Form changes
    "Hoopa-Unbound":         ("Hoopa",           "hoopa-unbound"),
    "Thundurus-Therian":     ("Thundurus",       "thundurus-therian"),
    "Rotom-Wash":            ("Rotom",           "rotom-wash"),
    "Basculegion-F":         ("Basculegion",     "basculegion-f"),
    "Ursaluna-Bloodmoon":    ("Ursaluna",        "ursaluna-bloodmoon"),
    # Ogerpon masks
    "Ogerpon-Wellspring":    ("Ogerpon",         "ogerpon-wellspring"),
    # Paldean Tauros
    "Tauros-Paldea-Aqua":    ("Tauros",          "tauros-paldeaaqua"),
}

def main():
    # Backup
    import shutil
    shutil.copy2(DB_PATH, BACKUP_PATH)
    print(f"Backed up to {BACKUP_PATH}")

    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    # Create alias table
    cur.execute("""
        CREATE TABLE IF NOT EXISTS species_aliases (
            alias_name TEXT PRIMARY KEY,
            species_id INTEGER NOT NULL,
            sprite_name TEXT NOT NULL,
            FOREIGN KEY (species_id) REFERENCES species(id)
        )
    """)

    # Look up base species IDs
    cur.execute("SELECT id, name FROM species")
    species_by_name = {r["name"].lower(): r["id"] for r in cur.fetchall()}

    inserted = 0
    for alias_name, (base_name, sprite_name) in sorted(FORM_MAPPINGS.items()):
        base_id = species_by_name.get(base_name.lower())
        if not base_id:
            print(f"  WARNING: base species '{base_name}' not found for alias '{alias_name}'")
            continue
        cur.execute(
            "INSERT OR REPLACE INTO species_aliases (alias_name, species_id, sprite_name) VALUES (?, ?, ?)",
            (alias_name, base_id, sprite_name),
        )
        inserted += 1

    conn.commit()

    # Verify
    cur.execute("SELECT * FROM species_aliases ORDER BY alias_name")
    rows = cur.fetchall()
    print(f"\nCreated {len(rows)} aliases:")
    for r in rows:
        print(f"  {r['alias_name']:30s} -> species #{r['species_id']}  sprite={r['sprite_name']}")

    conn.close()
    print(f"\nDone. {inserted} aliases created.")

if __name__ == "__main__":
    main()

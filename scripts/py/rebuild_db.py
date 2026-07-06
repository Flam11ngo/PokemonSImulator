#!/usr/bin/env python3
"""Rebuild SQLite game data tables from Showdown JSON dump.
- Species: base forms only (one per NatDex number)
- Learnsets: merged across all forms sharing same NatDex number
- Keeps user/team tables intact
Usage: python scripts/py/rebuild_db.py [data/showdown_data.json] [data/pokemon.db]
"""
import json, sqlite3, sys, os

JSON_PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'showdown_data.json')
DB_PATH = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'pokemon.db')

with open(JSON_PATH, 'r', encoding='utf-8') as f:
    dump = json.load(f)

conn = sqlite3.connect(DB_PATH)

# Drop only game-data tables (keep users, user_teams, sqlite_sequence)
conn.execute("DROP TABLE IF EXISTS species")
conn.execute("DROP TABLE IF EXISTS moves")
conn.execute("DROP TABLE IF EXISTS abilities")
conn.execute("DROP TABLE IF EXISTS items")
conn.execute("DROP TABLE IF EXISTS learnsets")
conn.execute("DROP TABLE IF EXISTS species_abilities")

# ── Schema (same as before, compatible with server) ──
conn.execute("""CREATE TABLE species (
    id INTEGER PRIMARY KEY, name TEXT, type1 TEXT, type2 TEXT,
    base_hp INTEGER, base_atk INTEGER, base_def INTEGER,
    base_spa INTEGER, base_spd INTEGER, base_spe INTEGER,
    height REAL DEFAULT 0, weight REAL DEFAULT 0, hidden_ability INTEGER DEFAULT 0)""")
conn.execute("""CREATE TABLE moves (
    id INTEGER PRIMARY KEY, name TEXT, type TEXT, category TEXT,
    power INTEGER, accuracy INTEGER, pp INTEGER, priority INTEGER DEFAULT 0,
    effect TEXT DEFAULT '', description TEXT DEFAULT '')""")
conn.execute("CREATE TABLE abilities (id INTEGER PRIMARY KEY, name TEXT, description TEXT)")
conn.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, description TEXT, is_battle INTEGER DEFAULT 1)")
conn.execute("CREATE TABLE learnsets (species_id INTEGER, move_id INTEGER, PRIMARY KEY(species_id, move_id))")
conn.execute("CREATE TABLE species_abilities (species_id INTEGER, ability_id INTEGER, is_hidden INTEGER DEFAULT 0, PRIMARY KEY(species_id, ability_id))")

# ── Species: base forms only (deduplicate by ID) ──
species_seen = set()
species_count = 0
for s in dump.get('species', []):
    sid = s['id']
    if sid <= 0 or sid in species_seen:
        continue
    species_seen.add(sid)

    raw_bs = s.get('baseStats', [0]*6) or [0]*6
    if isinstance(raw_bs, dict):
        bs = [raw_bs.get(k, 0) for k in ('hp','atk','def','spa','spd','spe')]
    else:
        bs = list(raw_bs)[:6] + [0]*max(0, 6-len(raw_bs))
    types = s.get('types', []) or []

    conn.execute("INSERT OR REPLACE INTO species VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)",
        (sid, s['name'], types[0] if len(types)>0 else '', types[1] if len(types)>1 else '',
         bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], 0, 0, s.get('hiddenAbility', 0)))
    species_count += 1

print(f"Species: {species_count} base forms (deduplicated from {len(dump.get('species',[]))} total)")

# ── Moves (deduplicate by ID) ──
move_ids_seen = set()
move_count = 0
for m in dump.get('moves', []):
    mid = m['id']
    if mid <= 0 or mid in move_ids_seen:
        continue
    move_ids_seen.add(mid)
    conn.execute("INSERT OR REPLACE INTO moves VALUES (?,?,?,?,?,?,?,?,?,?)",
        (mid, m['name'], m.get('type',''), m.get('category',''),
         m.get('power',0), m.get('accuracy',0), m.get('pp',0), 0, '', m.get('description','')))
    move_count += 1

print(f"Moves: {move_count}")

# ── Abilities (deduplicate by ID) ──
ab_ids_seen = set()
ab_count = 0
for a in dump.get('abilities', []):
    aid = a['id']
    if aid <= 0 or aid in ab_ids_seen:
        continue
    ab_ids_seen.add(aid)
    conn.execute("INSERT OR REPLACE INTO abilities VALUES (?,?,?)", (aid, a['name'], a.get('description','')))
    ab_count += 1

print(f"Abilities: {ab_count}")

# ── Items (deduplicate by ID, keep first = modern name) ──
item_ids_seen = set()
item_count = 0
for it in dump.get('items', []):
    iid = it['id']
    if iid in item_ids_seen:
        continue
    item_ids_seen.add(iid)
    conn.execute("INSERT OR REPLACE INTO items VALUES (?,?,?,?)", (iid, it['name'], it.get('description',''), 1))
    item_count += 1

print(f"Items: {item_count}")

# ── Learnsets (already merged in JSON, keyed by species Num) ──
ls_count = 0
for sid, moves in dump.get('learnsets', {}).items():
    for mid in moves:
        conn.execute("INSERT OR REPLACE INTO learnsets VALUES (?,?)", (int(sid), mid))
        ls_count += 1

print(f"Learnsets: {ls_count} rows for {len(dump.get('learnsets',{}))} species")

# ── Species Abilities ──
sa_count = 0
for entries in dump.get('speciesAbilities', {}).values():
    for ab in entries:
        conn.execute("INSERT OR REPLACE INTO species_abilities VALUES (?,?,?)",
            (ab['speciesId'], ab['abilityId'], 1 if ab.get('isHidden') else 0))
        sa_count += 1

print(f"Species abilities: {sa_count} rows")

conn.commit()

# ── Verification ──
verify = conn.execute("""
    SELECT s.id, s.name, COUNT(l.move_id) as moves
    FROM species s LEFT JOIN learnsets l ON s.id = l.species_id
    WHERE s.id IN (6, 25, 150, 382, 383, 384, 493, 888, 889, 898)
    GROUP BY s.id ORDER BY s.id
""").fetchall()
print("\nVerification:")
for row in verify:
    print(f"  {row[1]} ({row[0]}): {row[2]} moves")

conn.close()
print(f"\nRebuilt {DB_PATH} successfully!")

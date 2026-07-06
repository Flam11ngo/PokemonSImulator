#!/usr/bin/env python3
"""
One-shot migration: convert pokemon.db from PokeAPI IDs → Showdown IDs.

Step 1: Rebuild game tables (species/moves/abilities/items/learnsets) from showdown_data.json
Step 2: Migrate user_teams — convert all species/move/item/ability IDs to Showdown numbering
         by matching names from the old PokeAPI tables.
Step 3: Verify

Usage: python scripts/py/migrate_to_showdown.py [data/showdown_data.json] [data/pokemon.db]
"""
import json, sqlite3, sys, os, shutil

JSON_PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'showdown_data.json')
DB_PATH = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'pokemon.db')

# Backup
backup_path = DB_PATH + ".pre_showdown_migration.bak"
print(f"Backing up {DB_PATH} → {backup_path}")
shutil.copy2(DB_PATH, backup_path)

with open(JSON_PATH, 'r', encoding='utf-8') as f:
    dump = json.load(f)

conn = sqlite3.connect(DB_PATH)
conn.row_factory = sqlite3.Row

# ── Build name→Showdown ID maps ──
sd_species_by_name = {}
for s in dump.get('species', []):
    key = s['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
    sd_species_by_name[key] = s['id']

sd_moves_by_name = {}
for m in dump.get('moves', []):
    key = m['name'].lower().replace(' ', '').replace('-', '')
    sd_moves_by_name[key] = m['id']

sd_abilities_by_name = {}
for a in dump.get('abilities', []):
    key = a['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
    sd_abilities_by_name[key] = a['id']

sd_items_by_name = {}
for it in dump.get('items', []):
    key = it['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
    sd_items_by_name[key] = it['id']

# ── Read old PokeAPI names from current DB ──
old_species = {}
try:
    for row in conn.execute("SELECT id, name FROM species"):
        key = row['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
        old_species[row['id']] = key
except: pass

old_moves = {}
try:
    for row in conn.execute("SELECT id, name FROM moves"):
        key = row['name'].lower().replace(' ', '').replace('-', '')
        old_moves[row['id']] = key
except: pass

old_abilities = {}
try:
    for row in conn.execute("SELECT id, name FROM abilities"):
        key = row['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
        old_abilities[row['id']] = key
except: pass

old_items = {}
try:
    for row in conn.execute("SELECT id, name FROM items"):
        key = row['name'].lower().replace(' ', '').replace('-', '').replace("'", '')
        old_items[row['id']] = key
except: pass

def pokeapi_to_sd(pokeapi_id, old_map, sd_map, field_name):
    """Convert a single PokeAPI ID to Showdown ID via name matching."""
    if pokeapi_id is None or pokeapi_id == 0:
        return 0
    name_key = old_map.get(pokeapi_id)
    if not name_key:
        return pokeapi_id  # keep as-is if unknown
    sd_id = sd_map.get(name_key)
    if sd_id:
        return sd_id
    print(f"  [WARN] {field_name}: no Showdown match for '{name_key}' (old id={pokeapi_id})")
    return pokeapi_id

# ── Migrate user teams ──
try:
    rows = conn.execute("SELECT username, team_name, team_json FROM user_teams").fetchall()
except:
    rows = []

migrated = 0
for r in rows:
    try:
        team = json.loads(r['team_json'])
    except:
        continue
    changed = False
    for p in team:
        if p.get('speciesID') and p['speciesID'] in old_species:
            new_id = pokeapi_to_sd(p['speciesID'], old_species, sd_species_by_name, 'species')
            if new_id != p['speciesID']:
                p['speciesID'] = new_id; changed = True
        if p.get('ability') and p['ability'] in old_abilities:
            new_id = pokeapi_to_sd(p['ability'], old_abilities, sd_abilities_by_name, 'ability')
            if new_id != p['ability']:
                p['ability'] = new_id; changed = True
        if p.get('item') and p['item'] in old_items:
            new_id = pokeapi_to_sd(p['item'], old_items, sd_items_by_name, 'item')
            if new_id != p['item']:
                p['item'] = new_id; changed = True
        if p.get('moves'):
            new_moves = []
            for mid in p['moves']:
                if mid in old_moves:
                    new_mid = pokeapi_to_sd(mid, old_moves, sd_moves_by_name, 'move')
                    new_moves.append(new_mid)
                else:
                    new_moves.append(mid)
            if new_moves != p['moves']:
                p['moves'] = new_moves; changed = True
    if changed:
        conn.execute("UPDATE user_teams SET team_json=? WHERE username=? AND team_name=?",
                     (json.dumps(team), r['username'], r['team_name']))
        migrated += 1

conn.commit()
print(f"Migrated {migrated}/{len(rows)} user teams")

# ── Rebuild game tables from Showdown dump ──
conn.execute("DROP TABLE IF EXISTS species")
conn.execute("DROP TABLE IF EXISTS moves")
conn.execute("DROP TABLE IF EXISTS abilities")
conn.execute("DROP TABLE IF EXISTS items")
conn.execute("DROP TABLE IF EXISTS learnsets")
conn.execute("DROP TABLE IF EXISTS species_abilities")

conn.execute("""CREATE TABLE species (
    id INTEGER PRIMARY KEY, name TEXT, type1 TEXT, type2 TEXT,
    base_hp INTEGER, base_atk INTEGER, base_def INTEGER,
    base_spa INTEGER, base_spd INTEGER, base_spe INTEGER,
    height REAL, weight REAL, hidden_ability INTEGER)""")
conn.execute("""CREATE TABLE moves (
    id INTEGER PRIMARY KEY, name TEXT, type TEXT, category TEXT,
    power INTEGER, accuracy INTEGER, pp INTEGER, priority INTEGER,
    effect TEXT, description TEXT)""")
conn.execute("CREATE TABLE abilities (id INTEGER PRIMARY KEY, name TEXT, description TEXT)")
conn.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, description TEXT, is_battle INTEGER DEFAULT 1)")
conn.execute("CREATE TABLE learnsets (species_id INTEGER, move_id INTEGER, PRIMARY KEY(species_id, move_id))")
conn.execute("CREATE TABLE species_abilities (species_id INTEGER, ability_id INTEGER, is_hidden INTEGER, PRIMARY KEY(species_id, ability_id))")

for s in dump.get('species', []):
    raw_bs = s.get('baseStats', [0]*6) or [0]*6
    # baseStats may be dict {hp,atk,...} or ordered list; normalize to list
    if isinstance(raw_bs, dict):
        bs = [raw_bs.get(k, 0) for k in ('hp','atk','def','spa','spd','spe')]
    else:
        bs = list(raw_bs)[:6] + [0]*max(0, 6-len(raw_bs))
    types = s.get('types', []) or []
    conn.execute("INSERT OR REPLACE INTO species VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)",
        (s['id'], s['name'], types[0] if len(types)>0 else '', types[1] if len(types)>1 else '',
         bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], 0, 0, s.get('hiddenAbility', 0)))

for m in dump.get('moves', []):
    conn.execute("INSERT OR REPLACE INTO moves VALUES (?,?,?,?,?,?,?,?,?,?)",
        (m['id'], m['name'], m.get('type',''), m.get('category',''),
         m.get('power',0), m.get('accuracy',0), m.get('pp',0), 0, '', m.get('description','')))

for a in dump.get('abilities', []):
    conn.execute("INSERT OR REPLACE INTO abilities VALUES (?,?,?)", (a['id'], a['name'], a.get('description','')))

for it in dump.get('items', []):
    conn.execute("INSERT OR REPLACE INTO items VALUES (?,?,?,?)", (it['id'], it['name'], it.get('description',''), 1))

for sid, moves in dump.get('learnsets', {}).items():
    for mid in moves:
        try: conn.execute("INSERT OR REPLACE INTO learnsets VALUES (?,?)", (int(sid), mid))
        except: pass

for entries in dump.get('speciesAbilities', {}).values():
    for ab in entries:
        try: conn.execute("INSERT OR REPLACE INTO species_abilities VALUES (?,?,?)",
                (ab['speciesId'], ab['abilityId'], 1 if ab.get('isHidden') else 0))
        except: pass

conn.commit()

# ── Verify ──
s_count = conn.execute("SELECT COUNT(*) FROM species").fetchone()[0]
m_count = conn.execute("SELECT COUNT(*) FROM moves").fetchone()[0]
a_count = conn.execute("SELECT COUNT(*) FROM abilities").fetchone()[0]
i_count = conn.execute("SELECT COUNT(*) FROM items").fetchone()[0]
t_count = conn.execute("SELECT COUNT(*) FROM user_teams").fetchone()[0]
print(f"Done: {s_count} species, {m_count} moves, {a_count} abilities, {i_count} items, {t_count} teams")
print(f"Old DB backed up to {backup_path}")
conn.close()

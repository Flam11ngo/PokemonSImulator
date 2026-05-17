#!/usr/bin/env python3
"""Compare engine enums vs data JSONs to find gaps."""
import json, re

def parse_enum(filepath, enum_name):
    with open(filepath) as f:
        text = f.read()
    pat = rf'enum class {enum_name}\s*\{{([^}}]+)\}}'
    m = re.search(pat, text, re.DOTALL)
    if not m:
        return set()
    entries = set()
    for line in m.group(1).split('\n'):
        line = line.strip().strip(',')
        if line and not line.startswith('//') and line not in ('Count',):
            name = line.split('=')[0].strip()
            if name:
                entries.add(name.lower())
    return entries

abil_engine = parse_enum('include/battle/Abilities.h', 'AbilityType')
item_engine = parse_enum('include/battle/Items.h', 'ItemType')

with open('data/abilities.json') as f:
    abil_data = json.load(f)
abil_json = {a['apiName'].lower().replace('-',''): a for a in abil_data.get('abilities', abil_data) if isinstance(a, dict)}

with open('data/items.json') as f:
    item_data = json.load(f)
item_json = {i['name'].lower(): i for i in item_data.get('items', item_data) if isinstance(i, dict)}

print("=== Abilities in data but NOT in engine ===")
missing_abil = []
for slug, a in abil_json.items():
    if slug not in abil_engine:
        missing_abil.append(a)
print(f"Missing: {len(missing_abil)} / {len(abil_json)}")
for a in missing_abil[:40]:
    print(f"  {a['id']}: {a['name']} ({a.get('apiName','?')})")

print("\n=== Items: FROZEN (see AGENTS.md Decision Log) ===")
print("Remaining battle items are non-PvP. Engine items: 145/304 competitive.")

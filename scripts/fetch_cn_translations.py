"""
Fetch Chinese names/descriptions for moves, items, abilities from PokeAPI.
Adds chinese_name + chinese_desc columns to pokemon.db.
"""
import sqlite3, urllib.request, json, time, re, sys
from concurrent.futures import ThreadPoolExecutor, as_completed

DB = 'E:/PokemonSImulator/data/pokemon.db'
BASE = 'https://pokeapi.co/api/v2'
WORKERS = 10

def fetch_json(url):
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'PokemonSim/1.0'})
        with urllib.request.urlopen(req, timeout=10) as r:
            return json.load(r)
    except:
        return None

def get_chinese(data):
    """Extract zh-hans name and flavor text from PokeAPI response."""
    cn_name = ''
    cn_desc = ''
    if not data:
        return cn_name, cn_desc
    for n in data.get('names', []):
        if n['language']['name'] == 'zh-hans':
            cn_name = n['name']
            break
    for ft in data.get('flavor_text_entries', []):
        if ft['language']['name'] == 'zh-hans':
            cn_desc = ft.get('flavor_text', ft.get('text', '')).replace('\n', ' ').replace('\f', ' ')
            break
    if not cn_desc:
        for e in data.get('effect_entries', []):
            if e['language']['name'] == 'zh-hans':
                cn_desc = e.get('short_effect', e.get('effect', ''))
                break
    return cn_name, cn_desc

def fetch_move(move_id, name_hint=''):
    """Try PokeAPI by ID, fallback to name-based lookup."""
    data = fetch_json(f'{BASE}/move/{move_id}')
    if data:
        cn_name, cn_desc = get_chinese(data)
        if cn_name:
            return move_id, cn_name, cn_desc
    # Try by English name
    if name_hint:
        name_key = name_hint.lower().replace(' ', '-')
        data = fetch_json(f'{BASE}/move/{name_key}')
        if data:
            cn_name, cn_desc = get_chinese(data)
            return move_id, cn_name, cn_desc
    return move_id, '', ''

def fetch_item(item_id, name_hint=''):
    # PokeAPI item IDs differ from Showdown, try by name
    name_key = name_hint.lower().replace(' ', '-').replace("'", '')
    data = fetch_json(f'{BASE}/item/{name_key}')
    if not data:
        data = fetch_json(f'{BASE}/item/{item_id}')
    if data:
        cn_name, cn_desc = get_chinese(data)
        return item_id, cn_name, cn_desc
    return item_id, '', ''

def fetch_ability(ab_id, name_hint=''):
    name_key = name_hint.lower().replace(' ', '-').replace("'", '')
    data = fetch_json(f'{BASE}/ability/{name_key}')
    if not data:
        data = fetch_json(f'{BASE}/ability/{ab_id}')
    if data:
        cn_name, cn_desc = get_chinese(data)
        return ab_id, cn_name, cn_desc
    return ab_id, '', ''

def main():
    conn = sqlite3.connect(DB)
    conn.row_factory = sqlite3.Row

    # Add columns if needed
    for table in ['moves', 'items', 'abilities']:
        for col in ['chinese_name', 'chinese_desc']:
            try:
                conn.execute(f'ALTER TABLE {table} ADD COLUMN {col} TEXT DEFAULT ""')
            except:
                pass

    # Fetch moves
    moves = [(r['id'], r['name']) for r in conn.execute('SELECT id, name FROM moves WHERE id > 0')]
    print(f'Fetching {len(moves)} moves...')
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = {pool.submit(fetch_move, mid, name): mid for mid, name in moves}
        ok = 0
        for i, f in enumerate(as_completed(futures)):
            mid, cn_name, cn_desc = f.result()
            if cn_name:
                conn.execute('UPDATE moves SET chinese_name=?, chinese_desc=? WHERE id=?',
                             (cn_name, cn_desc, mid))
                ok += 1
            if (i+1) % 100 == 0:
                print(f'  moves: {i+1}/{len(moves)} ({ok} translated)')
    conn.commit()
    print(f'Moves done: {ok}/{len(moves)} translated')

    # Fetch items
    items = [(r['id'], r['name']) for r in conn.execute('SELECT id, name FROM items WHERE id > 0')]
    print(f'\nFetching {len(items)} items...')
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = {pool.submit(fetch_item, iid, name): iid for iid, name in items}
        ok = 0
        for i, f in enumerate(as_completed(futures)):
            iid, cn_name, cn_desc = f.result()
            if cn_name:
                conn.execute('UPDATE items SET chinese_name=?, chinese_desc=? WHERE id=?',
                             (cn_name, cn_desc, iid))
                ok += 1
            if (i+1) % 100 == 0:
                print(f'  items: {i+1}/{len(items)} ({ok} translated)')
    conn.commit()
    print(f'Items done: {ok}/{len(items)} translated')

    # Fetch abilities
    abilities = [(r['id'], r['name']) for r in conn.execute('SELECT id, name FROM abilities WHERE id > 0')]
    print(f'\nFetching {len(abilities)} abilities...')
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = {pool.submit(fetch_ability, aid, name): aid for aid, name in abilities}
        ok = 0
        for i, f in enumerate(as_completed(futures)):
            aid, cn_name, cn_desc = f.result()
            if cn_name:
                conn.execute('UPDATE abilities SET chinese_name=?, chinese_desc=? WHERE id=?',
                             (cn_name, cn_desc, aid))
                ok += 1
            if (i+1) % 50 == 0:
                print(f'  abilities: {i+1}/{len(abilities)} ({ok} translated)')
    conn.commit()
    print(f'Abilities done: {ok}/{len(abilities)} translated')

    conn.close()
    print('\nDone!')

if __name__ == '__main__':
    main()

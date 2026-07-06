"""Fix items that were incorrectly excluded from battle list (Choice Scarf, Red Card, etc.)"""
import json, re, sqlite3
from PIL import Image, ImageDraw, ImageFont

SIZE, COLS = 24, 20

with open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js', 'r') as f:
    content = f.read()
data = json.loads(content[content.index('{"url"'):content.rindex('}') + 1])
mapping = data['mapping']

def norm(n):
    return re.sub(r'[^a-z0-9]', '', n.lower())

NOT_BATTLE = [
    r'ball$', r'.+-ball$', r'stone$', r'.+-stone$', r'mail$', r'.+-mail$',
    r'fossil', r'ite$', r'.+ite$', r'ium.z$', r'iumz$',
    r'^tr\d', r'berry$', r'.+-berry$', r'gem$', r'.+-gem$',
    r'wing$', r'memory$', r'drive$', r'rod$', r'bike$',
    r'key$', r'ticket$', r'pass$',
    r'candy$', r'bone$', r'mushroom$', r'stardust$', r'star-piece$',
    r'comet-shard$', r'nugget$', r'pearl$', r'big-nugget', r'big-pearl',
    r'pp-up$', r'pp-max$', r'hp-up$', r'protein$', r'iron$',
    r'carbos$', r'calcium$', r'zinc$', r'honey$', r'repel$',
    r'potion$', r'ether$', r'elixir$', r'revive$', r'shard$', r'shoal',
    r'heart-scale$', r'mulch$', r'apricorn', r'flute$',
    r'doll$', r'toy$', r'sack$', r'case$', r'pouch$', r'album$', r'kit$',
    r'sounds$', r'checker$', r'radar$', r'seeker$', r'xtransceiver$',
    r'vs-recorder$', r'vs-seeker$', r'poke-radar$', r'go-goggles$',
    r'capsule$', r'chip$', r'goods$', r'coupon', r'parcel',
    r'letter$', r'coin-case$', r'devon-', r'galactic-', r'storage-',
    r'sprayduck$', r'works-key$',
    r'sweet-apple$', r'tart-apple$', r'syrupy-apple$',
    r'.+-sweet$', r'cracked-pot$', r'chipped-pot$',
    r'whipped-dream$', r'sachet$', r'galarica-cuff$', r'galarica-wreath$',
    r'malicious-armor$', r'auspicious-armor$',
    r'unremarkable-teacup$', r'masterpiece-teacup$', r'metal-alloy$',
    r'deep-sea-tooth$', r'deep-sea-scale$',
    r'electirizer$', r'magmarizer$', r'protector$', r'reaper-cloth$',
    r'up-grade$', r'dubious-disc$', r'prism-scale$', r'dragon-scale$',
    r'kings-rock$', r'metal-coat$', r'bottle-cap$',
    r'macho-brace$', r'power-bracer$', r'power-belt$', r'power-lens$',
    r'power-band$', r'power-anklet$', r'power-weight$',
    r'red-orb$', r'blue-orb$', r'jade-orb$',
    r'berry-juice$', r'old-amber$', r'rare-bone$',
    r'dire-hit', r'guard-spec', r'x-attack', r'x-defense', r'x-sp-atk',
    r'x-sp-def', r'x-speed', r'x-accuracy',
    r'ability-urge', r'item-urge', r'item-drop', r'reset-urge',
    r'lucky-punch', r'lucky-egg', r'amulet-coin',
    r'exp-share', r'soothe-bell', r'cleanse-tag',
    r'pal-pad', r'journal', r'rule-book', r'point-card',
    r'town-map', r'member-card', r'azure-flute',
    r'lunar-wing', r'contest-pass', r'magma-emblem', r'old-sea-map',
    r'secret-key', r'apricorn-box', r'berry-pots', r'red-apricorn',
    r'unown-report', r'clear-bell', r'tidal-bell', r'blue-card',
    r'red-scale', r'mystery-egg', r'gb-sounds', r'lock-capsule',
    r'photo-album', r'mach-bike', r'acro-bike', r'wailmer-pail',
    r'soot-sack', r'pokeblock-case', r'eon-ticket',
    r'scanner', r'meteorite', r'devon-scope', r'poke-flute',
    r'bike-voucher', r'gold-teeth', r'lift-key', r'silph-scope',
    r'fame-checker', r'berry-pouch', r'teachy-tv',
    r'tri-pass', r'rainbow-pass', r'tea', r'mysticticket', r'auroraticket',
    r'powder-jar', r'ruby', r'sapphire',
    r'sweet-heart', r'health-wing', r'muscle-wing', r'resist-wing',
    r'genius-wing', r'clever-wing', r'swift-wing', r'pretty-wing',
    r'liberty-pass', r'pass-orb', r'prop-case', r'dragon-skull',
    r'relic-copper', r'relic-silver', r'relic-gold', r'relic-vase',
    r'relic-band', r'relic-statue', r'relic-crown', r'casteliacone',
    r'gram-1', r'gram-2', r'gram-3',
    r'explorer-kit', r'loot-sack', r'red-chain',
    r'gracidea', r'silver-wing', r'rainbow-wing',
    r'douse-drive', r'shock-drive', r'burn-drive', r'chill-drive',
    r'vile-vial', r'crucibellite', r'fairy-feather',
    r'old-gateau', r'lava-cookie', r'fresh-water', r'soda-pop',
    r'lemonade', r'moomoo-milk', r'energy-root', r'heal-powder',
    r'revival-herb', r'sacred-ash', r'full-restore', r'full-heal',
    r'antidote', r'burn-heal', r'ice-heal', r'awakening',
    r'escape-rope', r'max-mushrooms',
]

# Known battle items that get false-positive excluded
BATTLE_WHITELIST = [
    'choice scarf', 'red card', 'safety goggles', 'snowball',
    'scope lens', 'silver powder',
]

def is_battle(name):
    nl = name.lower().replace(' ', '-')
    if name.lower() in BATTLE_WHITELIST:
        return True
    for pat in NOT_BATTLE:
        if re.search(pat, nl):
            return False
    return True

conn = sqlite3.connect('E:/PokemonSImulator/data/pokemon.db')
all_items = [(r[0], r[1]) for r in conn.execute('SELECT id, name FROM items ORDER BY id')]
conn.close()

battle = [(i, n) for i, n in all_items if is_battle(n)]
missing = [(i, n) for i, n in battle if norm(n) not in mapping and n.lower().replace(' ', '-').replace("'", '').replace('.', '') not in mapping]

print(f'Battle items: {len(battle)}, missing from sheet: {len(missing)}')
for i, n in missing:
    nl = n.lower().replace(' ', '-')
    print(f'  {i:4d} | {n:30s} | key: {norm(n)}')

# Add missing items to sheet with placeholders
if missing:
    sheet_img = Image.open('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png')
    current_rows = sheet_img.size[1] // SIZE
    current_items = len(set(v for v in mapping.values()))

    try:
        font = ImageFont.truetype('arial.ttf', 9)
    except:
        font = ImageFont.load_default()

    for idx, (item_id, name) in enumerate(missing):
        slot = current_items + idx
        col, row = slot % COLS, slot // COLS
        x, y = col * SIZE, row * SIZE

        # Extend sheet if needed
        if row >= current_rows:
            new_h = (row + 1) * SIZE
            new_sheet = Image.new('RGBA', (COLS * SIZE, new_h), (0, 0, 0, 0))
            new_sheet.paste(sheet_img, (0, 0))
            sheet_img = new_sheet
            current_rows = row + 1

        canvas = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
        draw = ImageDraw.Draw(canvas)
        draw.rounded_rectangle([1, 1, SIZE - 2, SIZE - 2], radius=3, fill=(100, 160, 200, 255))
        lt = name.strip()[0].upper() if name else '?'
        bb = draw.textbbox((0, 0), lt, font=font)
        draw.text(((SIZE - bb[2] + bb[0]) // 2, (SIZE - bb[3] + bb[1]) // 2), lt, fill=(255, 255, 255, 255), font=font)
        sheet_img.paste(canvas, (x, y), canvas)

        sk = norm(name)
        hk = name.lower().replace(' ', '-').replace("'", '').replace('.', '')
        mapping[sk] = f'-{x}px -{y}px'
        if hk != sk:
            mapping[hk] = f'-{x}px -{y}px'

    sheet_img.save('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png', 'PNG', optimize=True)

data['mapping'] = mapping
with open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js', 'w') as f:
    f.write('export const ITEM_SHEET = ' + json.dumps(data, indent=None) + ';\n')

kb = __import__('os').path.getsize('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png') // 1024
print(f'\nSheet updated: {len(mapping)} keys, {kb}KB')

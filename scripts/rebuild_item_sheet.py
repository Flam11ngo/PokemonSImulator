"""Rebuild items-sheet.png from individual sprites in sprites/items/, then delete that folder."""
from PIL import Image
import os, re, sqlite3, json, shutil

SPRITES = 'E:/PokemonSImulator/frontend/public/sprites/items'
SIZE, COLS = 24, 20

def norm(n):
    return re.sub(r'[^a-z0-9]', '', n.lower())

# Fix webp files and trailing spaces
for f in os.listdir(SPRITES):
    path = os.path.join(SPRITES, f)
    if f.endswith('.webp'):
        img = Image.open(path).convert('RGBA')
        img.save(path.replace('.webp', '.png'), 'PNG')
        os.remove(path)
        print(f'  Convert: {f} -> .png')
    if ' .png' in f:
        new = f.replace(' .png', '.png')
        os.rename(path, os.path.join(SPRITES, new))
        print(f'  Fixed space: {f}')

# Load all sprites
sprites = {}
for f in os.listdir(SPRITES):
    if f.endswith('.png'):
        sprites[f.replace('.png', '')] = Image.open(os.path.join(SPRITES, f)).convert('RGBA')

print(f'{len(sprites)} sprites loaded')

# Non-battle regex patterns
NOT_BATTLE = [
    r'ball$', r'.+-ball$', r'stone$', r'.+-stone$', r'mail$', r'.+-mail$',
    r'fossil', r'ite$', r'.+ite$', r'ium.z$', r'iumz$',
    r'^tr\d', r'berry$', r'.+-berry$', r'gem$', r'.+-gem$',
    r'wing$', r'memory$', r'drive$', r'rod$', r'bike$',
    r'key$', r'ticket$', r'pass$', r'scope$',
    r'candy$', r'bone$', r'mushroom$', r'stardust$', r'star-piece$',
    r'comet-shard$', r'nugget$', r'pearl$', r'big-nugget', r'big-pearl',
    r'pp-up$', r'pp-max$', r'hp-up$', r'protein$', r'iron$',
    r'carbos$', r'calcium$', r'zinc$', r'honey$', r'repel$',
    r'potion$', r'ether$', r'elixir$', r'revive$', r'shard$', r'shoal',
    r'heart-scale$', r'mulch$', r'scarf$', r'apricorn', r'flute$',
    r'doll$', r'toy$', r'sack$', r'case$', r'pouch$', r'album$', r'kit$',
    r'sounds$', r'checker$', r'radar$', r'seeker$', r'xtransceiver$',
    r'vs-recorder$', r'vs-seeker$', r'poke-radar$', r'go-goggles$',
    r'card$', r'capsule$', r'chip$', r'goods$', r'coupon', r'parcel',
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
    r'sprayduck', r'poffin-case', r'seal-case', r'fashion-case', r'seal-bag',
    r'pal-pad', r'journal', r'rule-book', r'point-card',
    r'town-map', r'suite-key', r'member-card', r'azure-flute',
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

def is_battle(name):
    nl = name.lower().replace(' ', '-')
    for pat in NOT_BATTLE:
        if re.search(pat, nl):
            return False
    return True

# Get DB items
conn = sqlite3.connect('E:/PokemonSImulator/data/pokemon.db')
all_items = [(r[0], r[1]) for r in conn.execute('SELECT id, name FROM items ORDER BY id')]
conn.close()

battle_items = [(i, n) for i, n in all_items if is_battle(n)]

# Match to sprites
has, missing = [], []
for item_id, name in battle_items:
    nk = norm(name)
    hk = name.lower().replace(' ', '-').replace("'", '').replace('.', '')
    found = None
    for key in [nk, hk, name.lower().replace(' ', '_'), name.lower().replace(' ', '')]:
        if key in sprites:
            found = sprites[key]
            break
    if found:
        has.append((item_id, name, found))
    else:
        missing.append((item_id, name))

print(f'Battle: {len(battle_items)} | Sprites: {len(has)} | Missing: {len(missing)}')

# Build sheet
rows = (len(has) + COLS - 1) // COLS
sheet = Image.new('RGBA', (COLS * SIZE, rows * SIZE), (0, 0, 0, 0))
mapping = {}

for idx, (item_id, name, img) in enumerate(has):
    col, row = idx % COLS, idx // COLS
    x, y = col * SIZE, row * SIZE
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    sheet.paste(img, (x, y), img)
    sk = norm(name)
    hk = name.lower().replace(' ', '-').replace("'", '').replace('.', '')
    mapping[sk] = f'-{x}px -{y}px'
    if hk != sk:
        mapping[hk] = f'-{x}px -{y}px'

# Save sheet
sheet_path = 'E:/PokemonSImulator/frontend/public/sprites/items-sheet.png'
sheet.save(sheet_path, 'PNG', optimize=True)
kb = os.path.getsize(sheet_path) // 1024

# Save mapping
data = {'url': '/sprites/items-sheet.png', 'size': SIZE, 'cols': COLS, 'mapping': mapping}
with open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js', 'w') as f:
    f.write('export const ITEM_SHEET = ' + json.dumps(data, indent=None) + ';\n')

# Delete items folder
shutil.rmtree(SPRITES)

print(f'\nSheet: {COLS*SIZE}x{rows*SIZE}px, {kb}KB')
print(f'Items: {len(has)} with sprites, {len(mapping)} lookup keys')
print(f'Deleted: sprites/items/')

if missing:
    with open('E:/PokemonSImulator/smogon_stats/missing_items.txt', 'w', encoding='utf-8') as f:
        for i, n in missing:
            f.write(f'{i:4d} | {n}\n')
    print(f'\nStill missing ({len(missing)}):')
    for i, n in missing:
        print(f'  {i:4d} | {n}')

"""Sync sprite sheet with server's get_items filter (battle items + berries)."""
import json, re, sqlite3, os
from PIL import Image, ImageDraw, ImageFont

SIZE, COLS = 24, 20

def norm(n):
    return re.sub(r'[^a-z0-9]', '', n.lower())

# Same filter as server's get_items
SKIP = ['potion','ether','elixir','revive','repel','tm','shard','nugget','pearl',
    'shoal','rare-candy','rare bone','big-mushroom','balm-mushroom','tiny-mushroom',
    'stardust','star-piece','comet-shard','pp-up','pp-max','heart-scale','honey',
    'growth-','stable-','gooey-','damp-','heat-','smooth-','icy-','ball','-ball',
    'stone','-stone','mail','fossil','-fossil','mega','-ite','ium-z','ium z',
    'z-crystal','tr','-tm','hm','data-card','coupon','key','ticket','pass',
    'letter','parcel','scope','bike','rod','flute','case','pouch','kit','sack',
    'album','goods','chip','capsule','card','candy','mushroom','bone','pearl',
    'nugget','stardust','comet','vitamin','protein','iron','calcium','zinc',
    'carbos','hp-up','exp-share','mulch','apricorn','doll','toy','contest',
    'sweet-apple','tart-apple','syrupy-apple','cracked-pot','chipped-pot',
    'whipped-dream','sachet','galarica','malicious-armor','auspicious-armor',
    'teacup','metal-alloy','deep-sea-tooth','deep-sea-scale','electirizer',
    'magmarizer','protector','reaper-cloth','up-grade','dubious-disc',
    'prism-scale','dragon-scale','kings-rock','metal-coat','bottle-cap',
    'gold-bottle-cap','macho-brace','power-bracer','power-belt','power-lens',
    'power-band','power-anklet','power-weight','-drive','drive-','memory',
    'poke-doll','poke-toy','fluffy-tail','old-gateau','lava-cookie',
    'fresh-water','soda-pop','lemonade','moomoo-milk','energy-root',
    'heal-powder','revival-herb','sacred-ash','full-restore','full-heal',
    'antidote','burn-heal','ice-heal','awakening','escape-rope','max-mushrooms',
    'berry-juice','sweet-heart','shoal-salt','shoal-shell','red-orb','blue-orb',
    'jade-orb','clear-bell','tidal-bell','blue-card','red-scale','lucky-punch',
    'lucky-egg','amulet-coin','silver-wing','rainbow-wing','soothe-bell',
    'cleanse-tag','vile-vial','crucibellite','fairy-feather']

WHITELIST = ['choice scarf','red card','safety goggles','snowball',
    'scope lens','silver powder','silk scarf','iron ball','light ball',
    'smoke ball','big root','black sludge',
    # All berries
    'cheri berry','chesto berry','pecha berry','rawst berry','aspear berry',
    'leppa berry','oran berry','persim berry','lum berry','sitrus berry',
    'figy berry','wiki berry','mago berry','aguav berry','iapapa berry',
    'razz berry','bluk berry','nanab berry','wepear berry','pinap berry',
    'pomeg berry','kelpsy berry','qualot berry','hondew berry','grepa berry',
    'tamato berry','cornn berry','magost berry','rabuta berry','nomel berry',
    'spelon berry','pamtre berry','durin berry','belue berry','occa berry',
    'passho berry','wacan berry','rindo berry','yache berry','chople berry',
    'kebia berry','shuca berry','coba berry','payapa berry','tanga berry',
    'charti berry','kasib berry','haban berry','colbur berry','babiri berry',
    'chilan berry','liechi berry','ganlon berry','salac berry','petaya berry',
    'apicot berry','lansat berry','starf berry','enigma berry','micle berry',
    'custap berry','jaboca berry','rowap berry','roseli berry','kee berry',
    'maranga berry']

def is_battle(name):
    nl = name.lower().replace(' ', '-')
    if name.lower().strip() in WHITELIST:
        return True
    for kw in SKIP:
        if kw in nl:
            return False
    return True

# Get DB items
conn = sqlite3.connect('E:/PokemonSImulator/data/pokemon.db')
all_items = [(r[0], r[1]) for r in conn.execute('SELECT id, name FROM items ORDER BY id')]
conn.close()

server_items = [(i, n) for i, n in all_items if is_battle(n)]

# Load current mapping
content = open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js').read()
data = json.loads(content[content.index('{"url"'):content.rindex('}') + 1])
mapping = data['mapping']

# Find missing
missing = []
for item_id, name in server_items:
    sk = norm(name)
    hk = name.lower().replace(' ', '-').replace("'", '').replace('.', '')
    if sk not in mapping and hk not in mapping:
        missing.append((item_id, name))

print(f'Server items: {len(server_items)} | In sheet: {len(server_items) - len(missing)} | Missing: {len(missing)}')

if not missing:
    print('All server items have sprites!')
else:
    # Add missing to sheet
    sheet = Image.open('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png')
    n_items = len(set(v for v in mapping.values()))
    rows = sheet.size[1] // SIZE
    try:
        font = ImageFont.truetype('arial.ttf', 9)
    except:
        font = ImageFont.load_default()

    for item_id, name in missing:
        sk, hk = norm(name), name.lower().replace(' ', '-').replace("'", '').replace('.', '')
        if sk in mapping or hk in mapping:
            continue
        col, row = n_items % COLS, n_items // COLS
        if row >= rows:
            ns = Image.new('RGBA', (COLS * SIZE, (row + 1) * SIZE), (0, 0, 0, 0))
            ns.paste(sheet, (0, 0))
            sheet = ns
            rows = row + 1
        x, y = col * SIZE, row * SIZE
        c = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
        d = ImageDraw.Draw(c)
        color = (100, 180, 100, 255) if 'berry' in name.lower() else (100, 160, 200, 255)
        d.rounded_rectangle([1, 1, SIZE - 2, SIZE - 2], radius=3, fill=color)
        lt = name.strip()[0].upper() if name else '?'
        bb = d.textbbox((0, 0), lt, font=font)
        d.text(((SIZE - bb[2] + bb[0]) // 2, (SIZE - bb[3] + bb[1]) // 2), lt, fill=(255, 255, 255, 255), font=font)
        sheet.paste(c, (x, y), c)
        mapping[sk] = f'-{x}px -{y}px'
        if hk != sk:
            mapping[hk] = f'-{x}px -{y}px'
        n_items += 1

    sheet.save('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png', 'PNG', optimize=True)

data['mapping'] = mapping
with open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js', 'w') as f:
    f.write('export const ITEM_SHEET = ' + json.dumps(data, indent=None) + ';\n')

kb = os.path.getsize('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png') // 1024
print(f'Sheet: {len(mapping)} keys, {kb}KB')

if missing:
    with open('E:/PokemonSImulator/smogon_stats/missing_items.txt', 'w', encoding='utf-8') as f:
        for i, n in missing:
            f.write(f'{i:4d} | {n}\n')
    print(f'Saved: smogon_stats/missing_items.txt')

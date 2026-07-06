"""Download item sprites from pokesprite CDN and rebuild items-sheet.png"""
from PIL import Image, ImageDraw, ImageFont
import urllib.request, io, os, json, re, sqlite3

SIZE, COLS = 24, 20
PS_BASE = 'https://cdn.jsdelivr.net/npm/pokesprite-images@2.7.0/items-outline'

PS_DIRS = {
    'berry': 'cheri chesto pecha rawst aspear leppa oran persim lum sitrus figy wiki mago aguav iapapa razz bluk nanab wepear pinap pomeg kelpsy qualot hondew grepa tamato cornn magost rabuta nomel spelon pamtre durin belue occa passho wacan rindo yache chople kebia shuca coba payapa tanga charti kasib haban colbur babiri chilan liechi ganlon salac petaya apicot lansat starf enigma micle custap jaboca rowap roseli kee maranga'.split(),
    'hold-item': 'leftovers shell-bell big-root black-sludge choice-band choice-scarf choice-specs expert-belt focus-band focus-sash life-orb metronome muscle-band wise-glasses wide-lens zoom-lens scope-lens quick-claw kings-rock razor-claw razor-fang bright-powder white-herb mental-herb power-herb toxic-orb flame-orb light-clay damp-rock heat-rock smooth-rock icy-rock grip-claw light-ball iron-ball rocky-helmet air-balloon binding-band destiny-knot float-stone eviolite assault-vest weakness-policy safety-goggles heavy-duty-boots room-service utility-umbrella eject-button eject-pack blunder-policy throat-spray red-card protective-pads terrain-extender adrenaline-orb shed-shell smoke-ball lagging-tail sticky-barb absorb-bulb cell-battery luminous-moss snowball ring-target black-glasses charcoal dragon-fang hard-stone magnet metal-coat miracle-seed mystic-water never-melt-ice poison-barb sharp-beak silk-scarf silver-powder soft-sand spell-tag twisted-spoon black-belt sea-incense lax-incense odd-incense rock-incense full-incense wave-incense rose-incense ability-shield clear-amulet mirror-herb punching-glove covert-cloak loaded-dice booster-energy'.split(),
    'evo-item': 'fire-stone water-stone thunder-stone leaf-stone moon-stone sun-stone shiny-stone dusk-stone dawn-stone ice-stone oval-stone'.split(),
}

def download(url):
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.read()
    except:
        return None

def norm(n):
    return re.sub(r'[^a-z0-9]', '', n.lower())

# Load sprites
sprites = {}
print('Downloading from pokesprite...')
for subdir, names in PS_DIRS.items():
    for name in names:
        data = download(f'{PS_BASE}/{subdir}/{name}.png')
        if data:
            sprites[name] = Image.open(io.BytesIO(data)).convert('RGBA')
print(f'  {len(sprites)} sprites loaded')

# Battle items
conn = sqlite3.connect('E:/PokemonSImulator/data/pokemon.db')
all_items = [(r[0], r[1]) for r in conn.execute('SELECT id, name FROM items ORDER BY id')]
conn.close()

SKIP = ['potion','ether','elixir','revive','repel','tm','shard','nugget','pearl','shoal',
    'rare-candy','rare bone','big-mushroom','balm-mushroom','tiny-mushroom','stardust',
    'star-piece','comet-shard','pp-up','pp-max','heart-scale','honey',
    'growth-','stable-','gooey-','damp-','heat-','smooth-','icy-',
    'stone','-stone','mail','fossil','-fossil','mega','-ite','ium-z','z-crystal',
    'tr00','tr01','tr02','tr03','tr04','tr05','tr06','tr07','tr08','tr09',
    'tr10','tr11','tr12','tr13','tr14','tr15','tr16','tr17','tr18','tr19',
    'tr20','tr30','tr40','tr50','tr60','tr70','tr80','tr90',
    '-tm','hm','data-card','coupon','key','ticket','pass','letter','parcel','scope',
    'bike','rod','flute','case','pouch','kit','sack','album','goods','chip',
    'capsule','card','candy','mushroom','bone','pearl','nugget','stardust','comet',
    'vitamin','protein','iron','calcium','zinc','carbos','hp-up','exp-share',
    'mulch','apricorn','doll','toy','contest',
    'sweet-apple','tart-apple','syrupy-apple','cracked-pot','chipped-pot',
    'whipped-dream','sachet','galarica','malicious-armor','auspicious-armor',
    'teacup','metal-alloy','deep-sea-tooth','deep-sea-scale',
    'electirizer','magmarizer','protector','reaper-cloth',
    'up-grade','dubious-disc','prism-scale','dragon-scale',
    'kings-rock','metal-coat','bottle-cap','gold-bottle-cap',
    'macho-brace','power-bracer','power-belt','power-lens',
    'power-band','power-anklet','power-weight','-drive','memory',
    'poke-doll','poke-toy','fluffy-tail','old-gateau','lava-cookie',
    'fresh-water','soda-pop','lemonade','moomoo-milk','energy-root',
    'heal-powder','revival-herb','sacred-ash','full-restore','full-heal',
    'antidote','burn-heal','ice-heal','awakening','escape-rope',
    'berry-juice','sweet-heart','shoal-salt','shoal-shell',
    'red-orb','blue-orb','jade-orb','clear-bell','tidal-bell',
    'blue-card','red-scale','lucky-punch','lucky-egg','amulet-coin',
    'silver-wing','rainbow-wing','soothe-bell','cleanse-tag',
    'vile-vial','crucibellite','fairy-feather']

WHITELIST = ['choice scarf','red card','safety goggles','snowball','scope lens',
             'silver powder','silk scarf','iron ball','light ball','smoke ball',
             'big root','black sludge']

def is_battle(name):
    nl = name.lower().replace(' ','-')
    if name.lower().strip() in WHITELIST: return True
    for kw in SKIP:
        if kw in nl: return False
    return True

battle = [(i,n) for i,n in all_items if is_battle(n)]

# Match
matched, missing = [], []
for item_id, name in battle:
    names = [name.lower().replace(' ','-').replace("'",'').replace('.','')]
    if 'berry' in name.lower() and name.lower() != 'berry juice':
        names.insert(0, name.lower().replace(' berry','').replace(' ','-'))
    names.append(name.lower().replace(' ','').replace("'",'').replace('.',''))

    found = None
    for k in names:
        if k in sprites:
            found = sprites[k]; break
    if found:
        matched.append((item_id, name, found))
    else:
        c = Image.new('RGBA', (SIZE,SIZE), (0,0,0,0))
        d = ImageDraw.Draw(c)
        d.rounded_rectangle([1,1,SIZE-2,SIZE-2], radius=3, fill=(150,140,190,200))
        try: font = ImageFont.truetype('arial.ttf',10)
        except: font = ImageFont.load_default()
        lt = name.strip()[0].upper()
        bb = d.textbbox((0,0), lt, font=font)
        d.text(((SIZE-bb[2]+bb[0])//2,(SIZE-bb[3]+bb[1])//2), lt, fill=(255,255,255,255), font=font)
        matched.append((item_id, name, c))
        missing.append((item_id, name))

# Build sheet
rows = (len(matched) + COLS - 1) // COLS
sheet = Image.new('RGBA', (COLS*SIZE, rows*SIZE), (0,0,0,0))
mapping = {}
for idx, (item_id, name, img) in enumerate(matched):
    col, row = idx % COLS, idx // COLS
    x, y = col*SIZE, row*SIZE
    img = img.resize((SIZE,SIZE), Image.LANCZOS)
    sheet.paste(img, (x,y), img)
    sk, hk = norm(name), name.lower().replace(' ','-').replace("'",'').replace('.','')
    mapping[sk] = f'-{x}px -{y}px'
    if hk != sk: mapping[hk] = f'-{x}px -{y}px'

sheet.save('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png', 'PNG', optimize=True)
kb = os.path.getsize('E:/PokemonSImulator/frontend/public/sprites/items-sheet.png')//1024

data = {'url':'/sprites/items-sheet.png','size':SIZE,'cols':COLS,'mapping':mapping}
with open('E:/PokemonSImulator/frontend/src/utils/itemSheet.js','w') as f:
    f.write('export const ITEM_SHEET = '+json.dumps(data, indent=None)+';\n')

real = len(matched) - len(missing)
print(f'Sheet: {COLS*SIZE}x{rows*SIZE}px, {kb}KB')
print(f'Items: {len(matched)} ({real} real sprites, {len(missing)} placeholders)')
if missing:
    print(f'Missing: {len(missing)}')
    for i,n in missing[:10]: print(f'  {i:4d} | {n}')
    if len(missing) > 10: print(f'  ... +{len(missing)-10}')

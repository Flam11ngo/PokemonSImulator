#!/usr/bin/env python3
"""Build item icon spritesheet: 20 per row, single PNG + mapping."""
import urllib.request, json, os, time
from pathlib import Path
from PIL import Image

PROJECT = Path(__file__).resolve().parent.parent
ITEMS_DIR = PROJECT / "frontend/public/sprites/items"
OUT_DIR = PROJECT / "frontend/public/sprites"
COLS = 20
ICON_SIZE = 24

def download_item(name):
    p = ITEMS_DIR / f"{name}.png"
    if p.exists() and p.stat().st_size > 100:
        return p
    url = f"https://play.pokemonshowdown.com/sprites/itemicons/{name}.png"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=5) as r:
            if r.status == 200:
                p.write_bytes(r.read())
                return p
    except: pass
    return None

def main():
    print(f"Output: {OUT_DIR}/items-sheet.png")

    with open(PROJECT / "data/items.json") as f:
        items = json.load(f).get("items", [])

    # Filter battle items
    skip_kw = ['-ball','potion','ether','elixir','revive','repel','tm','-mail',
               'shard','stone','plate','incense','mulch','nugget','pearl','shoal',
               'rare-candy','rare bone','big-mushroom','balm-mushroom',
               'tiny-mushroom','stardust','star-piece','comet-shard',
               'rm-','pp-up','pp-max','heart-scale','honey',
               'growth-','stable-','gooey-','damp-','heat-','smooth-','icy-']
    battle_items = []
    for it in items:
        name = it["name"].lower()
        if any(kw in name for kw in skip_kw): continue
        battle_items.append(it)
    print(f"Battle items: {len(battle_items)}/{len(items)}")

    total = len(battle_items)
    rows = (total + COLS - 1) // COLS
    sheet_w = COLS * ICON_SIZE
    sheet_h = rows * ICON_SIZE
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    mapping = {}
    ok = miss = 0

    for idx, it in enumerate(battle_items):
        name = it["name"]
        col = idx % COLS; row = idx // COLS
        x, y = col * ICON_SIZE, row * ICON_SIZE

        icon = download_item(name)
        if icon:
            try:
                img = Image.open(icon).convert("RGBA")
                img = img.resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS)
                sheet.paste(img, (x, y), img)
                mapping[name] = f"-{x}px -{y}px"
                ok += 1
            except: miss += 1
        else: miss += 1

        if (idx+1) % 200 == 0: print(f"  {idx+1}/{total} ({ok} ok, {miss} miss)")

    sheet.save(OUT_DIR / "items-sheet.png", "PNG", optimize=True)
    sz = (OUT_DIR / "items-sheet.png").stat().st_size

    js = "export const ITEM_SHEET = " + json.dumps({
        "url": "/sprites/items-sheet.png", "size": ICON_SIZE, "cols": COLS, "mapping": mapping
    }, separators=(',',':')) + ";\n"
    (PROJECT / "frontend/src/utils/itemSheet.js").write_text(js)

    print(f"\n✅ Done! {sz/1024:.0f} KB, {ok} placed, {miss} missing")
    print(f"   Sheet: {OUT_DIR}/items-sheet.png ({sheet_w}x{sheet_h})")
    print(f"   Map: frontend/src/utils/itemSheet.js")

if __name__ == "__main__":
    main()

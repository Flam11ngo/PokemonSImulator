#!/usr/bin/env python3
"""Build Pokemon icon spritesheet: 20 icons per row, single PNG + CSS mapping."""
import urllib.request, json, os, time
from pathlib import Path
from PIL import Image

PROJECT = Path(__file__).resolve().parent.parent
ICONS_DIR = PROJECT / "frontend/public/sprites/icons"
OUT_DIR = PROJECT / "frontend/public/sprites"
COLS = 20
ICON_SIZE = 40  # output icon size in spritesheet

def download_icon(sid):
    """Download from Showdown CDN, save as temp PNG."""
    p = ICONS_DIR / f"{sid}.png"
    if p.exists() and p.stat().st_size > 100:  # valid existing file
        return p
    url = f"https://play.pokemonshowdown.com/sprites/gen5icons/{sid}.png"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=5) as r:
            if r.status == 200:
                p.write_bytes(r.read())
                return p
    except: pass
    return None

def main():
    print(f"Output: {OUT_DIR}/icons-sheet.png")
    print(f"Grid: {COLS} per row, {ICON_SIZE}px each")

    # Load species to get total count
    with open(PROJECT / "data/species.json") as f:
        species = json.load(f).get("species", [])
    total = len(species)
    rows = (total + COLS - 1) // COLS

    # Create blank canvas
    sheet_w = COLS * ICON_SIZE
    sheet_h = rows * ICON_SIZE
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    mapping = {}  # species_id → {x, y}
    downloaded = 0
    missing = 0

    for idx, sp in enumerate(species):
        sid = sp["id"]
        col = idx % COLS
        row = idx // COLS
        x, y = col * ICON_SIZE, row * ICON_SIZE

        icon_path = download_icon(sid)
        if icon_path:
            try:
                img = Image.open(icon_path).convert("RGBA")
                # Resize to ICON_SIZE
                img = img.resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS)
                sheet.paste(img, (x, y), img)
                mapping[sid] = f"-{x}px -{y}px"
                downloaded += 1
            except: missing += 1
        else:
            missing += 1

        if (idx + 1) % 200 == 0:
            print(f"  {idx+1}/{total} processed ({downloaded} ok, {missing} miss)")

    # Save spritesheet
    sheet.save(OUT_DIR / "icons-sheet.png", "PNG", optimize=True)
    sheet_size = (OUT_DIR / "icons-sheet.png").stat().st_size

    # Save CSS/JS mapping
    mapping_js = "export const ICON_SHEET = " + json.dumps({
        "url": "/sprites/icons-sheet.png",
        "size": ICON_SIZE,
        "cols": COLS,
        "mapping": mapping,
    }, separators=(',', ':')) + ";\n"
    (PROJECT / "frontend/src/utils/iconSheet.js").write_text(mapping_js)

    print(f"\n✅ Done!")
    print(f"   Spritesheet: {OUT_DIR}/icons-sheet.png ({sheet_size/1024:.0f} KB)")
    print(f"   Resolution: {sheet_w}x{sheet_h}")
    print(f"   Icons: {downloaded} placed, {missing} missing")
    print(f"   Mapping: frontend/src/utils/iconSheet.js")

if __name__ == "__main__":
    main()

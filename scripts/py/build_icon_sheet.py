#!/usr/bin/env python3
"""Build Pokemon icon spritesheet from local GIFs (first frame)."""
import json
from pathlib import Path
from PIL import Image

PROJECT = Path(__file__).resolve().parent.parent
GIF_DIR = PROJECT / "frontend/public/sprites"
OUT = PROJECT / "frontend/public/sprites/icons-sheet.png"
CELL = 36   # cell size with padding
ICON = 32   # icon size inside cell
PAD = (CELL - ICON) // 2
COLS = 16

def main():
    with open(PROJECT / "data/species.json") as f:
        species = json.load(f).get("species", [])
    total = len(species)
    rows = (total + COLS - 1) // COLS
    w, h = COLS * CELL, rows * CELL

    print(f"Building: {COLS}×{rows}, cell={CELL}px icon={ICON}px pad={PAD}px → {w}×{h}")
    sheet = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ok = miss = 0

    for sp in species:
        sid = sp["id"]
        col, row = sid % COLS, sid // COLS
        cx, cy = col * CELL + PAD, row * CELL + PAD

        gif = GIF_DIR / f"{sid}.gif"
        if gif.exists():
            try:
                img = Image.open(gif); img.seek(0)
                frame = img.convert("RGBA").resize((ICON, ICON), Image.LANCZOS)
                sheet.paste(frame, (cx, cy), frame)
                ok += 1; img.close()
            except: miss += 1
        else: miss += 1

        if sid % 200 == 0: print(f"  {sid}/{total} ({ok} ok, {miss} miss)")

    sheet.save(OUT, "PNG", optimize=True)
    print(f"\n✅ {OUT} ({OUT.stat().st_size/1024:.0f}KB)")
    print(f"   {ok} placed, {miss} missing, grid {COLS}×{rows}")

    # Write JS mapping
    js = f"export const ICON_SHEET = {{ url:'/sprites/icons-sheet.png', cell:{CELL}, icon:{ICON}, pad:{PAD}, cols:{COLS} }};\n"
    (PROJECT / "frontend/src/utils/iconSheet.js").write_text(js)
    print(f"   Mapping: frontend/src/utils/iconSheet.js")

if __name__ == "__main__":
    main()

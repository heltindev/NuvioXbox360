#!/usr/bin/env python3
"""Generate Xbox 360-friendly antialiased font and icon assets.

Run on a PC with Pillow. The generated header is included in the .xex,
so the player does not need fonts or icons installed on the Xbox.
The drawings are independently authored; no Nuvio code/assets are copied.
"""

from pathlib import Path
import argparse
from PIL import Image, ImageDraw, ImageFont


W = H = 512
CELL_W = 40
CELL_H = 48
ICON_Y = 388
ICON_CELL = 64


def make_icons(image):
    # Render vectors at 4x and downsample for antialiasing.
    scale = 4
    canvas = Image.new("L", (ICON_CELL * 5 * scale, ICON_CELL * scale))
    draw = ImageDraw.Draw(canvas)

    def xy(index, x, y):
        return ((index * ICON_CELL + x) * scale, y * scale)

    def line(index, coords, width=3, joint="curve"):
        draw.line([xy(index, x, y) for x, y in coords], fill=255, width=width * scale, joint=joint)

    def oval(index, x0, y0, x1, y1, width=3):
        draw.ellipse((*xy(index, x0, y0), *xy(index, x1, y1)), outline=255, width=width * scale)

    # Home: roof + facade + door.
    line(0, [(15, 31), (32, 17), (49, 31)])
    line(0, [(20, 28), (20, 48), (44, 48), (44, 28)])
    line(0, [(29, 48), (29, 36), (36, 36), (36, 48)])

    # Discover/compass.
    oval(1, 16, 16, 48, 48)
    line(1, [(39, 25), (34, 36), (25, 40), (30, 29), (39, 25)])

    # Magnifying glass.
    oval(2, 17, 16, 41, 40)
    line(2, [(39, 39), (49, 49)], width=4)

    # Library collection icon.
    line(3, [(19, 20), (43, 20), (43, 47), (19, 47), (19, 20)])
    line(3, [(24, 16), (47, 16), (47, 43)])
    line(3, [(25, 29), (36, 29)])

    # Settings gear and hub.
    oval(4, 24, 24, 40, 40)
    for x0, y0, x1, y1 in [
        (32, 13, 32, 21), (32, 43, 32, 51), (13, 32, 21, 32),
        (43, 32, 51, 32), (19, 19, 24, 24), (40, 40, 45, 45),
        (19, 45, 24, 40), (40, 24, 45, 19)
    ]:
        line(4, [(x0, y0), (x1, y1)], width=4)

    canvas = canvas.resize((ICON_CELL * 5, ICON_CELL), Image.Resampling.LANCZOS)
    image.paste(canvas, (0, ICON_Y))


def rle_pack(raw):
    packed = bytearray()
    start = 0
    while start < len(raw):
        color = raw[start]
        end = start + 1
        while end < len(raw) and raw[end] == color and end - start < 255:
            end += 1
        packed.extend((end - start, color))
        start = end
    return bytes(packed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", type=Path, default=Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    parser.add_argument("--output", type=Path, default=Path("VisualStudio2010/src/NuvioUiAtlas.h"))
    parser.add_argument("--preview", type=Path)
    args = parser.parse_args()
    font = ImageFont.truetype(str(args.font), 31)
    bitmap = Image.new("L", (W, H))
    draw = ImageDraw.Draw(bitmap)
    advances = []
    for code in range(32, 128):
        index = code - 32
        left = (index % 12) * CELL_W
        top = (index // 12) * CELL_H
        char = chr(code)
        draw.text((left + 2, top + 4), char, font=font, anchor="la", fill=255)
        advances.append(min(CELL_W, max(9, round(font.getlength(char) + 1))))
    make_icons(bitmap)
    raw = bitmap.tobytes()
    packed = rle_pack(raw)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    for index in range(0, len(packed), 24):
        rows.append("    " + ",".join(str(x) for x in packed[index:index + 24]) + ",")
    glyphs = ["    " + ",".join(str(x) for x in advances[i:i + 16]) + "," for i in range(0, len(advances), 16)]
    content = (
        "// Generated antialiased bitmap font and icons, do not edit manually.\n"
        "// Recreate with: python tools/generate_ui_atlas.py\n"
        "#ifndef NUVIO_UI_ATLAS_H\n#define NUVIO_UI_ATLAS_H\n"
        "enum { NUVIO_ATLAS_WIDTH = 512, NUVIO_ATLAS_HEIGHT = 512, "
        "NUVIO_FONT_CELL_W = 40, NUVIO_FONT_CELL_H = 48, "
        "NUVIO_ICON_TOP = 388 };\n"
        "static const unsigned char kNuvioGlyphAdvance[96] = {\n" + "\n".join(glyphs) + "\n};\n"
        "static const unsigned char kNuvioUiAtlasRle[] = {\n" + "\n".join(rows) + "\n};\n"
        "#endif\n"
    )
    args.output.write_text(content, encoding="ascii")
    if args.preview:
        bitmap.save(args.preview)
    print("Atlas: {} bytes raw, {} bytes RLE, output {}".format(len(raw), len(packed), args.output))


if __name__ == "__main__":
    main()

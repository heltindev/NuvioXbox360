#!/usr/bin/env python3
"""Generate Xbox 360-friendly antialiased font and icon assets.

Run on a PC with Pillow. The generated header is included in the .xex,
so the player does not need fonts or icons installed on the Xbox.
The drawings are independently authored; no Nuvio code/assets are copied.
"""

from pathlib import Path
import argparse
import math
import re
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

    # Continuous eight-tooth cog silhouette with a circular hub opening.
    points = []
    for tooth in range(8):
        for offset, radius in [(-.5, 19), (-.3, 19), (-.3, 25), (.3, 25), (.3, 19), (.5, 19)]:
            angle = (tooth + offset) * math.pi / 4
            points.append(xy(4, 32 + math.cos(angle) * radius, 32 + math.sin(angle) * radius))
    draw.polygon(points, fill=255)
    draw.ellipse((*xy(4, 23, 23), *xy(4, 41, 41)), fill=0)

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
    parser.add_argument("--source-atlas", type=Path, help="Keep the existing font bitmap and regenerate icons/metrics")
    args = parser.parse_args()
    bitmap = Image.new("L", (W, H))
    draw = ImageDraw.Draw(bitmap)
    advances = []
    if args.source_atlas:
        source = args.source_atlas.read_text(encoding="ascii")
        advances = list(map(int, re.search(r'kNuvioGlyphAdvance\[96\] = \{(.*?)\};', source, re.S)[1].replace('\n', '').strip(' ,').split(',')))
        packed = list(map(int, re.search(r'kNuvioUiAtlasRle\[\] = \{(.*?)\};', source, re.S)[1].replace('\n', '').strip(' ,').split(',')))
        raw = bytearray()
        for i in range(0, len(packed), 2):
            raw.extend(bytes([packed[i + 1]]) * packed[i])
        assert len(raw) == W * H and len(advances) == 96
        bitmap = Image.frombytes("L", (W, H), bytes(raw))
        ImageDraw.Draw(bitmap).rectangle((0, ICON_Y, W, ICON_Y + ICON_CELL), fill=0)
    else:
        font = ImageFont.truetype(str(args.font), 31)
        for code in range(32, 128):
            index = code - 32
            left = (index % 12) * CELL_W
            top = (index // 12) * CELL_H
            char = chr(code)
            draw.text((left + 2, top + 4), char, font=font, anchor="la", fill=255)
            advances.append(min(CELL_W, max(9, round(font.getlength(char) + 1))))
    bounds = []
    for index in range(96):
        left, top = (index % 12) * CELL_W, (index // 12) * CELL_H
        cell = bitmap.crop((left, top, left + CELL_W, top + CELL_H))
        bounds.append(cell.getbbox() or (0, 0, 0, 0))
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
        "static const unsigned char kNuvioGlyphInk[96][4] = {\n" +
        "\n".join("    {" + ",".join(map(str, box)) + "}," for box in bounds) + "\n};\n"
        "static const unsigned char kNuvioUiAtlasRle[] = {\n" + "\n".join(rows) + "\n};\n"
        "#endif\n"
    )
    args.output.write_text(content, encoding="ascii")
    if args.preview:
        bitmap.save(args.preview)
    print("Atlas: {} bytes raw, {} bytes RLE, output {}".format(len(raw), len(packed), args.output))


if __name__ == "__main__":
    main()

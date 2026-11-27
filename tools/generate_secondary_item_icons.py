from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
SIZE = 48
SCALE = 4
PALETTES = {
    "din_fire": [
        (0, 0, 0, 0), (45, 10, 12, 255), (112, 19, 20, 255),
        (181, 33, 24, 255), (227, 66, 23, 255), (250, 112, 22, 255),
        (255, 164, 32, 255), (255, 207, 72, 255), (255, 237, 154, 255),
        (255, 255, 224, 255), (128, 28, 22, 160), (213, 59, 29, 192),
        (255, 140, 35, 192), (248, 188, 63, 224), (255, 220, 121, 160),
        (255, 255, 255, 255),
    ],
    "nayru_love": [
        (0, 0, 0, 0), (8, 15, 51, 255), (13, 31, 86, 255),
        (20, 54, 130, 255), (24, 78, 180, 255), (33, 115, 223, 255),
        (37, 155, 241, 255), (75, 190, 255, 255), (116, 214, 255, 255),
        (169, 235, 255, 255), (221, 249, 255, 255), (255, 255, 255, 255),
        (13, 57, 150, 128), (47, 126, 185, 160), (139, 206, 255, 192),
        (220, 249, 255, 128),
    ],
    "giant_mask": [
        (0, 0, 0, 0), (22, 15, 42, 255), (47, 27, 78, 255),
        (76, 39, 120, 255), (111, 54, 161, 255), (153, 77, 194, 255),
        (195, 115, 220, 255), (229, 174, 237, 255), (255, 215, 91, 255),
        (255, 239, 157, 255), (16, 23, 42, 255), (58, 158, 177, 255),
        (104, 210, 216, 255), (255, 255, 238, 255), (125, 62, 156, 192),
        (255, 255, 255, 255),
    ],
}


def color_distance(left, right):
    return sum((left[index] - right[index]) ** 2 for index in range(4))


def encode_rgb5a3(color):
    red, green, blue, alpha = color
    if alpha >= 224:
        value = 0x8000
        value |= round(red * 31 / 255) << 10
        value |= round(green * 31 / 255) << 5
        value |= round(blue * 31 / 255)
    else:
        value = round(alpha * 7 / 255) << 12
        value |= round(red * 15 / 255) << 8
        value |= round(green * 15 / 255) << 4
        value |= round(blue * 15 / 255)
    return value.to_bytes(2, "big")


def draw_icon(name):
    canvas = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)

    def points(coords):
        return [(x * SCALE, y * SCALE) for x, y in coords]

    def ellipse(coords, fill, outline=None, width=1):
        draw.ellipse(
            tuple(value * SCALE for value in coords),
            fill=fill,
            outline=outline,
            width=width * SCALE,
        )

    if name == "din_fire":
        draw.polygon(
            points([(24, 2), (36, 16), (39, 9), (44, 23), (42, 34),
                    (35, 43), (24, 47), (13, 43), (6, 34), (4, 23),
                    (9, 12), (13, 20), (17, 8)]),
            fill=(112, 19, 20, 255),
        )
        draw.polygon(
            points([(24, 8), (32, 20), (36, 16), (38, 27), (34, 37),
                    (24, 43), (14, 37), (10, 28), (14, 22), (18, 27)]),
            fill=(250, 112, 22, 255),
        )
        draw.polygon(
            points([(24, 17), (30, 27), (28, 35), (24, 40), (18, 35),
                    (18, 29)]),
            fill=(255, 207, 72, 255),
        )
        draw.polygon(
            points([(24, 24), (28, 31), (24, 36), (20, 31)]),
            fill=(255, 255, 224, 255),
        )
        ellipse((20, 2, 28, 10), (255, 237, 154, 255))
        draw.polygon(
            points([(8, 34), (12, 38), (8, 40)]),
            fill=(255, 164, 32, 255),
        )
        draw.polygon(
            points([(40, 34), (36, 38), (40, 40)]),
            fill=(255, 164, 32, 255),
        )
    elif name == "nayru_love":
        outer = [(24, 2), (44, 24), (24, 46), (4, 24)]
        inner = [(24, 7), (39, 24), (24, 41), (9, 24)]
        draw.polygon(points(outer), fill=(13, 57, 150, 255))
        draw.line(points(outer + [outer[0]]), fill=(116, 214, 255, 255),
                  width=3 * SCALE, joint="curve")
        draw.polygon(points(inner), fill=(24, 78, 180, 255))
        draw.line(points(inner + [inner[0]]), fill=(169, 235, 255, 255),
                  width=2 * SCALE, joint="curve")
        draw.polygon(points([(24, 10), (36, 24), (24, 24)]),
                     fill=(37, 155, 241, 255))
        draw.polygon(points([(12, 24), (24, 24), (24, 39)]),
                     fill=(33, 115, 223, 255))
        draw.polygon(points([(24, 24), (36, 24), (24, 39)]),
                     fill=(116, 214, 255, 255))
        draw.polygon(points([(24, 15), (30, 24), (24, 33), (18, 24)]),
                     fill=(255, 255, 255, 255))
        for x, y in ((24, 2), (4, 24), (44, 24), (24, 46)):
            ellipse((x - 1, y - 1, x + 1, y + 1), (255, 255, 255, 255))
    else:
        draw.polygon(
            points([(9, 17), (3, 8), (7, 5), (17, 13), (31, 13),
                    (41, 5), (45, 8), (39, 18), (39, 30), (33, 42),
                    (24, 47), (15, 42), (9, 30)]),
            fill=(22, 15, 42, 255),
        )
        draw.polygon(
            points([(10, 18), (18, 15), (30, 15), (38, 18), (36, 30),
                    (31, 39), (24, 43), (17, 39), (12, 30)]),
            fill=(111, 54, 161, 255),
        )
        draw.polygon(
            points([(9, 8), (16, 15), (12, 19), (6, 11)]),
            fill=(255, 215, 91, 255),
        )
        draw.polygon(
            points([(39, 8), (32, 15), (36, 19), (42, 11)]),
            fill=(255, 215, 91, 255),
        )
        draw.polygon(
            points([(11, 20), (21, 22), (18, 27), (12, 26)]),
            fill=(16, 23, 42, 255),
        )
        draw.polygon(
            points([(37, 20), (27, 22), (30, 27), (36, 26)]),
            fill=(16, 23, 42, 255),
        )
        draw.line(points([(18, 33), (24, 36), (30, 33)]),
                  fill=(255, 239, 157, 255), width=2 * SCALE)
        ellipse((21, 7, 27, 13), (255, 215, 91, 255))

    return canvas.resize((SIZE, SIZE), Image.Resampling.LANCZOS)


def encode_ci4(image, palette):
    rgba = image.load()
    indices = []
    for y in range(SIZE):
        for x in range(SIZE):
            color = rgba[x, y]
            if color[3] < 24:
                indices.append(0)
            else:
                indices.append(min(
                    range(1, len(palette)),
                    key=lambda index: color_distance(color, palette[index]),
                ))

    tiled = bytearray()
    for tile_y in range(0, SIZE, 8):
        for tile_x in range(0, SIZE, 8):
            for y in range(tile_y, tile_y + 8):
                for x in range(tile_x, tile_x + 8, 2):
                    left = indices[y * SIZE + x]
                    right = indices[y * SIZE + x + 1]
                    tiled.append((left << 4) | right)
    return bytes(tiled)


def main():
    template = (ROOT / "res" / "ice_arrow.bti").read_bytes()
    if len(template) != 1216 or template[0] != 8:
        raise ValueError("Ice Arrow icon is not the expected 48x48 CI4 BTI template")

    for name, palette in PALETTES.items():
        palette_bytes = b"".join(encode_rgb5a3(color) for color in palette)
        image_bytes = encode_ci4(draw_icon(name), palette)
        output = template[:32] + palette_bytes + image_bytes
        if len(output) != len(template):
            raise ValueError(f"{name} icon does not match the BTI template size")
        (ROOT / "res" / f"{name}.bti").write_bytes(output)


if __name__ == "__main__":
    main()

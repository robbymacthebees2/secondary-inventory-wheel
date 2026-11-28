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
    "hawk_grass": [
        (0, 0, 0, 0), (17, 45, 24, 255), (28, 72, 32, 255),
        (43, 105, 39, 255), (64, 142, 47, 255), (97, 174, 57, 255),
        (145, 198, 71, 255), (204, 220, 112, 255), (89, 55, 27, 255),
        (139, 86, 35, 255), (181, 133, 53, 255), (236, 186, 57, 255),
        (255, 222, 104, 255), (233, 239, 175, 255), (30, 89, 45, 192),
        (255, 255, 255, 255),
    ],
    "shield_surf": [
        (0, 0, 0, 0), (17, 29, 48, 255), (35, 55, 78, 255),
        (69, 92, 113, 255), (133, 157, 169, 255), (205, 217, 215, 255),
        (242, 231, 184, 255), (174, 119, 49, 255), (31, 94, 135, 255),
        (38, 139, 184, 255), (68, 183, 217, 255), (149, 224, 234, 255),
        (231, 249, 242, 255), (18, 60, 99, 192), (82, 147, 168, 160),
        (255, 255, 255, 255),
    ],
    "goron_tunic": [
        (0, 0, 0, 0), (42, 15, 20, 255), (79, 24, 26, 255),
        (126, 28, 27, 255), (171, 37, 32, 255), (210, 54, 41, 255),
        (235, 83, 55, 255), (255, 124, 76, 255), (96, 58, 27, 255),
        (150, 100, 42, 255), (204, 155, 61, 255), (241, 199, 98, 255),
        (255, 228, 151, 255), (18, 31, 43, 255), (124, 47, 40, 192),
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
    elif name == "giant_mask":
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
    elif name == "hawk_grass":
                  draw.line(points([(24, 45), (24, 17)]), fill=(28, 72, 32, 255),
                            width=3 * SCALE)
                  draw.polygon(
                      points([(23, 39), (16, 35), (8, 24), (10, 20), (19, 26),
                              (23, 31)]),
                      fill=(43, 105, 39, 255),
                  )
                  draw.polygon(
                      points([(25, 39), (32, 35), (40, 24), (38, 20), (29, 26),
                              (25, 31)]),
                      fill=(64, 142, 47, 255),
                  )
                  draw.polygon(
                      points([(22, 31), (15, 26), (11, 14), (14, 12), (21, 20),
                              (24, 27)]),
                      fill=(97, 174, 57, 255),
                  )
                  draw.polygon(
                      points([(26, 31), (33, 26), (37, 14), (34, 12), (27, 20),
                              (24, 27)]),
                      fill=(145, 198, 71, 255),
                  )
                  draw.polygon(
                      points([(23, 21), (18, 15), (20, 5), (23, 9), (24, 16)]),
                      fill=(64, 142, 47, 255),
                  )
                  draw.polygon(
                      points([(25, 21), (30, 15), (28, 5), (25, 9), (24, 16)]),
                      fill=(97, 174, 57, 255),
                  )
                  for start, end in (
                      ((10, 23), (20, 30)), ((38, 23), (28, 30)),
                      ((13, 15), (21, 24)), ((35, 15), (27, 24)),
                  ):
                      draw.line(points([start, end]), fill=(204, 220, 112, 255),
                                width=1 * SCALE)
                  ellipse((21, 11, 27, 17), (236, 186, 57, 255),
                          outline=(89, 55, 27, 255), width=1)
                  draw.line(points([(23, 17), (20, 21)]), fill=(181, 133, 53, 255),
                            width=1 * SCALE)
                  draw.line(points([(25, 17), (28, 21)]), fill=(181, 133, 53, 255),
                            width=1 * SCALE)
    elif name == "shield_surf":
                  draw.arc((0, 27 * SCALE, 48 * SCALE, 55 * SCALE), 195, 345,
                           fill=(31, 94, 135, 255), width=4 * SCALE)
                  draw.arc((4 * SCALE, 30 * SCALE, 44 * SCALE, 54 * SCALE), 195, 345,
                           fill=(68, 183, 217, 255), width=2 * SCALE)
                  outer = [(24, 3), (43, 10), (40, 28), (34, 37), (24, 44),
                           (14, 37), (8, 28), (5, 10)]
                  inner = [(24, 8), (37, 13), (35, 27), (30, 34), (24, 39),
                           (18, 34), (13, 27), (11, 13)]
                  draw.polygon(points(outer), fill=(17, 29, 48, 255))
                  draw.polygon(points(inner), fill=(205, 217, 215, 255))
                  draw.line(points(outer + [outer[0]]), fill=(174, 119, 49, 255),
                            width=2 * SCALE, joint="curve")
                  draw.polygon(
                      points([(24, 11), (32, 15), (31, 26), (24, 34),
                              (17, 26), (16, 15)]),
                      fill=(38, 139, 184, 255),
                  )
                  draw.polygon(
                      points([(24, 14), (28, 20), (24, 27), (20, 20)]),
                      fill=(231, 249, 242, 255),
                  )
                  draw.line(points([(15, 39), (22, 43), (30, 42), (36, 38)]),
                            fill=(68, 183, 217, 255), width=2 * SCALE)
    elif name == "goron_tunic":
                  draw.polygon(
                      points([(17, 8), (22, 4), (26, 4), (31, 8), (39, 10),
                              (46, 18), (40, 24), (36, 21), (37, 43), (11, 43),
                              (12, 21), (8, 24), (2, 18), (9, 10)]),
                      fill=(42, 15, 20, 255),
                  )
                  draw.polygon(
                      points([(18, 10), (22, 7), (26, 7), (30, 10), (37, 13),
                              (41, 18), (38, 20), (33, 17), (33, 39), (15, 39),
                              (15, 17), (10, 20), (7, 18), (11, 13)]),
                      fill=(171, 37, 32, 255),
                  )
                  draw.polygon(
                      points([(19, 9), (22, 7), (26, 7), (29, 9), (26, 15),
                              (24, 18), (22, 15)]),
                      fill=(241, 199, 98, 255),
                  )
                  draw.line(points([(15, 33), (33, 33)]), fill=(204, 155, 61, 255),
                            width=2 * SCALE)
                  draw.line(points([(18, 21), (18, 29)]), fill=(235, 83, 55, 255),
                            width=2 * SCALE)
                  draw.line(points([(30, 21), (30, 29)]), fill=(235, 83, 55, 255),
                            width=2 * SCALE)
                  draw.polygon(
                      points([(24, 20), (29, 25), (24, 30), (19, 25)]),
                      fill=(96, 58, 27, 255),
                  )
                  draw.polygon(
                      points([(24, 21), (27, 25), (24, 28), (21, 25)]),
                      fill=(241, 199, 98, 255),
                  )

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

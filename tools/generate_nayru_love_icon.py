from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
SCALE = 4
SIZE = 48
PALETTE = [
    (0, 0, 0, 0),
    (8, 15, 51, 255),
    (13, 31, 86, 255),
    (20, 54, 130, 255),
    (24, 78, 180, 255),
    (33, 115, 223, 255),
    (37, 155, 241, 255),
    (75, 190, 255, 255),
    (116, 214, 255, 255),
    (169, 235, 255, 255),
    (221, 249, 255, 255),
    (255, 255, 255, 255),
    (13, 57, 150, 128),
    (47, 126, 185, 160),
    (139, 206, 255, 192),
    (220, 249, 255, 128),
]


def color_distance(left, right):
    return sum((left[i] - right[i]) ** 2 for i in range(4))


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


def draw_icon():
    canvas = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)

    def points(coords):
        return [(x * SCALE, y * SCALE) for x, y in coords]

    outer = [(24, 2), (44, 24), (24, 46), (4, 24)]
    inner = [(24, 7), (39, 24), (24, 41), (9, 24)]
    core = [(24, 12), (34, 24), (24, 36), (14, 24)]
    draw.polygon(points(outer), fill=(13, 57, 150, 155))
    draw.line(points(outer + [outer[0]]), fill=(116, 214, 255, 255),
              width=3 * SCALE, joint="curve")
    draw.polygon(points(inner), fill=(24, 78, 180, 225))
    draw.line(points(inner + [inner[0]]), fill=(169, 235, 255, 255),
              width=2 * SCALE, joint="curve")
    draw.polygon(points([(24, 8), (24, 24), (12, 24)]),
                 fill=(75, 190, 255, 220))
    draw.polygon(points([(24, 8), (36, 24), (24, 24)]),
                 fill=(37, 155, 241, 220))
    draw.polygon(points([(12, 24), (24, 24), (24, 40)]),
                 fill=(33, 115, 223, 225))
    draw.polygon(points([(24, 24), (36, 24), (24, 40)]),
                 fill=(116, 214, 255, 235))
    draw.polygon(points(core), fill=(221, 249, 255, 255))
    draw.polygon(points([(24, 15), (30, 24), (24, 33), (18, 24)]),
                 fill=(75, 190, 255, 255))

    for x, y in ((24, 1), (2, 24), (45, 24), (24, 47)):
        draw.rectangle(
            ((x - 1) * SCALE, (y - 1) * SCALE,
             (x + 1) * SCALE, (y + 1) * SCALE),
            fill=(255, 255, 255, 255),
        )

    return canvas.resize((SIZE, SIZE), Image.Resampling.LANCZOS)


def encode_ci4(image):
    rgba = image.load()
    indices = []
    for y in range(SIZE):
        for x in range(SIZE):
            color = rgba[x, y]
            if color[3] < 24:
                indices.append(0)
            else:
                indices.append(min(
                    range(1, len(PALETTE)),
                    key=lambda index: color_distance(color, PALETTE[index]),
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
    template = (ROOT / "res" / "din_fire.bti").read_bytes()
    if len(template) < 64 or template[0] != 8:
        raise ValueError("Din's Fire icon is not the expected CI4 BTI template")

    image = draw_icon()
    palette_bytes = b"".join(encode_rgb5a3(color) for color in PALETTE)
    image_bytes = encode_ci4(image)
    output = template[:32] + palette_bytes + image_bytes
    if len(output) != len(template):
        raise ValueError("Generated BTI size does not match the template")

    (ROOT / "res" / "nayru_love.bti").write_bytes(output)


if __name__ == "__main__":
    main()

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
SCALE = 4
SIZE = 48
PALETTE = [
    (0, 0, 0, 0),
    (20, 17, 60, 255),
    (37, 31, 94, 255),
    (66, 52, 139, 255),
    (100, 72, 177, 255),
    (146, 98, 197, 255),
    (190, 133, 217, 255),
    (226, 173, 230, 255),
    (250, 212, 236, 255),
    (255, 239, 218, 255),
    (255, 255, 244, 255),
    (255, 255, 255, 255),
    (52, 156, 142, 255),
    (80, 207, 174, 255),
    (154, 236, 208, 255),
    (223, 255, 241, 255),
]


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


def draw_icon():
    canvas = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)

    def box(coords):
        return tuple(value * SCALE for value in coords)

    draw.ellipse(box((4, 4, 44, 44)), fill=(20, 17, 60, 255))
    draw.ellipse(box((7, 7, 41, 41)), fill=(66, 52, 139, 255))
    draw.ellipse(box((10, 10, 38, 38)), outline=(154, 236, 208, 255),
                 width=2 * SCALE)
    draw.arc(box((11, 11, 37, 37)), 205, 335, fill=(255, 239, 218, 255),
             width=3 * SCALE)
    draw.arc(box((13, 13, 35, 35)), 25, 155, fill=(190, 133, 217, 255),
             width=2 * SCALE)

    draw.polygon(
        [(24 * SCALE, 35 * SCALE), (14 * SCALE, 26 * SCALE),
         (14 * SCALE, 21 * SCALE), (17 * SCALE, 18 * SCALE),
         (21 * SCALE, 18 * SCALE), (24 * SCALE, 22 * SCALE),
         (27 * SCALE, 18 * SCALE), (31 * SCALE, 18 * SCALE),
         (34 * SCALE, 21 * SCALE), (34 * SCALE, 26 * SCALE)],
        fill=(250, 212, 236, 255),
    )
    draw.polygon(
        [(24 * SCALE, 32 * SCALE), (17 * SCALE, 25 * SCALE),
         (17 * SCALE, 22 * SCALE), (20 * SCALE, 21 * SCALE),
         (24 * SCALE, 25 * SCALE), (28 * SCALE, 21 * SCALE),
         (31 * SCALE, 22 * SCALE), (31 * SCALE, 25 * SCALE)],
        fill=(255, 255, 244, 255),
    )

    for x, y in ((24, 5), (5, 24), (43, 24), (24, 43)):
        draw.ellipse(box((x - 1, y - 1, x + 1, y + 1)),
                     fill=(223, 255, 241, 255))

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

    output = (
        template[:32]
        + b"".join(encode_rgb5a3(color) for color in PALETTE)
        + encode_ci4(draw_icon())
    )
    if len(output) != len(template):
        raise ValueError("Generated BTI size does not match the template")

    (ROOT / "res" / "song_of_healing.bti").write_bytes(output)


if __name__ == "__main__":
    main()

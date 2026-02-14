import hashlib
import os
from pathlib import Path

from PIL import Image

# Matches the existing tools/png_to_array.py behavior

def rgb888_to_rgb565_swapped(r: int, g: int, b: int, a: int, alpha_threshold: int = 128) -> int:
    if a < alpha_threshold:
        return 0x0000

    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F

    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Force byte swap to match the display's expected byte order (same as png_to_array.py)
    rgb565 = ((rgb565 & 0x00FF) << 8) | ((rgb565 & 0xFF00) >> 8)
    return rgb565


def load_png_rgb565(png_path: Path, target_size=(48, 48), alpha_threshold: int = 128):
    img = Image.open(png_path).convert("RGBA")
    # Pillow version compatibility
    try:
        img = img.resize(target_size, Image.Resampling.LANCZOS)
    except AttributeError:
        img = img.resize(target_size, Image.ANTIALIAS)

    width, height = img.size
    pixels = img.load()

    data = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            data.append(rgb888_to_rgb565_swapped(r, g, b, a, alpha_threshold))

    return width, height, data


def safe_stem_id(stem: str) -> str:
    # Use a stable, ASCII-only identifier for filenames/array names
    h = hashlib.md5(stem.encode("utf-8")).hexdigest()[:8]
    return f"ico_{h}"


def extract_code_and_label(stem: str):
    # stem like "02阴" or "13阵雪（夜间）" or "4中度霾"
    i = 0
    while i < len(stem) and stem[i].isdigit():
        i += 1
    code = stem[:i] if i > 0 else ""
    label = stem[i:] if i < len(stem) else ""
    return code, label


def write_h_c(out_dir: Path, base_id: str, array_name: str, w: int, h: int, data):
    out_dir.mkdir(parents=True, exist_ok=True)

    h_path = out_dir / f"{base_id}.h"
    c_path = out_dir / f"{base_id}.c"

    guard = f"{array_name.upper()}_H"

    # .h (extern declarations)
    with h_path.open("w", encoding="utf-8") as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"// {w}x{h} RGB565 (transparent = 0x0000, byte-swapped)\n")
        f.write(f"extern const uint16_t {array_name}[];\n")
        f.write(f"extern const uint32_t {array_name}_len;\n")
        f.write(f"#define {array_name}_W {w}\n")
        f.write(f"#define {array_name}_H {h}\n")
        f.write(f"#define {array_name}_TRANSPARENT 0x0000\n\n")
        f.write(f"#endif // {guard}\n")

    # .c (definitions)
    with c_path.open("w", encoding="utf-8") as f:
        f.write(f"#include \"{h_path.name}\"\n\n")
        f.write(f"const uint16_t {array_name}[] = {{\n")
        total = len(data)
        for i in range(0, total, 16):
            chunk = data[i : i + 16]
            hex_line = ", ".join([f"0x{v:04X}" for v in chunk])
            f.write(f"    {hex_line}{'' if i + 16 >= total else ','}\n")
        f.write("};\n\n")
        f.write(f"const uint32_t {array_name}_len = sizeof({array_name}) / sizeof(uint16_t);\n")


def write_icons_inc(inc_path: Path, entries):
    inc_path.parent.mkdir(parents=True, exist_ok=True)

    with inc_path.open("w", encoding="utf-8") as f:
        f.write("// Auto-generated. DO NOT EDIT.\n")
        f.write("// Source: data/new_ico/*.png\n")
        f.write("\n")
        f.write("// All icons are resized to 48x48 and stored as RGB565 (byte-swapped).\n")
        f.write("// Transparent pixels (alpha < 128) become 0x0000 (black).\n")
        f.write("\n")

        for e in entries:
            f.write(f"// {e['stem']} ({e['png_name']})\n")
            f.write(f"const uint16_t {e['array_name']}[] PROGMEM = {{\n")
            data = e["data"]
            total = len(data)
            for i in range(0, total, 16):
                chunk = data[i : i + 16]
                hex_line = ", ".join([f"0x{v:04X}" for v in chunk])
                f.write(f"    {hex_line}{'' if i + 16 >= total else ','}\n")
            f.write("};\n\n")

        f.write("struct WeatherIconEntry {\n")
        f.write("    const char* label;\n")
        f.write("    const uint16_t* pixels;\n")
        f.write("};\n\n")
        f.write("// label comes from filename (digits stripped). Matching is done in code.\n")
        f.write("static const WeatherIconEntry WEATHER_ICON_TABLE[] = {\n")
        for e in entries:
            label = e["label"]
            # escape backslashes and quotes
            label_escaped = label.replace("\\", "\\\\").replace('"', '\\"')
            f.write(f"    {{\"{label_escaped}\", {e['array_name']}}},\n")
        f.write("};\n")
        f.write("static const size_t WEATHER_ICON_TABLE_COUNT = sizeof(WEATHER_ICON_TABLE) / sizeof(WEATHER_ICON_TABLE[0]);\n")
        f.write("static const int WEATHER_ICON_W = 48;\n")
        f.write("static const int WEATHER_ICON_H = 48;\n")


def main():
    repo_root = Path(__file__).resolve().parents[1]
    src_dir = repo_root / "data" / "new_ico"

    out_dir = repo_root / "tools" / "generated_new_ico"
    inc_path = repo_root / "include" / "weather_icons_generated.inc"

    pngs = sorted(src_dir.glob("*.png"), key=lambda p: p.name)
    if not pngs:
        raise SystemExit(f"No PNG files found in: {src_dir}")

    entries = []
    for png in pngs:
        stem = png.stem
        code, label = extract_code_and_label(stem)
        if not label:
            label = stem

        w, h, data = load_png_rgb565(png, target_size=(48, 48), alpha_threshold=128)
        if (w, h) != (48, 48):
            raise SystemExit(f"Unexpected size after resize for {png}: {w}x{h}")

        base_id = safe_stem_id(stem)
        # stable array name for the intermediate .c/.h
        c_array_name = f"{base_id}_data"

        # stable array name for icons.h inclusion
        md = hashlib.md5(stem.encode("utf-8")).hexdigest()[:6]
        code_part = code if code else "xx"
        inc_array_name = f"icon_weather_{code_part}_{md}"

        write_h_c(out_dir, base_id, c_array_name, w, h, data)

        entries.append(
            {
                "png_name": png.name,
                "stem": stem,
                "code": code,
                "label": label,
                "array_name": inc_array_name,
                "data": data,
            }
        )

    write_icons_inc(inc_path, entries)

    print("✅ Generated:")
    print(f"- {inc_path}")
    print(f"- {out_dir} ({len(pngs)} icons -> {len(pngs)*2} files)")


if __name__ == "__main__":
    main()

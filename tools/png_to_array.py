import sys
from PIL import Image

def rgb888_to_rgb565(r, g, b, a, alpha_threshold=128):
    if a < alpha_threshold:
        return 0x0000

    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F

    # 标准RGB565位序
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # 强制反转字节（适配硬件）
    rgb565 = ((rgb565 & 0x00FF) << 8) | ((rgb565 & 0xFF00) >> 8)

    return rgb565

def png_to_rgb565_c_array(png_path, output_h, output_c, array_name, 
                          target_size=(32, 32),
                          alpha_threshold=128):
    """
    最终版：适配硬件解析逻辑的RGB565转换
    """
    # 1. 读取图片（强制保留RGBA通道）
    try:
        img = Image.open(png_path).convert('RGBA')
        # 缩放（兼容新旧Pillow）
        try:
            img = img.resize(target_size, Image.Resampling.LANCZOS)
        except AttributeError:
            img = img.resize(target_size, Image.ANTIALIAS)
        width, height = img.size
        assert (width, height) == target_size, f"尺寸错误：{width}×{height}（预期{target_size}）"
    except Exception as e:
        print(f"图片处理失败：{e}")
        sys.exit(1)

    # 2. 转换像素（直接适配硬件位序）
    pixels = img.load()
    rgb565_data = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            rgb565 = rgb888_to_rgb565(r, g, b, a, alpha_threshold)
            rgb565_data.append(rgb565)

    # 3. 生成.h文件
    with open(output_h, 'w', encoding='utf-8') as f:
        f.write(f"#ifndef {array_name.upper()}_H\n")
        f.write(f"#define {array_name.upper()}_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"// {width}×{height} RGB565数组（透明色=0x0000，适配硬件位序）\n")
        f.write(f"extern const uint16_t {array_name}[];\n")
        f.write(f"extern const uint32_t {array_name}_len;\n")
        f.write(f"#define {array_name}_W {width}\n")
        f.write(f"#define {array_name}_H {height}\n")
        f.write(f"#define {array_name}_TRANSPARENT 0x0000\n\n")
        f.write(f"#endif // {array_name.upper()}_H\n")

    # 4. 生成.c文件（无多余逗号）
    with open(output_c, 'w', encoding='utf-8') as f:
        h_filename = output_h.split('\\')[-1] if '\\' in output_h else output_h.split('/')[-1]
        f.write(f"#include \"{h_filename}\"\n\n")
        f.write(f"const uint16_t {array_name}[] = {{\n")
        
        total = len(rgb565_data)
        for i in range(0, total, 16):
            end = min(i+16, total)
            line_data = rgb565_data[i:end]
            hex_line = ', '.join([f"0x{val:04X}" for val in line_data])
            f.write(f"    {hex_line}{'' if end==total else ','}\n")
        
        f.write(f"}};\n\n")
        f.write(f"const uint32_t {array_name}_len = sizeof({array_name}) / sizeof(uint16_t);\n")

    # 输出信息
    print(f"✅ 转换完成！")
    print(f"- 尺寸：{width}×{height}")
    print(f"- 透明阈值：Alpha < {alpha_threshold} → 0x0000")
    print(f"- 输出：{output_h}, {output_c}")

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("用法：python png2rgb565.py 输入.png 输出.h 输出.c 数组名")
        sys.exit(1)
    
    png_to_rgb565_c_array(
        png_path=sys.argv[1],
        output_h=sys.argv[2],
        output_c=sys.argv[3],
        array_name=sys.argv[4],
        target_size=(48, 48)
    )
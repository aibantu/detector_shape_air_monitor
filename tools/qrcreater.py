import qrcode
import numpy as np
from PIL import Image

def generate_qr_code(
    ap_ssid: str = "BanAirMonitor",
    ap_password: str = "",
    config_url: str = "http://192.168.4.1",
    qr_size: int = 40,  # 适配240×320/320×320屏幕的最优点阵尺寸
    qr_block_size: int = 7,  # 单个模块显示像素（7px）
    output_img: str = "BanAirMonitor_qr_code.png"
) -> tuple[list[int], int]:
    """
    生成适配ESP32-S3（240×320/320×320屏幕）的WiFi配置二维码
    并输出ESP32可用的C语言点阵数组
    
    Args:
        ap_ssid: ESP32 AP名称
        ap_password: AP密码（无密码则为空）
        config_url: 配置页URL
        qr_size: 二维码点阵尺寸（最优40）
        qr_block_size: 单个点阵模块显示像素（最优7）
        output_img: 二维码图片输出路径
    
    Returns:
        (二维码点阵数组, 点阵尺寸)
    """
    # 1. 构造二维码内容（WiFi连接+URL格式）
    encryption = "NONE" if ap_password == "" else "WPA"
    qr_content = f"WIFI:T:{encryption};S:{ap_ssid};P:{ap_password};;H:false;;"
    print(f"二维码内容: {qr_content}")
    print(f"适配屏幕：240×320/320×320 | 点阵尺寸：{qr_size}×{qr_size} | 显示尺寸：{qr_size*qr_block_size}×{qr_size*qr_block_size}px")

    # 2. 生成二维码（根据点阵尺寸自动适配版本）
    # 版本对应：版本1=21x21, 版本2=25x25, 版本3=29x29, 版本4=33x33, 版本5=37x33, 版本6=41x41
    qr_version = (qr_size - 21) // 4 + 1 if qr_size >=21 else 1
    qr = qrcode.QRCode(
        version=qr_version,
        error_correction=qrcode.constants.ERROR_CORRECT_M,  # 中纠错（平衡大小和容错）
        box_size=10,  # 图片渲染的基础像素（不影响点阵）
        border=1,     # 二维码边框（1个模块）
    )
    qr.add_data(qr_content)
    qr.make(fit=True)

    # 3. 生成PIL图片并缩放到目标点阵大小
    img = qr.make_image(fill_color="black", back_color="white")
    img = img.resize((qr_size, qr_size), Image.NEAREST)  # 无插值，保证点阵准确
    img.save(output_img)
    print(f"二维码图片已保存到: {output_img}")

    # 4. 转换为点阵数组（1=黑块，0=白块）
    img_array = np.array(img.convert("L"))  # 灰度图（0=黑，255=白）
    qr_matrix = []
    for y in range(qr_size):
        for x in range(qr_size):
            qr_matrix.append(1 if img_array[y, x] < 128 else 0)

    # 5. 生成ESP32可用的C语言数组代码
    print("\n=== ESP32-S3可用的C语言点阵数组 ===")
    print(f"// 适配240×320/320×320屏幕 | 显示尺寸：{qr_size*qr_block_size}×{qr_size*qr_block_size}px")
    print(f"const uint8_t qrCodeData[] = {{")
    # 每行输出qr_size个元素，提升代码可读性
    for i in range(0, len(qr_matrix), qr_size):
        row = qr_matrix[i:i+qr_size]
        row_str = ", ".join(map(str, row)) + ","
        # 每行缩进+换行，适配ESP32代码格式
        print(f"    {row_str}")
    print(f"}};")
    print(f"const int qrCodeSize = {qr_size};")
    print(f"const int qrBlockSize = {qr_block_size}; // 单个模块显示像素（最优7）")

    return qr_matrix, qr_size

# 主函数
if __name__ == "__main__":
    # 核心配置（和ESP32项目中的AP一致）
    AP_SSID = "BanAirMonitor"    # 必须和ESP32代码中的AP名称一致
    AP_PASSWORD = ""            # AP无密码则为空，有密码填实际值
    CONFIG_URL = "http://192.168.4.1"  # 配置页URL,不带

    # 适配240×320/320×320屏幕的最优参数
    OPTIMAL_QR_SIZE = 32        # 最优点阵尺寸
    OPTIMAL_BLOCK_SIZE = 5      # 最优单模块像素

    # 生成二维码和点阵数组
    generate_qr_code(
        ap_ssid=AP_SSID,
        ap_password=AP_PASSWORD,
        config_url=CONFIG_URL,
        qr_size=OPTIMAL_QR_SIZE,
        qr_block_size=OPTIMAL_BLOCK_SIZE,
        output_img="BanAirMonitor_qr_code.png"
    )
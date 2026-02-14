替换.pio中的TFT_eSPI.h
将ESP32_S3_ST7789.h复制到User_Setups文件夹中
**GIF 显示（分帧播放）**

- **概述:** : 将动画 GIF 拆帧为一系列原始 RGB565 帧，放入 `data/res/frames/`，通过 SPIFFS 上传到设备，然后在 `src/main.cpp` 中逐帧读取并只更新右下角 `100×100` 区域以播放动画，保持原有界面布局不变。

- **拆帧脚本:** : 项目中提供 `tools/split_gif.py`。用法示例：

	```powershell
	python tools\split_gif.py res\358.gif --out data\res\frames --size 100
	```

	- 输出文件格式：`frame000.raw`, `frame001.raw`, …
	- 每帧为 **RGB565 little-endian**（2 bytes/pixel），尺寸为 `size x size`（上例为 100×100），每帧大小 = 100 * 100 * 2 = 20000 字节。

- **上传到 SPIFFS:** : 将 `data/` 下的内容上传到设备文件系统（PlatformIO）：

	```powershell
	& 'C:\Users\<你用户名>\.platformio\penv\Scripts\platformio.exe' run -t uploadfs
	```

	或在 VS Code PlatformIO 中使用 “Upload File System Image”。上传成功后设备启动时会在串口打印 `Found frame count: N`。

- **代码行为（已实现）:** :
	- 启动时挂载 SPIFFS 并扫描 `/res/frames/frameNNN.raw`，记录 `gifFrameCount`。
	- 若 `gifFrameCount > 0`，`updateGifArea()` 会按 `gifFrameInterval` 读取每帧并通过 `sensorSprite->pushImage()` 写入 sprite 区域，接着 `pushSprite()` 只更新右下角 100×100 区域。其他 UI（温湿度、CO2 等）和部分重绘逻辑保持不变。
	- 若没有帧文件，程序使用占位动画回退，保证不会崩溃。

- **调整帧率与性能调优建议:** :
	- 当前参数：`gifFrameInterval` 在 `src/main.cpp` 中可调（默认 50 ms → 约 20 FPS）。要提高帧率请减小该值，但受 SPIFFS 读速与屏驱限制。
	- 优化选项：
		- 把所有帧合并为一个连续文件并按偏移读取（减少频繁 open/close 开销）。
		- 使用更小的像素格式（例如 8-bit 调色板）以减小 IO 数据量。
		- 若仍不够，可考虑把帧存放在更快的分区或使用外部 SPI Flash。

- **调试与确认:** :
	- 在串口监视器（115200 或项目中设置的波特率）查看输出：`SPIFFS mounted`、`Found frame count: N`。
	- 如果帧数为 0，确认 `data/res/frames/` 已上传且文件名格式为 `frame%03d.raw`。

如需我把帧上传并在设备上验证播放（并进一步优化帧率），可以直接让我执行 `uploadfs` 和重刷固件并打开串口查看日志。



传感器
• DHT22 温湿度
◦ DHT 数据线 → ESP32-S3 GPIO2（DHTPIN=2）
◦ VCC → 3.3V（或模块要求电压）
◦ GND → GND
◦ 备注：如果是裸 DHT22，通常需要  DATA 到 3.3V 上拉电阻（常见 4.7k–10k）；带小板模块一般自带上拉
• CO2（UART 被动输出）
◦ 传感器 TXD → ESP32-S3 GPIO38（CO2_UART_RX=38）
◦ 传感器 RXD → 不需要连接（工程里 CO2_UART_TX=-1）
◦ 传感器 GND → ESP32-S3 GND（必须共地）
◦ 传感器 VCC → 按模块要求（3.3V 或 5V）
◦ 重要：ESP32-S3 IO 为 3.3V 逻辑；若 CO2 模块 TXD 是 5V TTL，建议加电平转换/分压
屏幕（TFT_eSPI，ST7789，SPI，240×320）
• 供电：VCC → 3.3V（多数 ST7789 模块可 3.3V；如你模块支持 5V 也可，但信号仍建议 3.3V 逻辑）、GND → GND
• SPI 信号（来自  ESP32_S3_ST7789.h）
◦ SCK/SCLK → GPIO18（TFT_SCLK=18）
◦ MOSI/SDA/DIN → GPIO15（TFT_MOSI=15）
◦ CS → GPIO17（TFT_CS=17）
◦ DC/A0 → GPIO16（TFT_DC=16）
◦ RST → GPIO5（TFT_RST=5）
◦ MISO → 未用（TFT_MISO=-1）
◦ BL/LED(背光) → 未由 MCU 控制（TFT_BL=-1，若模块需要背光脚，通常直接上拉到电源或通过三极管/PWM 自己加）
• 触摸：工程编译期提示未定义 TOUCH_CS，即 未使用触摸（不需要接触摸相关引脚）
按键
• BUTTON_CLEAR=GPIO0：INPUT_PULLUP，按下判定为 LOW
◦ 接法：按钮一端 → GPIO0，另一端 → GND
◦ 注意：GPIO0 常作为启动/下载相关的“绑带脚”，如果上电或复位时按住，可能影响启动模式（画电路时建议标注/或改到非绑带脚更稳）
• BUTTON_WEATHER=GPIO41：INPUT_PULLUP，按下判定为 LOW
◦ 接法：按钮一端 → GPIO41，另一端 → GND
• BUTTON_TOGGLE_MODE=GPIO14：当前配置 BUTTON_TOGGLE_ACTIVE_LOW=0（也就是 高电平有效，并启用 INPUT_PULLDOWN）
◦ 接法（按当前代码）：按钮一端 → GPIO14，另一端 → 3.3V（按下读到 HIGH）
◦ 如果你更想“按下接地”：把 BUTTON_TOGGLE_ACTIVE_LOW 改成 1，接线改为按钮另一端接 GND（用内部上拉）
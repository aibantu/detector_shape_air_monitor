### 项目背景
这是一个桌面气象站, 有一个屏幕，可以显示温度、湿度、二氧化碳浓度。
有电池，主控使用ESP32S3-N8R8，


传感器
• DHT22 温湿度
◦ DHT是数据线 连接到 ESP32-S3 的GPIO13
◦ VCC → 3.3V（或模块要求电压）
◦ GND → GND

• CO2 传感器（UART 被动输出）
◦ 传感器 TXD → ESP32-S3 GPIO5
◦ 传感器 RXD → ESP32-S3 GPIO4
◦ 传感器 GND → ESP32-S3 GND（必须共地）
◦ 传感器 VCC → 按模块要求（5V）

屏幕（TFT_eSPI，ST7789，SPI，240×320）
• 供电：VCC → 3.3V、GND → GND
• SPI 信号（来自  ESP32_S3_ST7789.h）

◦ SCK/SCLK → GPIO18（TFT_SCLK=18）
◦ MOSI/SDA/DIN → GPIO17（TFT_MOSI=17）
◦ RST → GPIO16（TFT_RST=16）
◦ DC/A0 → GPIO15（TFT_DC=15）
◦ CS → GPIO7（TFT_CS=7）
◦ BL/LED(背光) → GPIO6
• 触摸：工程编译期提示未定义 TOUCH_CS，即 未使用触摸（不需要接触摸相关引脚）

按键
• BUTTON_WEATHER=GPIO8,高电平触发


充电情况：
当插入typeC充电接口时，会有电压进入，依次经过R1是2K欧姆的电阻和R2是3K欧姆的电阻后与大地GND相连，同时在经过R1后接入ESP32-S3的GPIO9,当该引脚为高电平时，表示正在充电，否则为没有充电。

电池电量显示情况：
电池的正极依次经过R17为2.7K的电阻和R16为4.7K的电阻后与大地GND相连，同时经过R17后有一路接到ESP32-S3的GPIO12，


优先：把 co2Serial 改为 UART1/2（软件修改）并验证 USB 串口稳定性（立即执行）。
把 co2Serial 改为 UART1/2（软件修改）并验证 USB 串口稳定性（立即执行），请问这是要做什么？CO2传感器使用GPIO4和GPIO5读取信息，改成 UART1/2的目的是什么？会影响正常的读取吗？

简短回答：把 co2Serial 从 UART0（HardwareSerial(0)）改为 UART1/UART2 的目的是避免与 USB 串口（Serial，默认映射到 UART0）发生冲突。对 CO2 传感器本身的读取不会受影响——只要你在 begin() 时把 RX/TX 引脚（GPIO4/GPIO5）按原样传进去，ESP32-S3 的 IO 矩阵会把选定的 UART 映射到这些引脚，数据仍然能正常收发。

详细说明与注意点（要点）：
为什么要改：
HardwareSerial co2Serial(0) 使用 UART0，UART0 通常是 USB 串口（Serial）。在运行时对 UART0 重新配置 RX/TX 或在两个 Serial 实例上同时使用同一串口，会导致 USB 串口不稳定、调试输出丢失，甚至与硬件启动行为产生交互（可能与你遇到的插拔重启有关）。
改成 UART1/2 会做什么：
只改变 UART 实例编号（1 或 2）；不改变你物理接线（GPIO4，GPIO5）——因为 co2Serial.begin(baud, config, rxPin, txPin) 会把该 UART 的 RX/TX 映射到指定 GPIO。
会不会影响读取？
不会，只要：
你把 co2Serial 的构造参数从 0 改成 1（或 2），
保持 co2Serial.begin(BAUD_RATE, SERIAL_8N1, CO2_UART_RX, CO2_UART_TX); 中的 RX/TX 引脚不变（GPIO4/GPIO5），
并确保这些 GPIO 在板子上不是被其他关键外设占用或不是受限启动引脚。
其他须知：
选用 UART1/UART2 时请确认该 UART 的硬件资源在你的板上可用（ESP32-S3 通常有 UART0/1/2 可用）。
避免把 CO2 的 RX/TX 指定到与 TFT SPI、PSRAM、或 flash/boot-strapping 冲突的 GPIO。
在改动后重新编译并刷机，观察串口（USB）是否稳定，以及 CO2 数据是否正常。


优先：确认 CHARGING_PIN（GPIO9）是否为安全引脚；若不是，换用安全 IO（软件 + 硬件改线）。


中期：将电池读取移入 readSensorData() 并把数据放到 SensorData（代码重构，利于统一采样与滤波）。
中期：根据串口日志判断是否为 brownout，若是按硬件补救方案处理。
长期：若需要更稳定并发/响应，考虑把显示、传感、网络放到独立 FreeRTOS 任务或重构事件循环，减少阻塞调用。
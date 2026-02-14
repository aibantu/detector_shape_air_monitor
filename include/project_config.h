#pragma once

// 集中项目配置常量
#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

// 屏幕与显示
#define LINE_NUM        5
#define SMALL_FONT_ID   2
#define MIDDLE_FONT_ID  4
#define LARGE_FONT_ID   7
#define CIRCLE_RADIUS   3

// 串口与屏幕旋转
#define BAUD_RATE       115200
#ifndef TFT_ROTATION
#define TFT_ROTATION    1
#endif

// Indoor temperature/humidity sensor (SHT31D / SHT3X-D, I2C)
// Wiring: SDA -> GPIO13, SCL -> GPIO14 (see src/read_I2C.cpp for reference)
#ifndef INDOOR_I2C_SDA
#define INDOOR_I2C_SDA 13
#endif
#ifndef INDOOR_I2C_SCL
#define INDOOR_I2C_SCL 14
#endif
#ifndef INDOOR_I2C_FREQ_HZ
#define INDOOR_I2C_FREQ_HZ 100000
#endif
#ifndef SHT3XD_ADDR_DEFAULT
#define SHT3XD_ADDR_DEFAULT 0x44
#endif
#ifndef SHT3XD_ADDR_ALT
#define SHT3XD_ADDR_ALT 0x45
#endif

#define UPDATE_INTERVAL 1000    // 数据更新时间间隔（ms）
#define TEMP_THRESHOLD  0.1
#define HUMI_THRESHOLD  0.1

// CO2 传感器串口（被动输出 16 字节帧，每秒一次）
// 固定接线: 传感器 TX -> ESP32-S3 GPIO38 (RX)；GND 共地；不需要 MCU TX
// 说明: 使用 UART0 IO 矩阵映射 RX 到 38；若某板载定义不支持可改为 UART1 并保持 begin 参数一致
#define CO2_UART_RX 4
#define CO2_UART_TX 5  // 未使用
#define CO2_UART_NUM 1 // 避免使用UART0，防止与串口打印冲突

// WiFi配网AP配置
#define AP_SSID "BanAirMonitor"    // 配网热点名称
// #define AP_PASS "12345678"        // 配网热点密码（至少8位）

// NTP时间配置
#define NTP_SERVER "pool.ntp.org" // NTP服务器地址
#define GMT_OFFSET 8 * 3600       // 时区偏移（东八区：8*3600秒）


#define BUTTON_CLEAR 0
#define BUTTON_WEATHER 41

// Mode toggle button (INDOOR <-> OUTDOOR view on the weather page)
// Signal pin can be GPIO13 or GPIO14 (INPUT_PULLUP, active LOW)
#define BUTTON_TOGGLE_MODE 8

// Toggle button electrical level
// 1: active LOW (pressed -> LOW, uses INPUT_PULLUP)
// 0: active HIGH (pressed -> HIGH, uses INPUT_PULLDOWN)
#define BUTTON_TOGGLE_ACTIVE_LOW 0

// 电池与充电检测（硬件设计：分压后接到 GPIO12，充电状态由 GPIO9 提供高电平）
#ifndef BATTERY_PIN
#define BATTERY_PIN 12
#endif

#ifndef CHARGING_PIN
#define CHARGING_PIN 9
#endif

// 分压系数：Vbatt = Vpin * FACTOR
// R17=2.7K, R16=4.7K => Vbatt = Vpin * (R17+R16)/R16 = Vpin * 6.7/4.7 ≈ 1.4255
#ifndef BATTERY_VOLTAGE_FACTOR
#define BATTERY_VOLTAGE_FACTOR 1.4255f
#endif

// 电阻值（以欧姆为单位），用于按实际电阻计算分压比（默认基于 PCB / 文档）
#ifndef BATTERY_R_TOP_OHMS
#define BATTERY_R_TOP_OHMS 2700  // R17 = 2.7k
#endif
#ifndef BATTERY_R_BOTTOM_OHMS
#define BATTERY_R_BOTTOM_OHMS 4700 // R16 = 4.7k
#endif

// 可选校准：如果你已用万用表测得电池实际电压（mV），填写在此处。
// 例如：4.06V -> 4060
#ifndef BATTERY_CALIBRATION_MV
#define BATTERY_CALIBRATION_MV 0
#endif

// 电池电压视觉化范围（可调整）
#ifndef BATTERY_VOLTAGE_MIN
#define BATTERY_VOLTAGE_MIN 3.70f
#endif
#ifndef BATTERY_VOLTAGE_MAX
#define BATTERY_VOLTAGE_MAX 4.20f
#endif

// // 任选：IP 或域名
// const char* API_BASE = "http://120.48.177.209:8000"; 
// // const char* API_BASE = "https://banbot.cn";

#endif
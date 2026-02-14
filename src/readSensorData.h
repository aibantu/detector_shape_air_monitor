#pragma once

#ifndef READ_SENSOR_DATA_H__
#define READ_SENSOR_DATA_H__

#include <Arduino.h>
#include "project_config.h"

#define CO2_BUF_MAX         64      // 缓冲区最大长度
#define CO2_FRAME_LEN       16      // 传感器帧固定长度
#define CO2_BYTE_TIMEOUT    2500    // 超过此时间未接收新字节则清空缓冲区（ms）
#define CO2_FRAME_TIMEOUT   3500    // 超过此时间
#define CO2_PPM_MIN         400     //合理下线
#define CO2_PPM_MAX         5000    //合理上线

// 传感器数据结构（由 main.cpp 定义实例）
struct SensorData {
    // Indoor sensor fields (filled by readSensorData)
    float temp;
    float humi;
    uint16_t co2;

    // Outdoor weather fields (filled by weatherUtils)
    float outdoorTemp;
    float outdoorHumi;

    int8_t windLevel;     // 0~12, -1 means unknown
    float lastTemp;
    float lastHumi;
    uint16_t lastCO2;

    int8_t lastWindLevel; // 0~12, -1 means unknown

    // Weather description text (e.g. "阴", "小雨")
    String weatherText;
    String lastWeatherText;
};

// 外部实例（在 main.cpp 中定义）
// extern SensorData sData;
extern HardwareSerial co2Serial;

// 初始化室内温湿度传感器（SHT31D/SHT3X-D, I2C）。
// 默认使用 INDOOR_I2C_SDA/INDOOR_I2C_SCL 与 0x44/0x45 地址。
bool initIndoorTempHumiSensor();

// 读取传感器数据的函数
void readSensorData(SensorData* data);

#endif // READ_SENSOR_DATA_H__

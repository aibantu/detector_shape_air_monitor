#include "readSensorData.h"

#include <Wire.h>
#include "ClosedCube_SHT3XD.h"

static ClosedCube_SHT3XD g_sht3xd;
static bool g_sht3xdReady = false;
static uint8_t g_sht3xdAddr = SHT3XD_ADDR_DEFAULT;
static uint32_t g_lastShtWarnMs = 0;

static uint8_t i2cPing(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission(true);
}

static bool tryInitSht3xd(uint8_t address)
{
    g_sht3xd.begin(address);

    // ClosedCube begin() may call Wire.begin() without pin mapping; re-apply.
    Wire.begin(INDOOR_I2C_SDA, INDOOR_I2C_SCL);
    Wire.setClock(INDOOR_I2C_FREQ_HZ);
#if defined(ESP32)
    Wire.setTimeOut(50);
#endif

    if (i2cPing(address) != 0)
        return false;

    (void)g_sht3xd.softReset();
    delay(10);

    const SHT3XD_ErrorCode rc = g_sht3xd.periodicStart(REPEATABILITY_HIGH, FREQUENCY_1HZ);
    if (rc != NO_ERROR)
        return false;

    g_sht3xdAddr = address;
    g_sht3xdReady = true;
    return true;
}

bool initIndoorTempHumiSensor()
{
    // Ensure I2C bus is up with expected pins.
    Wire.begin(INDOOR_I2C_SDA, INDOOR_I2C_SCL);
    Wire.setClock(INDOOR_I2C_FREQ_HZ);

    g_sht3xdReady = false;

    if (tryInitSht3xd(SHT3XD_ADDR_DEFAULT)) {
        Serial.printf("SHT31D initialized at 0x%02X\n", (unsigned)g_sht3xdAddr);
        return true;
    }
    if (tryInitSht3xd(SHT3XD_ADDR_ALT)) {
        Serial.printf("SHT31D initialized at 0x%02X\n", (unsigned)g_sht3xdAddr);
        return true;
    }

    Serial.println("SHT31D init failed (0x44/0x45 NACK or periodicStart error)");
    return false;
}

// 定义 readSensorData.h 中声明的全局变量（单一定义）
uint8_t co2Buf[CO2_BUF_MAX] = {0};
size_t co2BufLen = 0;
uint16_t co2ppm = 0;
uint8_t lastFrame[CO2_FRAME_LEN] = {0};
unsigned long lastFrameMillis = 0;
unsigned long co2FrameCount = 0;

// void processCo2Buffer(SensorData* data);
void readCO2SensorData(SensorData* data);

void readSensorData(SensorData* data){

    // data->lastCO2 = data->co2;
    // data->lastTemp = data->temp;
    // data->lastHumi = data->humi;
    
        // 使用 SHT31D（I2C）读取温湿度
        if (!g_sht3xdReady) {
            // Lazy init: avoid blocking boot if sensor is absent.
            (void)initIndoorTempHumiSensor();
        }

        if (g_sht3xdReady) {
            const SHT3XD result = g_sht3xd.periodicFetchData();
            if (result.error == NO_ERROR && result.rh >= 0.0f && result.rh <= 100.0f) {
                data->temp = result.t;
                data->humi = result.rh;
                Serial.printf("Indoor Temp: %.2f C, Humi: %.2f %%\n", data->temp, data->humi);
            } else {
                const uint32_t nowMs = millis();
                if (nowMs - g_lastShtWarnMs > 5000) {
                    g_lastShtWarnMs = nowMs;
                    Serial.printf("[WARN] SHT31D read failed addr=0x%02X err=%d\n", (unsigned)g_sht3xdAddr, (int)result.error);
                }
            }
        }

        readCO2SensorData(data); 

}

void readCO2SensorData(SensorData* data) {

    static unsigned long lastByteMillis = 0;
    size_t beforeLen = co2BufLen;

    size_t readCount = 0;
    while (co2Serial.available() && readCount < 32) {
        uint8_t b = (uint8_t)co2Serial.read();
        if (co2BufLen < CO2_BUF_MAX) {
            co2Buf[co2BufLen] = b;
            co2BufLen++;
        } else {
            // 缓冲区满，丢弃字节
            Serial.println("[ERROR] CO2 buffer overflow, clear buffer");
            co2BufLen = 0;
            break;
        }
        lastByteMillis = millis();
        readCount++;
    }

    // 仅用于打印新增数据，仅用于调试
    // if (co2BufLen != beforeLen) {
    //     Serial.print("Passive CO2 bytes received: +"); Serial.print(co2BufLen - beforeLen);
    //     Serial.print(" total="); Serial.println(co2BufLen);
    //     size_t dumpN = co2BufLen < 16 ? co2BufLen : 16;
    //     Serial.print("Buf head: ");
    //     for (size_t z = 0; z < dumpN; z++) {
    //     if (co2Buf[z] < 16) Serial.print('0');
    //     Serial.print(co2Buf[z], HEX);
    //     Serial.print(' ');
    //     }
    //     Serial.println();
    //     lastReportedCo2BufLen = co2BufLen;
    // }

    // 超时清理缓冲区（超过2.5秒无新数据，清空无效数据）
    if (millis() - lastByteMillis > CO2_BYTE_TIMEOUT && co2BufLen > 0) {
        Serial.print("[WARN] CO2 buffer timeout, clear ");
        Serial.println(co2BufLen);
        co2BufLen = 0;
    }

    // 检查CO2数据是否超时（例如3秒未更新）
    if (millis() - lastFrameMillis > CO2_FRAME_TIMEOUT && co2FrameCount > 0) {
        Serial.println("[WARN] CO2 data timeout");
        data->co2 = 0; 
    }

    while (co2BufLen >= CO2_FRAME_LEN) {
        // 寻找头部（若首字节不是0x42或第二字节不是0x4D，丢弃一个字节继续）
        size_t headerPos = 0;
        bool found = false;
        while (headerPos <= co2BufLen - CO2_FRAME_LEN) {
            if (co2Buf[headerPos] == 0x42 && co2Buf[headerPos + 1] == 0x4D) {
                found = true;
                break;
            }
            headerPos++;
        }
        if (!found) {
            // 未找到有效帧头，保留最后CO2_FRAME_LEN-1字节
            if (co2BufLen > CO2_FRAME_LEN - 1) {
                size_t keep = co2BufLen - (CO2_FRAME_LEN - 1);
                memmove(co2Buf, co2Buf + keep, CO2_FRAME_LEN - 1);
                co2BufLen = CO2_FRAME_LEN - 1;
            }
            break;
        }

        // 如果帧头不在起始位置，移动数据
        if (headerPos > 0) {
            memmove(co2Buf, co2Buf + headerPos, co2BufLen - headerPos);
            co2BufLen -= headerPos;
        }

        // 计算校验和（BYTE0..BYTE14）
        uint16_t sum = 0;
        for (int i = 0; i <= 14; ++i) 
            sum += co2Buf[i];
        uint8_t expected = (uint8_t)(sum & 0xFF);
        // uint8_t recvChk = co2Buf[15];
        bool chkOk = (expected == co2Buf[15]);

        // 解析CO2浓度（BYTE6..BYTE7）
        uint16_t co2val = ((uint16_t)co2Buf[6] << 8) | co2Buf[7];
        // 额外校验：浓度值在合理范围内才更新（过滤异常值）
        bool valOk = (co2val >= CO2_PPM_MIN && co2val <= CO2_PPM_MAX);
        
        // 仅在校验和+数值都有效时，更新核心变量
        if (chkOk && valOk) {
            data->co2 = co2val;
            memcpy(lastFrame, co2Buf, CO2_FRAME_LEN);
            lastFrameMillis = millis();
            co2FrameCount++;
        }

        // 移除已解析帧
        size_t remaining = co2BufLen - CO2_FRAME_LEN;
        if (remaining > 0) 
            memmove(co2Buf, co2Buf + CO2_FRAME_LEN, remaining);
        co2BufLen = remaining;
    }

    // processCo2Buffer(data);
}

// 仅解析 16 字节帧：格式
// BYTE0=0x42, BYTE1=0x4D, BYTE2..BYTE14=数据内容, BYTE15= (BYTE0+...+BYTE14) & 0xFF
// CO2 浓度 = BYTE6*256 + BYTE7
// void processCo2Buffer(SensorData* data) {
//   // 不断尝试解析首部为 0x42 0x4D 的 16 字节帧
//   while (co2BufLen >= CO2_FRAME_LEN) {
//     // 寻找头部（若首字节不是0x42或第二字节不是0x4D，丢弃一个字节继续）
//     size_t headerPos = 0;
//     bool found = false;
//     while (headerPos <= co2BufLen - CO2_FRAME_LEN) {
//         if (co2Buf[headerPos] == 0x42 && co2Buf[headerPos + 1] == 0x4D) {
//             found = true;
//             break;
//         }
//         headerPos++;
//     }
//     if (!found) {
//         // 未找到有效帧头，保留最后CO2_FRAME_LEN-1字节
//         if (co2BufLen > CO2_FRAME_LEN - 1) {
//             size_t keep = co2BufLen - (CO2_FRAME_LEN - 1);
//             memmove(co2Buf, co2Buf + keep, CO2_FRAME_LEN - 1);
//             co2BufLen = CO2_FRAME_LEN - 1;
//         }
//         break;
//     }

//     // 如果帧头不在起始位置，移动数据
//     if (headerPos > 0) {
//         memmove(co2Buf, co2Buf + headerPos, co2BufLen - headerPos);
//         co2BufLen -= headerPos;
//     }
//     // if (co2Buf[0] != 0x42 || co2Buf[1] != 0x4D) {
//     //     // 失配移除首字节
//     //     if (co2BufLen > 1)
//     //         memmove(co2Buf, co2Buf + 1, co2BufLen - 1);
//     //     co2BufLen --;
//     //     // 避免缓冲区被无效单字节填满（比如全是0x00）
//     //     if (co2BufLen > CO2_BUF_MAX - 2) {
//     //         Serial.println("[WARN] CO2 buffer full with invalid data, clear");
//     //         co2BufLen = 0;
//     //         break;
//     //     }
//     //     continue;
//     // }

//     // 计算校验和（BYTE0..BYTE14）
//     uint16_t sum = 0;
//     for (int i = 0; i <= 14; ++i) 
//         sum += co2Buf[i];
//     uint8_t expected = (uint8_t)(sum & 0xFF);
//     // uint8_t recvChk = co2Buf[15];
//     bool chkOk = (expected == co2Buf[15]);

//     // 解析CO2浓度（BYTE6..BYTE7）
//     uint16_t co2val = ((uint16_t)co2Buf[6] << 8) | co2Buf[7];
//     // 额外校验：浓度值在合理范围内才更新（过滤异常值）
//     bool valOk = (co2val >= CO2_PPM_MIN && co2val <= CO2_PPM_MAX);
    
//     // 仅在校验和+数值都有效时，更新核心变量
//     if (chkOk && valOk) {
//         data.co2 = co2val;
//         memcpy(lastFrame, co2Buf, CO2_FRAME_LEN);
//         lastFrameMillis = millis();
//         co2FrameCount++;
//     }
    
//     // 打印帧解析结果（调试用）
//     // Serial.print("CO2 frame: ");
//     // for (int i = 0; i < 16; ++i) {
//     //     if (co2Buf[i] < 16) 
//     //         Serial.print('0');
//     //     Serial.print(co2Buf[i], HEX); 
//     //     Serial.print(' ');
//     // }
//     // Serial.print(" -> CO2="); 
//     // Serial.print(co2ppm);
//     // if (chkOk) {
//     //     Serial.print(" ppm (valid)");
//     // } else {
//     //     Serial.print(" (checksum mismatch exp="); 
//     //     Serial.print(expected, HEX);
//     //     Serial.print(" got=0x"); 
//     //     Serial.print(recvChk, HEX); 
//     //     Serial.print(")");
//     // }
//     // Serial.println();

//     // 移除已解析帧
//     size_t remaining = co2BufLen - CO2_FRAME_LEN;
//     if (remaining > 0) 
//         memmove(co2Buf, co2Buf + CO2_FRAME_LEN, remaining);
//     co2BufLen = remaining;
//   }
// }
#include <TFT_eSPI.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <time.h>
#include <math.h>

#include "project_config.h"
#include "screenDisplay.h"
#include "readSensorData.h"
#include "WiFiManager.h"
#include "esp_system.h"
// #include "timeManager.h"
#include "buttonHandler.h"
#include "weatherUtils.h"



// 使用集中配置头文件中的常量
enum State {
    STATE_WIFI_CHECK,
    // STATE_WIFI_CONNECTING,
    STATE_WIFI_CONFIG,
    STATE_TIME_SET,
    STATE_INDOOR,
    STATE_WEATHER
};

State currentState = STATE_WIFI_CHECK;

// GIF frame interval in milliseconds. Lower = faster playback.

// 室内温湿度：已切换为 SHT31D / SHT3X-D（I2C）

HardwareSerial co2Serial(CO2_UART_NUM);

SensorData sData; // 传感器数据实例

TFT_eSPI tft = TFT_eSPI();


bool isFirstDraw = true; 

enum WeatherViewMode {
    VIEW_OUTDOOR = 0,
    VIEW_INDOOR = 1,
};


unsigned long wifiConnectStart = 0;
const unsigned long WIFI_CONNECT_TIMEOUT = 10000;

static volatile bool g_bootFetchDone = false;
static int g_bootFetchHour = -1;

int scanFrameFiles();
void updateGifArea();
void cleanupOnExit();
void setupTimeManually();


void setup() {
    Serial.begin(BAUD_RATE); // 提升波特率，避免串口卡顿
    delay(200);

    // 注册程序退出时的清理函数
    atexit(cleanupOnExit);  
    tft.init();
    tft.setRotation(TFT_ROTATION); // 设置屏幕旋转角度
    tft.fillScreen(TFT_BLACK);

    // 充电检测引脚初始化：使用下拉避免浮空（有些板子在拔插时会出现毛刺）
    pinMode(CHARGING_PIN, INPUT_PULLDOWN);

    // ADC 设置：提高 ADC 稳定性（12-bit）并选择合适的衰减
    analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP32S3)
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
#endif

    // 打印复位原因，帮助排查拔插导致的重启
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[boot] reset reason=%d\n", (int)reason);

    // 串口输出屏幕参数，验证配置是否正确
    Serial.print("width:");
    Serial.println(tft.width());
    Serial.print("height:");
    Serial.println(tft.height());

    if (!initSprite(&tft)) {
        Serial.println("Error: init dispSprite failed!");
    }

    // 挂载 SPIFFS（用于读取 res 下的图片文件，注意：需将文件上传到 SPIFFS/ LittleFS）
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
    } else {
        Serial.println("SPIFFS mounted");
    }

    // scan frames in SPIFFS (data/res/frames/frame000.raw ...)
    // gifFrameCount = scanFrameFiles();
    // Serial.print("Found frame count: "); Serial.println(gifFrameCount);

    // 初始化 I2C（指定 SDA=GPIO13, SCL=GPIO14）并启动 SHT31D
    Wire.begin(INDOOR_I2C_SDA, INDOOR_I2C_SCL);
    Wire.setClock(INDOOR_I2C_FREQ_HZ);
    (void)initIndoorTempHumiSensor();

    co2Serial.begin(BAUD_RATE, SERIAL_8N1, CO2_UART_RX, CO2_UART_TX);

    initButtons();

    // loadTimeFromRTC();

    // listFiles(); // 列出根目录所有文件
    // listFiles("/res"); // 列出/res目录所有文件 

    Serial.println("Init complete!");

    // Weather fields default (unknown)
    sData.outdoorTemp = NAN;
    sData.outdoorHumi = NAN;
    sData.windLevel = -1;
    sData.lastWindLevel = -2;

    sData.weatherText = "";
    sData.lastWeatherText = "__init__";
}

void loop() {

    // 定期更新电池显示与调试信息（每2秒）
    static unsigned long __lastBatMs = 0;
    const unsigned long __batInterval = 2000;
    if (millis() - __lastBatMs > __batInterval) {
        __lastBatMs = millis();
        if (dispSprite) updateBatteryStatus(dispSprite, &dLayout);
    }


    // 检测按键是否按下（低电平触发）
    static unsigned long lastBtnPress = 0;
    if (isButtonPressed(BUTTON_CLEAR)) {
        // delay(200);  // 消抖
        // if (digitalRead(CLEAR_BTN_PIN) == LOW) {
        //     clearWiFiConfig();
        //     currentState = STATE_WIFI_CHECK;
        // }
        if (millis() - lastBtnPress > 200) { // 非阻塞消抖
            lastBtnPress = millis();
            clearWiFiConfig();
            currentState = STATE_WIFI_CHECK;
        }
    }

    // Toggle indoor/outdoor view while staying on the weather page
    static unsigned long lastTogglePress = 0;
    static WeatherViewMode weatherViewMode = VIEW_OUTDOOR;
    static bool lastTogglePressed = false;
    const bool togglePressedNow = isToggleModePressed();
    const bool toggleEdge = togglePressedNow && !lastTogglePressed;
    lastTogglePressed = togglePressedNow;
    if (toggleEdge) {
        if (millis() - lastTogglePress > 200) {
            lastTogglePress = millis();
            weatherViewMode = (weatherViewMode == VIEW_OUTDOOR) ? VIEW_INDOOR : VIEW_OUTDOOR;

            Serial.printf("[button] toggle mode -> %s\n", weatherViewMode == VIEW_OUTDOOR ? "OUTDOOR" : "INDOOR");

            // Force a full refresh of this page to avoid stale values/icons
            isFirstDraw = true;
            sData.lastTemp = NAN;
            sData.lastHumi = NAN;
            sData.lastCO2 = 0xFFFF;
            sData.lastWindLevel = -2;
            sData.lastWeatherText = "__force__";
        }
    }



    switch (currentState) {
        case STATE_WIFI_CHECK:

            tft.fillScreen(TFT_BLACK);
            if (checkWiFiConfig()) {                

                drawDisplay(false,"Connecting WiFi...");

                if (autoConnectWiFi()){
                    Serial.println("WiFi connected");
                    // syncTimeFromNTP();
                    currentState = STATE_WEATHER;
                } else {
                    clearWiFiConfig();
                    currentState = STATE_WIFI_CONFIG;
                }
            } else {
                Serial.println("Entering WiFi config mode");
                currentState = STATE_WIFI_CONFIG;
            }
            break;
        
        case STATE_WIFI_CONFIG: {
            Serial.println("Start to config");
            WiFi.scanNetworks(true); // 异步扫描
            startConfigAP();
            // String apUrl = "http://" + WiFi.softAPIP().toString();
            // Serial.print("Config AP URL: "); Serial.println(apUrl);
            
            drawDisplay(true, "Please scan the code");

            // 等待配置并重启（确保生效）
            if (waitForWiFiConfig()) {
                Serial.println("WiFi config saved, restarting...");

                ESP.restart(); // 重启应用新配置
            } else {
                Serial.println("Config timeout, retry...");
                currentState = STATE_WIFI_CHECK; // 超时后重试
            }
            break;
        }

        case STATE_TIME_SET:
            setupTimeManually();
            currentState = STATE_WEATHER;
            break;
        
        case STATE_INDOOR: {
            static unsigned long lastUpdateMs = 0;
            unsigned long indoorNow = millis();

            // Update time display (minute-level). Safe and cheap; redraws only when needed.
            if (!isFirstDraw) {
                updateTimeArea(dispSprite, &dLayout);
            }

            if (indoorNow - lastUpdateMs < UPDATE_INTERVAL) {
                // 非阻塞调度：尚未到更新时间则返回
                // updateGifArea();
                return;
            }

            lastUpdateMs = indoorNow;

            readSensorData(&sData);
            // 获取当前时间
            // DateTime currentDT = getDateAndTime();

            if(isFirstDraw){
                initDrawSensorData(&tft, &sData);
                isFirstDraw = false;
            } else {
            
                if (abs( sData.temp - sData.lastTemp) > TEMP_THRESHOLD ) {
                    updateSensorData(dispSprite, 1, sData.temp, &dLayout);
                    sData.lastTemp = sData.temp;
                }
                if (abs( sData.humi - sData.lastHumi) > HUMI_THRESHOLD ) {
                    updateSensorData(dispSprite, 2, sData.humi, &dLayout);
                    sData.lastHumi = sData.humi;
                }
                if (sData.co2 != sData.lastCO2) {
                    updateSensorData(dispSprite, 3, sData.co2, &dLayout);
                    sData.lastCO2 = sData.co2;
                }
            }

            // Ensure time stays updated even if sensor values don't change
            updateTimeArea(dispSprite, &dLayout);
            // updateGifArea();
            break;
        }

        case STATE_WEATHER: {
            // 目标：
            // 1) 上电后 WiFi 连接成功 -> 立刻请求一次天气并显示
            // 2) 后续每小时在整点后 0~5 分钟窗口内最多请求一次

            static bool ntpStarted = false;
            static bool bootFetched = false;
            static uint32_t bootFetchMs = 0;
            static bool timeValidEver = false;
            // day*48 + hour*2 + slot(0=00min,1=30min)
            static int32_t lastFetchSlotId = -1;

            const unsigned long nowMs = millis();

            if (!ntpStarted) {
                setupNtp();
                ntpStarted = true;
            }

            // 读取时间（用于整点窗口调度）
            time_t now;
            time(&now);
            const bool timeValid = (now > 1700000000);
            struct tm t;
            if (timeValid) {
                localtime_r(&now, &t);
            }

            // Compute whether we should fetch weather, but DEFER the actual HTTP call
            // until after UI/time updates to avoid visible time lag during blocking IO.
            bool doBootFetch = (!bootFetched);
            bool doScheduledFetch = false;

            // 当时间第一次变为有效：如果刚刚已经做过开机拉取，则把“本小时已拉取”标记上，避免立刻重复。
            if (timeValid && !timeValidEver) {
                timeValidEver = true;
                if (!doBootFetch && (uint32_t)(nowMs - bootFetchMs) < 10UL * 60UL * 1000UL) {
                    const int slotNow = (t.tm_min >= 30) ? 1 : 0;
                    lastFetchSlotId = (int32_t)t.tm_yday * 48 + (int32_t)t.tm_hour * 2 + slotNow;
                }
            }

            // 每半小时请求一次：在 00:00~00:04 或 00:30~00:34 窗口内最多请求一次
            if (timeValid) {
                const bool inWindow00 = (t.tm_min >= 0 && t.tm_min < 5);
                const bool inWindow30 = (t.tm_min >= 30 && t.tm_min < 35);
                if (inWindow00 || inWindow30) {
                    const int slot = inWindow30 ? 1 : 0;
                    const int32_t slotId = (int32_t)t.tm_yday * 48 + (int32_t)t.tm_hour * 2 + slot;
                    if (slotId != lastFetchSlotId) {
                        doScheduledFetch = true;
                        lastFetchSlotId = slotId;
                    }
                }
            }

            // Update time display ASAP (minute-level). This should run even if we skip UI updates.
            if (!isFirstDraw) {
                updateTimeArea(dispSprite, &dLayout);
            }

            // UI 刷新节流（显示更新频率仍由 UPDATE_INTERVAL 控制）
            static unsigned long lastUiUpdateMs = 0;
            if (nowMs - lastUiUpdateMs < UPDATE_INTERVAL) {
                break;
            }
            lastUiUpdateMs = nowMs;

            // If in indoor view, update indoor sensor readings on the same cadence
            if (weatherViewMode == VIEW_INDOOR) {
                readSensorData(&sData);
            }

            if(isFirstDraw){
                SensorData drawData = sData;
                if (weatherViewMode == VIEW_OUTDOOR) {
                    drawData.temp = sData.outdoorTemp;
                    drawData.humi = sData.outdoorHumi;
                    // drawData.co2 is not used in outdoor view (3rd slot overwritten by wind)
                }
                initDrawSensorData(&tft, &drawData);

                if (weatherViewMode == VIEW_OUTDOOR) {
                    // Weather page: show wind level in the 3rd slot
                    updateWeatherWind(dispSprite, sData.windLevel, &dLayout);
                    // Weather page: show weather icon at top-right
                    updateWeatherIcon(dispSprite, sData.weatherText);
                } else {
                    // Indoor view: show indoor icon at the same top-right position
                    updateIndoorIcon(dispSprite);
                }
                isFirstDraw = false;
            } else {

                if (weatherViewMode == VIEW_OUTDOOR) {
                    if (abs(sData.outdoorTemp - sData.lastTemp) > TEMP_THRESHOLD) {
                        updateSensorData(dispSprite, 1, sData.outdoorTemp, &dLayout);
                        sData.lastTemp = sData.outdoorTemp;
                    }
                    if (abs(sData.outdoorHumi - sData.lastHumi) > HUMI_THRESHOLD) {
                        updateSensorData(dispSprite, 2, sData.outdoorHumi, &dLayout);
                        sData.lastHumi = sData.outdoorHumi;
                    }

                    // Weather page: show wind level (0~12)
                    if (sData.windLevel != sData.lastWindLevel) {
                        updateWeatherWind(dispSprite, sData.windLevel, &dLayout);
                        sData.lastWindLevel = sData.windLevel;
                    }

                    if (sData.weatherText != sData.lastWeatherText) {
                        updateWeatherIcon(dispSprite, sData.weatherText);
                        sData.lastWeatherText = sData.weatherText;
                    }
                } else {
                    if (abs(sData.temp - sData.lastTemp) > TEMP_THRESHOLD) {
                        updateSensorData(dispSprite, 1, sData.temp, &dLayout);
                        sData.lastTemp = sData.temp;
                    }
                    if (abs(sData.humi - sData.lastHumi) > HUMI_THRESHOLD) {
                        updateSensorData(dispSprite, 2, sData.humi, &dLayout);
                        sData.lastHumi = sData.humi;
                    }
                    if (sData.co2 != sData.lastCO2) {
                        updateSensorData(dispSprite, 3, sData.co2, &dLayout);
                        sData.lastCO2 = sData.co2;
                    }

                    // Indoor view: ensure icon stays as indoor
                    updateIndoorIcon(dispSprite);
                }
            }

            // Minute-level refresh (covers cases where only time changes)
            updateTimeArea(dispSprite, &dLayout);

            // Perform deferred fetches after UI updates
            if (doBootFetch) {
                fetchWeather(&sData);
                bootFetched = true;
                bootFetchMs = nowMs;
            } else if (doScheduledFetch) {
                fetchWeather(&sData);
            }
            break;

        }
    }
    // update gif animation area (non-blocking) — can run frequently now
    // updateGifArea();
    delay(10);
}

void setupTimeManually() {
    return;
}


// ====================== 10. 断电/重启时的清理（可选） ======================
void cleanupOnExit() {
    safeDeleteSprite(dispSprite);
    SPIFFS.end();
}

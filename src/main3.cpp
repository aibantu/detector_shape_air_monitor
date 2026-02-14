// #include <Arduino.h>
// #include <TFT_eSPI.h>

// bool isFirstDraw = true; 

// TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite *sensorSprite = nullptr;

// void drawSensorData();

// void setup() {
//     Serial.begin(9600); // 提升波特率，避免串口乱码（9600易卡顿）
//     delay(500); // 延长初始化等待，适配屏幕上电

//     // ========== 1. 校验屏幕初始化 ==========
//     tft.init();
//     tft.setRotation(1); // 根据屏幕实际方向调整（0/1/2/3）

//     // 打印屏幕参数，验证硬件通信
//     uint16_t tftW = tft.width();
//     uint16_t tftH = tft.height();
//     // tft.setViewport(0, 0, tftW, tftH);
//     tft.fillScreen(TFT_BLACK);

//     Serial.print("屏幕参数：width=");
//     Serial.print(tftW);
//     Serial.print(", height=");
//     Serial.println(tftH);
//     // 校验屏幕尺寸有效性（避免0值）
//     if (tftW == 0 || tftH == 0) {
//         Serial.println("屏幕尺寸无效！");
//         while (1);
//     }

//     // ========== 2. 安全初始化Sprite ==========
//     sensorSprite = new TFT_eSprite(&tft);
//     if (sensorSprite == nullptr) {
//         Serial.println("Sprite创建失败（内存不足）！");
//         while (1);
//     }
//     // 校验Sprite创建（传入有效尺寸）
//     bool spriteCreated = sensorSprite->createSprite(tftW, tftH);
//     if (!spriteCreated) {
//         Serial.println("Sprite画布创建失败！");
//         delete sensorSprite; // 释放无效内存
//         sensorSprite = nullptr;
//         while (1);
//     }
//     // Sprite基础配置（必须在createSprite之后）
//     sensorSprite->setColorDepth(16); // 16位色（ST7789推荐）
//     sensorSprite->fillSprite(TFT_BLACK); // 清空背景
//     sensorSprite->setTextColor(TFT_WHITE, TFT_BLACK); // 文字白+背景黑（避免透明）
//     sensorSprite->setTextDatum(TL_DATUM); // 默认左上锚点
//     Serial.println("Sprite初始化完成");
// }

// void loop() {
//     if (isFirstDraw) {
//         drawSensorData();
//         isFirstDraw = false;
//         return;
//     }
//     delay(2000);
// }

// void drawSensorData() {
//     // 前置校验：Sprite必须有效
//     if (sensorSprite == nullptr) {
//         Serial.println("Sprite为空，无法绘制！");
//         return;
//     }

//     // 清空Sprite画布
//     uint16_t w = sensorSprite->width();
//     uint16_t h = sensorSprite->height();

//     // 使用最简单可靠的渲染路径：setCursor + print
//     sensorSprite->setTextWrap(false);
//     sensorSprite->setTextFont(2);       // GLCD 基本字体
//     sensorSprite->setTextSize(2);       // 稍大字号，保证可见
//     sensorSprite->setTextColor(TFT_WHITE, TFT_BLACK);

//     // 清屏并画一条参考线
//     sensorSprite->fillSprite(TFT_BLACK);
//     sensorSprite->drawFastHLine(0, 40, w, TFT_WHITE);

//     // 标题
//     // sensorSprite->setCursor(10, 10);
//     sensorSprite->drawString("Time", 20, 10);

//     // 温度

//     sensorSprite->setCursor(10, 50);
//     sensorSprite->print("Temp: ");
//     sensorSprite->print(" C"); // 显示摄氏度符号（若不兼容可改为 ' C'）

//     // 湿度

//     sensorSprite->setCursor(10, 80);
//     sensorSprite->print("Humi: ");
//     // sensorSprite->print(String(sData.humi, 1));
//     sensorSprite->print(" %");

//     // ========== 强制推送Sprite到屏幕 ==========
//     sensorSprite->pushSprite(0, 0); // 从屏幕左上角开始显示
//     Serial.println("文字绘制完成，已推送至屏幕");
// }
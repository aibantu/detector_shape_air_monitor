// // #include <Arduino.h>
// #include <TFT_eSPI.h>
// #include <DHT.h>
// #include <SPIFFS.h>

// #define DHTPIN          2     // DHT 传感器连接引脚
// #define DHTTYPE         DHT22 // DHT 22 (AM2302), AM232
// #define UPDATE_INTERVAL 2000    // 数据更新时间间隔（ms）
// #define TEMP_THRESHOLD  0.1
// #define HUMI_THRESHOLD  0.1
// #define LINE_NUM        5
// #define FONT_ID         4
// #define CIRCLE_RADIUS   3
// #define BAUD_RATE       9600
// // #define HEAD_INIT_X     5


// struct SensorData {
//     float temp = 0.0;
//     float humi = 0.0;
//     float lastTemp = 0.0;
//     float lastHumi = 0.0;
// } sData;

// struct DrawConfig {
//     int16_t headX;
//     int16_t dataX;
//     int16_t tempY;
//     int16_t humiY;
//     int16_t dataW;
//     int16_t dataH;
//     int16_t unitW;
//     int16_t gifW;
// } drawCfg = {5, 0, 0, 0, 0, 0, 0, 100};

// // GIF animation state (right-bottom area)
// int gifX = 0;
// int gifY = 0;
// int gifW = 100;
// uint32_t gifLastMs = 0;
// uint16_t gifFrame = 0;
// // GIF frame interval in milliseconds. Lower = faster playback.
// uint16_t gifFrameInterval = 50; // 50 ms per frame ~20 FPS
// int gifFrameCount = 0;

// // Scan SPIFFS for raw frames named frame000.raw ... frameNNN.raw under /res/frames/
// int scanFrameFiles() {
//     const int maxFrames = 500;
//     char path[64];
//     int count = 0;
//     for (int i = 0; i < maxFrames; ++i) {
//         snprintf(path, sizeof(path), "/res/frames/frame%03d.raw", i);
//         if (SPIFFS.exists(path)) count++;
//         else break;
//     }
//     return count;
// }

// DHT dht(DHTPIN, DHTTYPE); // 初始化 DHT 传感器
// TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite *sensorSprite = nullptr;

// bool isFirstDraw = true; 

// // int16_t headX = 5;
// // int16_t dataX = 0;
// // int16_t dataW = 0;
// // int16_t unitW = 0;
// // int16_t gifW = 100;
// // unsigned long lastUpdateTime = 0;
// // const unsigned long DEBOUNCE_TIME = 500;

// void readSensorData();
// void initDrawSensorData();
// void updateSensorData(int id);
// void calcLayoutParams();
// void safeDeleteSprite();
// void cleanupOnExit();


// void setup() {
//     Serial.begin(BAUD_RATE); // 提升波特率，避免串口卡顿
//     delay(200);

//     atexit(cleanupOnExit);  // 添加此行代码

//     tft.init();
//     tft.setRotation(TFT_ROTATION); // 设置屏幕旋转角度
//     tft.fillScreen(TFT_BLACK);

//     // 串口输出屏幕参数，验证配置是否正确
//     Serial.print("width:");
//     Serial.println(tft.width());
//     Serial.print("height:");
//     Serial.println(tft.height());

//     // int w = tft.width();

//     sensorSprite = new TFT_eSprite(&tft);
//     if(!sensorSprite) {
//         Serial.println("Failed to create sprite!");
//         while(1);
//     }
//     sensorSprite->setColorDepth(16);
//     sensorSprite->createSprite(tft.width(), tft.height());
//     sensorSprite->fillSprite(TFT_BLACK);
//     sensorSprite->setTextColor(TFT_WHITE, TFT_BLACK);
//     sensorSprite->setTextFont(FONT_ID);

//     // 挂载 SPIFFS（用于读取 res 下的图片文件，注意：需将文件上传到 SPIFFS/ LittleFS）
//     if (!SPIFFS.begin(true)) {
//         Serial.println("⚠️ SPIFFS mount failed");
//     } else {
//         Serial.println("SPIFFS mounted");
//     }

//     // scan frames in SPIFFS (data/res/frames/frame000.raw ...)
//     gifFrameCount = scanFrameFiles();
//     Serial.print("Found frame count: "); Serial.println(gifFrameCount);

//     // unitW = sensorSprite->textWidth("ppm", FONT_ID);
//     // drawPos.valueW = sensorSprite->textWidth("1000", FONT_ID);
//     // drawPos.valueH = sensorSprite->fontHeight(FONT_ID);
//     // drawPos.valueX = w - gifW - unitW - drawPos.valueW;

//     dht.begin();

//     Serial.print("Init complete!");
// }

// // 非破坏性地更新 GIF 区域：优先尝试集成 GIF 解码库（若可用），否则使用模拟动画
// void updateGifArea() {
//     if (!sensorSprite) return;
//     uint32_t now = millis();
//     if (now - gifLastMs < gifFrameInterval) return;
//     gifLastMs = now;

// #if __has_include(<AnimatedGIF.h>)
//     // 如果系统中有 AnimatedGIF 库，可以在这里集成并解码帧到 sensorSprite 的区域
//     // 这里保留占位：实际库接入需要实现回调并在解码时把像素写入 sprite
//     // For now fall back to placeholder if library present but not yet wired.
// #endif
//     // If we have pre-split frames in SPIFFS, play them; otherwise fallback to simulated animation
//     if (gifFrameCount > 0) {
//         // play next raw frame from SPIFFS
//         // implemented in helper below
//         // playNextFrame will advance gifFrame
//         // and update only the gif region
        
//         // call playback helper
//         // (implemented further down in file)
        
//         // We call directly here
//         {
//             // forward-declared helper (defined below)
//             extern void playNextFrame();
//             playNextFrame();
//         }
//         return;
//     }

//     // fallback: simple non-blocking simulated animation (moving dot + color cycle)
//     gifFrame++;
//     int localW = gifW;
//     int cx = gifX + (gifFrame % (localW - 10)) + 5;
//     int cy = gifY + (gifFrame * 3 % (localW - 10)) + 5;

//     // draw background for gif region (slightly darker)
//     uint16_t bgCol = tft.color565(32, 32, 32);
//     sensorSprite->fillRect(gifX, gifY, localW, localW, bgCol);

//     // draw shifting colored circles to simulate frames
//     uint8_t r = (gifFrame * 7) & 0xFF;
//     uint8_t g = (gifFrame * 13) & 0xFF;
//     uint8_t b = (gifFrame * 19) & 0xFF;
//     uint16_t col = tft.color565(r, g, b);
//     sensorSprite->fillCircle(cx, cy, 12, col);

//     // small frame index text
//     sensorSprite->setTextDatum(TR_DATUM);
//     sensorSprite->setTextFont(1);
//     sensorSprite->setTextSize(1);
//     sensorSprite->setTextColor(TFT_WHITE, bgCol);
//     sensorSprite->drawString(String(gifFrame % 100), gifX + localW - 4, gifY + localW - 4);

//     // push only gif region
//     sensorSprite->pushSprite(gifX, gifY, gifX, gifY, localW, localW);
// }

// // Read a single raw RGB565 frame from SPIFFS and display it in the gif region.
// // Frame files must be named /res/frames/frameNNN.raw and be gifW x gifW pixels,
// // 16-bit RGB565 little-endian.
// void playNextFrame() {
//     if (gifFrameCount <= 0) return;

//     char path[64];
//     snprintf(path, sizeof(path), "/res/frames/frame%03d.raw", gifFrame);
//     File f = SPIFFS.open(path, "r");
//     if (!f) {
//         Serial.print("Failed to open frame: "); Serial.println(path);
//         // advance to next to avoid stuck loop
//         gifFrame = (gifFrame + 1) % gifFrameCount;
//         return;
//     }

//     size_t expect = (size_t)gifW * (size_t)gifW * 2;
//     uint8_t *buf = (uint8_t*)malloc(expect);
//     if (!buf) {
//         Serial.println("Out of memory for frame buffer");
//         f.close();
//         return;
//     }

//     size_t readBytes = f.read(buf, expect);
//     f.close();
//     if (readBytes != expect) {
//         Serial.print("Frame read size mismatch: "); Serial.print(readBytes); Serial.print("/" ); Serial.println(expect);
//         free(buf);
//         gifFrame = (gifFrame + 1) % gifFrameCount;
//         return;
//     }

//     // Cast to 16-bit pixel array (little-endian as written by splitter)
//     uint16_t *pix = (uint16_t*)buf;

//     // Push the image into the sprite memory at gifX,gifY
//     sensorSprite->pushImage(gifX, gifY, gifW, gifW, pix);

//     // Push only that region to the TFT
//     sensorSprite->pushSprite(gifX, gifY, gifX, gifY, gifW, gifW);

//     free(buf);

//     gifFrame = (gifFrame + 1) % gifFrameCount;
// }

// unsigned long lastSensorMs = 0;

// void loop() {
//     unsigned long now = millis();

//     // first-time initialization
//     if(isFirstDraw){
//         // ensure we have at least one sensor read before drawing
//         readSensorData();
//         initDrawSensorData();
//         isFirstDraw = false;
//         lastSensorMs = now;
//     }

//     // sensor updates are timed separately so we don't block the main loop
//     if (now - lastSensorMs >= UPDATE_INTERVAL) {
//         readSensorData();

//         if (abs( sData.temp - sData.lastTemp) > TEMP_THRESHOLD ) {
//             updateSensorData(1);
//             sData.lastTemp = sData.temp;
//         }
//         if (abs( sData.humi - sData.lastHumi) > HUMI_THRESHOLD ) {
//             updateSensorData(2);
//             sData.lastHumi = sData.humi;
//         }

//         lastSensorMs = now;
//     }

//     // regular push of the main sprite (other regions updated as needed)
//     sensorSprite->pushSprite(0, 0);

//     // update gif animation area (non-blocking) — can run frequently now
//     updateGifArea();
// }

// void calcLayoutParams(int16_t w, int16_t h) {

//     int16_t headWMin = sensorSprite->textWidth("TEMP:", FONT_ID);
//     int16_t dataWMin = sensorSprite->textWidth("9999", FONT_ID);
//     int16_t unitWMin = sensorSprite->textWidth(" ppm", FONT_ID);

//     int16_t remain = w - headWMin - dataWMin - unitWMin - drawCfg.headX;

//     if (remain <= 0) {
//         Serial.println("Not enough space for drawing");
//         while(1);
//     } 
    
//         drawCfg.gifW = (remain < drawCfg.gifW) ? remain : drawCfg.gifW;
//         // store gif region for animator
//         gifX = w - drawCfg.gifW - 2; // 右下角留2像素边距
//         gifY = h - drawCfg.gifW - 2;
//     remain -= drawCfg.gifW;

//     int16_t gap = remain / 4;

//     drawCfg.headX +=  gap;
//     drawCfg.dataX = drawCfg.headX + headWMin + gap;
//     drawCfg.dataW = dataWMin + gap;
//     drawCfg.unitW = unitWMin + gap;
// }

// void readSensorData(){
//     sData.lastTemp = sData.temp;
//     sData.lastHumi = sData.humi;
    
//     sData.temp = dht.readTemperature();
//     sData.humi = dht.readHumidity();

//     // 检查读取是否成功
//     if (isnan(sData.temp) || isnan(sData.humi)) {
//         Serial.println("Failed to read from DHT sensor!");
//         sData.temp = sData.lastTemp;
//         sData.humi = sData.lastHumi;
//         return;
//     }
// }


// void updateSensorData(int id) {

//     if (!sensorSprite) return ;
    
//     char valueBuf[10];
//     int16_t targetY = 0;
//     uint16_t color = TFT_WHITE;

//     switch(id) {
//       case 1: // 温度更新
//         color = TFT_YELLOW;
//         sprintf(valueBuf, "%.1f", sData.temp);
//         // valueStr = String(sData.temp, 1);
//         targetY = drawCfg.tempY;
//         break;
//       case 2: // 湿度更新
//         color = TFT_GREEN;
//         sprintf(valueBuf, "%.1f", sData.humi);
//         // valueStr = String(sData.humi, 1);
//         targetY = drawCfg.humiY;
//         break;
//       default:
//         return;
//     }

//     sensorSprite->fillRect(drawCfg.dataX, targetY, drawCfg.dataW, drawCfg.dataH, TFT_BLACK);
//     sensorSprite->setTextColor(color);
//     sensorSprite->setCursor(drawCfg.dataX + drawCfg.dataW - sensorSprite->textWidth(valueBuf, FONT_ID), targetY);
//     sensorSprite->print(valueBuf);
//     sensorSprite->pushSprite(drawCfg.dataX - 2, targetY - 2, drawCfg.dataX - 2, targetY - 2, drawCfg.dataW + 4, drawCfg.dataH + 4);
// }

// void initDrawSensorData() {

//     if(!sensorSprite) return ;

//     uint16_t w = sensorSprite->width();
//     uint16_t h = sensorSprite->height();

//     calcLayoutParams(w, h);
    
//     int16_t fistLineH = sensorSprite->fontHeight(FONT_ID);
//     drawCfg.dataH = fistLineH;
//     int16_t segH = h - fistLineH - drawCfg.dataH * (LINE_NUM - 2) - 2;
//     int16_t lineSpacing = segH / LINE_NUM;

//     // 绘制标题
//     uint16_t x = w / 2;
//     uint16_t y = lineSpacing;
//     sensorSprite->setTextDatum(TC_DATUM);
//     sensorSprite->setTextColor(TFT_WHITE);
//     sensorSprite->drawString("Time", x, y);

//     // 绘制全屏分割线
//     y += fistLineH + lineSpacing / 2;
//     sensorSprite->drawFastHLine(0, y, w, TFT_WHITE);
//     sensorSprite->drawFastHLine(0, y + 1, w, TFT_WHITE);
    
//     uint16_t unitXBase = w - drawCfg.gifW - drawCfg.unitW;

//     // 绘制温度行
//     y += lineSpacing + 2;
//     drawCfg.tempY = y;

//     sensorSprite->setTextColor(TFT_YELLOW);

//     sensorSprite->setTextDatum(TL_DATUM);
//     sensorSprite->drawString("Temp: " , drawCfg.headX, y);
//     sensorSprite->drawCircle(unitXBase +  CIRCLE_RADIUS + 5, y + CIRCLE_RADIUS, CIRCLE_RADIUS, TFT_YELLOW);
//     sensorSprite->drawString(" C", unitXBase + 2 * CIRCLE_RADIUS, y); // 温度单位

//     sensorSprite->setTextDatum(TR_DATUM);
//     sensorSprite->drawString(String(sData.temp, 1), unitXBase, y); // 温度数值

//     // 绘制湿度行
//     y += drawCfg.dataH + lineSpacing;
//     drawCfg.humiY = y; // 动态赋值湿度行Y坐标（关键）

//     sensorSprite->setTextColor(TFT_GREENYELLOW);

//     sensorSprite->setTextDatum(TL_DATUM);
//     sensorSprite->drawString("Humi: ", drawCfg.headX, y);
//     sensorSprite->drawString(" %", unitXBase, y); // 湿度单位
    
//     sensorSprite->setTextDatum(TR_DATUM);
//     sensorSprite->drawString(String(sData.humi, 1), unitXBase, y); // 湿度数值

//     // 绘制CO2行
//     y += drawCfg.dataH + lineSpacing;
//     sensorSprite->setTextColor(TFT_GREEN);

//     sensorSprite->setTextDatum(TL_DATUM);
//     sensorSprite->drawString("CO2: ", drawCfg.headX, y);
//     sensorSprite->drawString(" ppm", unitXBase, y); // CO2单位
//     sensorSprite->setTextDatum(TR_DATUM);
//     sensorSprite->drawString("1234", unitXBase, y);


//     // 首次绘制必须推送屏幕
//     sensorSprite->pushSprite(0, 0);
// }

// // ====================== 9. 内存管理（新增：安全释放Sprite） ======================
// void safeDeleteSprite() {
//     if (sensorSprite) {
//         sensorSprite->deleteSprite();
//         delete sensorSprite;
//         sensorSprite = nullptr;
//     }
// }

// // ====================== 10. 断电/重启时的清理（可选） ======================
// void cleanupOnExit() {
//     safeDeleteSprite();
// }
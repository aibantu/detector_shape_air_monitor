// // #include <Arduino.h>
// #include <TFT_eSPI.h>
// #include <DHT.h>

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
// } drawCfg = {5, 0, 0, 0, 0, 0, 0, 80};

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
// void safeDeletSprite();

// void setup() {
//     Serial.begin(BAUD_RATE); // 提升波特率，避免串口卡顿
//     delay(200);

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

//     // unitW = sensorSprite->textWidth("ppm", FONT_ID);
//     // drawPos.valueW = sensorSprite->textWidth("1000", FONT_ID);
//     // drawPos.valueH = sensorSprite->fontHeight(FONT_ID);
//     // drawPos.valueX = w - gifW - unitW - drawPos.valueW;

//     dht.begin();

//     Serial.print("Init complete!");
// }

// void loop() {
//     readSensorData();

//     if(isFirstDraw){
//         initDrawSensorData();
//         isFirstDraw = false;
//         return;
//     } 

//     // unsigned long currentTime = millis();
    
//     if (abs( sData.temp - sData.lastTemp) > TEMP_THRESHOLD ) {
//         updateSensorData(1);
//         sData.lastTemp = sData.temp;
//     }
//     if (abs( sData.humi - sData.lastHumi) > HUMI_THRESHOLD ) {
//         updateSensorData(2);
//         sData.lastHumi = sData.humi;
//     }

//     sensorSprite->pushSprite(0, 0);
//     delay(UPDATE_INTERVAL); // 每2秒更新一次数据
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
    
//     drawCfg.gifW = (remain <drawCfg.gifW) ? remain : drawCfg.gifW;
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

// //     sensorSprite->fillRect(drawPos.valueX, targetY, drawPos.valueW, drawPos.valueH, TFT_BLACK);
// //     sensorSprite->setTextColor(color);
// //     sensorSprite->setCursor(drawPos.valueX + drawPos.valueW - sensorSprite->textWidth(valueBuf, FONT_ID), targetY);
// //     sensorSprite->print(valueBuf);
// //     sensorSprite->pushSprite(drawPos.valueX - 2, targetY - 2, drawPos.valueX - 2, targetY - 2, drawPos.valueW + 4, drawPos.valueH + 4);

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
// void end() {
//     safeDeleteSprite();
// }
// #include <Arduino.h>
// #include <TFT_eSPI.h>
// #include <RoboEyes.h>
// #include <DHT.h>
// #include "screenDisplay.h"
// // #include "parameters.h"

// #define DHTPIN 2     // DHT 传感器连接引脚
// #define DHTTYPE DHT22 // DHT 22 (AM2302), AM232

// const int SCREEN_WIDTH = 172;  // 屏幕宽度
// const int SCREEN_HEIGHT = 320; // 屏幕高度
// const int SCREEN_ROTATION = 1; // 屏幕旋转角度

// const int EYE_AREA_W = 250;  // 眼睛显示区域宽度
// const int EYE_AREA_H = 160;  // 眼睛显示区域高度

// SystemState currentState = STATE_EYE_ANIM; // 初始状态为眼睛动画状态

// unsigned long stateTimer = 0; // 状态定时器
// float temp = 0.0;
// float humi = 0.0;

// DHT dht(DHTPIN, DHTTYPE); // 初始化 DHT 传感器

// TFT_eSPI tft = TFT_eSPI();

// TFT_eSprite *eyeSprite = nullptr;

// TFT_eSprite *sensorSprite = nullptr;

// RoboEyes *roboEyes = nullptr; 

// void readSensorData();
// void switchState(SystemState newState);

// // TFT_eSprite ballSprite = TFT_eSprite(&tft);  // 创建精灵对象
// void setup() {
//   Serial.begin(9600);
//   delay(200);

//   tft.init();
//   tft.setRotation(SCREEN_ROTATION);
//   tft.fillScreen(TFT_BLACK);

//   // 串口输出屏幕参数，验证配置是否正确
//   Serial.print("width:");
//   Serial.println(tft.width());  // 应输出 320
//   Serial.print("height:");
//   Serial.println(tft.height()); // 应输出 172

//   // Create sprites for sensor and eyes
//   sensorSprite = new TFT_eSprite(&tft);
//   sensorSprite->setColorDepth(16);
//   sensorSprite->createSprite(tft.width(), tft.height());
//   sensorSprite->fillSprite(TFT_BLACK);

//   eyeSprite = new TFT_eSprite(&tft);
//   // eyeSprite->fillScreen(TFT_BLACK);
  
//   // Initialize RoboEyes with a valid sprite reference
//   roboEyes = new RoboEyes(*eyeSprite, (tft.width() - EYE_AREA_W) / 2, (tft.height() - EYE_AREA_H) / 2, EYE_AREA_W, EYE_AREA_H);
//   roboEyes->begin(50); 
//   roboEyes->setDisplayColors(TFT_BLACK, TFT_GREEN);  // 背景色=黑色，眼睛色=白色
//   roboEyes->setAutoblinker(true, 3, 2);             // 自动眨眼：间隔3秒±2秒（随机波动）
//   roboEyes->setIdleMode(true, 2, 1);                // 空闲模式：2秒±1秒随机移动眼睛
//   // roboEyes.setCuriosity(true);                     // 好奇模式：眼睛看向边缘时变大
//   // roboEyes.setCyclops(true);                     // 开启独眼模式（默认关闭，按需启用）
//   // roboEyes.setSweat(true);                       // 开启流汗动画（默认关闭，按需启用）
  
//   roboEyes->setMood(ROBO_DEFAULT);  // 正常表情（可选：ROBO_TIRED/ROBO_ANGRY/ROBO_HAPPY）
  
//   Serial.println("RoboEyes initialized!");

//   dht.begin(); // 初始化 DHT 传感器

//   Serial.println("DHT initialized!");

//   stateTimer = millis();

// }

// void loop() {

//    // 状态切换计时逻辑
//   if (millis() - stateTimer >= 10000 ) {
//     // 切换到另一个状态
//     SystemState newState = (currentState == STATE_EYE_ANIM) ? STATE_SENSOR_DISPLAY : STATE_EYE_ANIM;
//     switchState(newState);
//     stateTimer = millis(); // 重置定时器
//     Serial.print("Switched to state: ");
//     Serial.println(newState == STATE_EYE_ANIM ? "EYE_ANIM" : "SENSOR_DISPLAY");
//   }

//   // 执行当前状态的逻辑
//   switch (currentState) {
//     case STATE_EYE_ANIM:
//       drawEyes(); // 更新眼睛动画
//       break;
//     case STATE_SENSOR_DISPLAY:
//       readSensorData();   // 读取传感器数据
//       drawSensorData();   // 显示温湿度
//       break;
//   }

//   // roboEyes->update();

//   // static unsigned long moodTimer = 0;
//   // if (millis() - moodTimer >= 5000) {  // 5秒切换一次表情
//   //   static int currentMood = 0;
//   //   currentMood = (currentMood + 1) % 4;  // 0=正常，1=疲惫，2=愤怒，3=开心
    
//   //   switch (currentMood) {
//   //     case 0:
//   //       roboEyes->setMood(ROBO_DEFAULT);
//   //       Serial.println("Mood: Default");
//   //       break;
//   //     case 1:
//   //       roboEyes->setMood(ROBO_TIRED);
//   //       Serial.println("Mood: Tired");
//   //       break;
//   //     case 2:
//   //       roboEyes->setMood(ROBO_ANGRY);
//   //       Serial.println("Mood: Angry");
//   //       break;
//   //     case 3:
//   //       roboEyes->setMood(ROBO_HAPPY);
//   //       Serial.println("Mood: Happy");
//   //       break;
//   //   }
    
//     // moodTimer = millis();  // 重置计时器
//   // }
  
// }
//   // put your main code here, to run repeatedly:
//   // // 1. 清空精灵（黑色背景）
//   // tft.fillRect(ballX,ballY,20,20,TFT_BLACK);
//   // ballSprite.fillSprite(TFT_BLACK);

// /**
//  * @brief 切换系统状态（核心逻辑：资源清理+新状态初始化）
//  * @param newState 目标状态
//  */
// void switchState(SystemState newState) {

//   currentState = newState;
//   tft.fillScreen(TFT_BLACK);

//   if (currentState == STATE_EYE_ANIM) {
//     eyeSprite->fillSprite(TFT_BLACK);
//     drawEyes();
//   } else {
//     sensorSprite->fillSprite(TFT_BLACK);
//     drawSensorData();
//   }

// }

// // /**
// //  * @brief 清理当前状态的资源（防止内存泄漏）
// //  */
// // void cleanupSpirteResources() {

// //   // 销毁精灵实例（如果存在）
// //   if (sprite != nullptr) {
// //     sprite->deleteSprite(); // 释放精灵显存
// //     delete sprite;          // 释放精灵对象内存
// //     sprite = nullptr;
// //   }
// // }

// void readSensorData(){
//   // 读取温湿度数据
//   temp = dht.readTemperature();
//   humi = dht.readHumidity();

//   // 检查读取是否成功
//   if (isnan(temp) || isnan(humi)) {
//     Serial.println("Failed to read from DHT sensor!");
//     return;
//   }

  
//   // Serial.print("Temperature: ");
//   // Serial.print(temp);
//   // Serial.print(" °C, Humidity: ");
//   // Serial.print(humi);
//   // Serial.println(" %");
// }
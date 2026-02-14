// #include <TFT_eSPI.h>
// #include <DHT.h>

// #include "project_config.h"
// #include "screenDisplay.h"
// #include "readSensorData.h"
// #include "WiFiManager.h"

// // 使用集中配置头文件中的常量
// #define GIF_BUFFER_PREALLOC 1  // 预分配帧缓冲区（避免频繁malloc）
// #define GIF_CACHE_FRAMES    2     // 缓存2帧数据（平衡内存/性能）
// #define GIF_FILE_PATH       "/res/358.gif"  // GIF文件路径（SPIFFS中）
// // #define HAS_ANIMATED_GIF

// DHT dht(DHTPIN, DHTTYPE); // 初始化 DHT 传感器

// HardwareSerial co2Serial(0);

// SensorData sData; // 传感器数据实例

// TFT_eSPI tft = TFT_eSPI();

// enum State {
//     STATE_WIFI_CHECK,
//     STATE_WIFI_CONFIG,
//     STATE_TIME_SET,
//     STATE_NORMAL_RUN
// };

// State currentState = STATE_WIFI_CHECK;


// // GIF animation state (right-bottom area)
// int gifX = 200;
// int gifY = 10;
// int gifW = 100;
// uint32_t gifLastMs = 0;
// uint16_t gifFrame = 0;
// uint16_t gifFrameInterval = 50; // 50 ms per frame ~20 FPS
// int gifFrameCount = -1; // 初始为-1表示未扫描，0表示无帧，正数表示有帧
// // GIF frame interval in milliseconds. Lower = faster playback.

// // ========== GIF 优化变量 ==========
// // 预分配帧缓冲区（静态内存，避免堆碎片）
// uint8_t *gifFrameBuffer = nullptr;
// // 帧缓存（缓存最近2帧，减少文件读取）
// uint8_t *gifCache[GIF_CACHE_FRAMES] = {nullptr};
// int gifCacheIndex[GIF_CACHE_FRAMES] = {-1}; // 缓存对应的帧编号

// // sensorSprite 与 drawCfg 由 screenDisplay 模块提供

// bool isFirstDraw = true; 

// // ========== GIF直解相关变量（仅在有库时生效） ==========
// #ifdef HAS_ANIMATED_GIF
// #include <AnimatedGIF.h>
// AnimatedGIF gifDecoder;        // GIF解码实例
// File gifFile;                  // GIF文件句柄
// bool isGifInitialized = false; // GIF解码器是否初始化完成
// bool isGifValid = false;       // GIF文件是否有效
// #endif

// // GIF绘制回调函数（仅在有库时编译）
// #ifdef HAS_ANIMATED_GIF
// void drawGifFrame(GIFDRAW *pDraw) {
//     if (!sensorSprite) return;

//     // 调色板（RGB565格式）
//     uint16_t *pColor = (uint16_t *)pDraw->pPalette;
//     // 像素索引数据
//     uint8_t *pPixels = pDraw->pPixels;
//     // 绘制起始坐标（叠加到原有GIF区域）
//     int x = gifX + pDraw->x;
//     int y = gifY + pDraw->y;

//     // 逐行绘制（仅绘制有效区域，跳过透明像素）
//     for (int row = 0; row < pDraw->iHeight; row++) {
//         for (int col = 0; col < pDraw->iWidth; col++) {
//             uint8_t pixelIdx = pPixels[col];
//             // 跳过透明像素（提升效率）
//             if (pixelIdx != pDraw->ucTransparent) {
//                 sensorSprite->drawPixel(x + col, y + row, pColor[pixelIdx]);
//             }
//         }
//         pPixels += pDraw->iWidth; // 移动到下一行
//     }

//     // 仅推送GIF区域到屏幕（复用原有优化逻辑）
//     sensorSprite->pushSprite(gifX, gifY, gifX, gifY, gifW, gifW);
// }
// #endif

// // GIF初始化函数（仅在有库时编译）
// #ifdef HAS_ANIMATED_GIF
// bool initGifDecoder() {
//     // 避免重复初始化
//     if (isGifInitialized) return isGifValid;

//     // 打开GIF文件
//     gifFile = SPIFFS.open(GIF_FILE_PATH, "r");
//     if (!gifFile) {
//         Serial.print("GIF file not found: "); Serial.println(GIF_FILE_PATH);
//         isGifInitialized = true;
//         isGifValid = false;
//         return false;
//     }

//     // 初始化解码器，绑定绘制回调
//     if (!gifDecoder.open(gifFile, drawGifFrame)) {
//         Serial.println("Failed to open GIF decoder");
//         gifFile.close();
//         isGifInitialized = true;
//         isGifValid = false;
//         return false;
//     }

//     // 设置循环播放（0=无限循环，1=播放1次）
//     gifDecoder.setLoop(0);
//     isGifInitialized = true;
//     isGifValid = true;
//     Serial.println("GIF decoder initialized successfully");
//     return true;
// }
// #endif


// int scanFrameFiles();
// void updateGifArea();
// void cleanupOnExit();
// void playNextFrame();
// void preallocGifBuffer(); // 预分配缓冲区
// void freeGifCache();      // 释放缓存


// void setup() {
//     Serial.begin(BAUD_RATE); // 提升波特率，避免串口卡顿
//     delay(200);

//     Serial.println("Setup start");

//     // 注册程序退出时的清理函数
//     atexit(cleanupOnExit);  // 添加此行代码

//     tft.init();
//     tft.setRotation(TFT_ROTATION); // 设置屏幕旋转角度
//     tft.fillScreen(TFT_BLACK);
//     // tft.fillRect(10, 10, 20, 20, TFT_RED);
//     // tft.fillRect(10, 40, 20, 20, TFT_GREEN);
//     // tft.fillRect(40, 10, 20, 20, TFT_BLUE);
//     // tft.fillRect(40, 40, 20, 20, TFT_YELLOW);
    

//     // 串口输出屏幕参数，验证配置是否正确
//     Serial.print("width:");
//     Serial.println(tft.width());
//     Serial.print("height:");
//     Serial.println(tft.height());

//     // 挂载 SPIFFS（用于读取 res 下的图片文件，注意：需将文件上传到 SPIFFS/ LittleFS）
//     if (!SPIFFS.begin(true)) {
//         Serial.println("SPIFFS mount failed");
//     } else {
//         Serial.println("SPIFFS mounted");

//         // scan frames in SPIFFS (data/res/frames/frame000.raw ...)
//         gifFrameCount = scanFrameFiles();
//         Serial.print("Found frame count: "); Serial.println(gifFrameCount);

//         preallocGifBuffer();

//     }

//     // if (!LittleFS.begin(true)) {
//     //     Serial.println("LittleFS mount failed");
//     // } else {
//     //     Serial.println("LittleFS mounted");
//     // }

//     dht.begin();

//     co2Serial.begin(9600, SERIAL_8N1, CO2_UART_RX, CO2_UART_TX);

//     Serial.print("Init complete!");
// }

// void loop() {
//     static unsigned long lastUpdateMs = 0;
//     unsigned long now = millis();
//     if (now - lastUpdateMs < UPDATE_INTERVAL) {
//         // 非阻塞调度：尚未到更新时间则返回
//         updateGifArea();
//         return;
//     }
//     lastUpdateMs = now;

//     readSensorData(&sData);

//     if(isFirstDraw){
//         initDrawSensorData(&tft, &sData);
//         isFirstDraw = false;
//     } else {
    
//         if (abs( sData.temp - sData.lastTemp) > TEMP_THRESHOLD ) {
//             updateSensorData(sensorSprite, 1, sData.temp, &dLayout);
//             sData.lastTemp = sData.temp;
//         }
//         if (abs( sData.humi - sData.lastHumi) > HUMI_THRESHOLD ) {
//             updateSensorData(sensorSprite, 2, sData.humi, &dLayout);
//             sData.lastHumi = sData.humi;
//         }
//         if (sData.co2 != sData.lastCO2) {
//             updateSensorData(sensorSprite, 3, sData.co2, &dLayout);
//             sData.lastCO2 = sData.co2;
//         }
//     }

//     // update gif animation area (non-blocking) — can run frequently now
//     updateGifArea();
// }

// // Scan SPIFFS for raw frames named frame000.raw ... frameNNN.raw under /res/frames/
// int scanFrameFiles() {
//     const int maxFrames = 500;
//     char path[64];
//     int count = 0;
//     for (int i = 0; i < maxFrames; ++i) {
//         snprintf(path, sizeof(path), "/res/frames/frame%03d.raw", i);
//         if (SPIFFS.exists(path)) count++;
//         // if (LittleFS.exists(path)) count++;
//         else {
//             if (count > 0) break;
//         }
//     }
//     return count;
// }

// // 非破坏性地更新 GIF 区域：优先尝试集成 GIF 解码库（若可用），否则使用模拟动画
// void updateGifArea() {
//     if (!sensorSprite || gifFrameCount < 0) return;

//     uint32_t now = millis();
//     if (now - gifLastMs < gifFrameInterval) return;

//     gifLastMs = now;

//     // #if __has_include(<AnimatedGIF.h>)
//     #ifdef HAS_ANIMATION_GIF
//         // 如果系统中有 AnimatedGIF 库，可以在这里集成并解码帧到 sensorSprite 的区域
//         // 这里保留占位：实际库接入需要实现回调并在解码时把像素写入 sprite
//         // For now fall back to placeholder if library present but not yet wired.
//         // 第一步：初始化GIF解码器（仅第一次执行）
//         if (initGifDecoder()) {
//             // 第二步：非阻塞播放下一帧
//             if (!gifDecoder.playFrame(false)) {
//                 // GIF播放结束，重新初始化实现循环
//                 gifFile.close();
//                 isGifInitialized = false;
//                 initGifDecoder();
//             }
//             return; // 直解成功，跳过后续拆分帧/模拟动画逻辑
//         }
//         // 若GIF直解失败，自动降级到原有逻辑
//     #endif
//     // If we have pre-split frames in SPIFFS, play them; otherwise fallback to simulated animation
//     if (gifFrameCount > 0) {
//         // play next raw frame from SPIFFS implemented in helper below
//         // playNextFrame will advance gifFrame and update only the gif region        
//         // call playback helper (implemented further down in file)       
//         // We call directly here
//         playNextFrame();
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
//     if (gifFrameCount <= 0 || !gifFrameBuffer || !sensorSprite) return;

//     char path[64];
//     snprintf(path, sizeof(path), "/res/frames/frame%03d.raw", gifFrame);

//     size_t expect = (size_t)gifW * gifW * 2;
    
//     int cacheHit = -1;
//     for (int i = 0; i < GIF_CACHE_FRAMES; i++) {
//         if (gifCacheIndex[i] == gifFrame) {
//             cacheHit = i;
//             break;
//         }
//     }

//     if (cacheHit >= 0 && gifCache[cacheHit]) {
//         sensorSprite->pushImage(gifX, gifY, gifW, gifW, (uint16_t*)gifCache[cacheHit]);
//     } else {
//         File f = SPIFFS.open(path, "r");
//         // File f = LittleFS.open(path, "r");
//         if (!f) {
//             Serial.print("Failed to open frame: "); Serial.println(path);
//             // advance to next to avoid stuck loop
//             gifFrame = (gifFrame + 1) % gifFrameCount;
//             return;
//         }
//         size_t readBytes = f.read(gifFrameBuffer, expect);
//         f.close();
//         if (readBytes != expect) {
//             Serial.print("Frame read size mismatch: "); Serial.print(readBytes); Serial.print("/" ); Serial.println(expect);
//             gifFrame = (gifFrame + 1) % gifFrameCount;
//             return;
//         }
//         static int cacheReplaceIdx = 0;
//         gifCacheIndex[cacheReplaceIdx] = gifFrame;
//         memcpy(gifCache[cacheReplaceIdx], gifFrameBuffer, expect);
//         cacheReplaceIdx = (cacheReplaceIdx + 1) % GIF_CACHE_FRAMES;
//         sensorSprite->pushImage(gifX, gifY, gifW, gifW, (uint16_t*)gifFrameBuffer);
//     }

//     // Push only that region to the TFT
//     sensorSprite->pushSprite(gifX, gifY, gifX, gifY, gifW, gifW);

//     gifFrame = (gifFrame + 1) % gifFrameCount;
// }

// void preallocGifBuffer() {
//     size_t frameSize = (size_t)gifW * gifW * 2;
//     // 预分配主缓冲区
//     if (!gifFrameBuffer) {
//         gifFrameBuffer = (uint8_t*)malloc(frameSize);
//         if (!gifFrameBuffer) {
//             Serial.println("Failed to prealloc gifFrameBuffer");
//         }
//     }
//     // 预分配缓存缓冲区
//     for (int i = 0; i < GIF_CACHE_FRAMES; i++) {
//         if (!gifCache[i]) {
//             gifCache[i] = (uint8_t*)malloc(frameSize);
//             if (!gifCache[i]) {
//                 Serial.printf("Failed to prealloc gifCache[%d]\n", i);
//             }
//             gifCacheIndex[i] = -1; // 初始化为无效帧
//         }
//     }    
// }


// // ========== 辅助函数：释放GIF缓存 ==========
// void freeGifCache() {
//     if (gifFrameBuffer) {
//         free(gifFrameBuffer);
//         gifFrameBuffer = nullptr;
//     }
//     for (int i = 0; i < GIF_CACHE_FRAMES; i++) {
//         if (gifCache[i]) {
//             free(gifCache[i]);
//             gifCache[i] = nullptr;
//             gifCacheIndex[i] = -1;
//         }
//     }
// }

// // ====================== 10. 断电/重启时的清理（可选） ======================
// void cleanupOnExit() {
//     safeDeleteSprite(sensorSprite);
//     freeGifCache();
//     // ========== 清理GIF解码器（仅在有库时生效） ==========
//     // #if __has_include(<AnimatedGIF.h>)
//     #ifdef HAS_ANIMATED_GIF
//     if (gifFile) {
//         gifFile.close();
//     }
//     gifDecoder.close();
//     #endif
//     SPIFFS.end();
// }

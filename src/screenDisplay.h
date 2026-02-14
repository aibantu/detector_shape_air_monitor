#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include <TFT_eSPI.h>
#include <SPIFFS.h>
#include "readSensorData.h"
#include "project_config.h"
#include "icons.h"

// struct DrawConfig {
// 	int16_t headX;
// 	int16_t dataX;
// 	int16_t tempY;
// 	int16_t humiY;
// 	int16_t co2Y;
// 	int16_t dataW;
// 	int16_t dataH;
// 	int16_t unitW;
// 	int16_t gifW;
// };

struct DisplayLayout {

	// 时间区域
	int16_t time_date_X;
	int16_t timeY;
	int16_t dateY;

	// 传感器数据区域
	int16_t sensorIconY;
	int16_t sensorDataY;
	int16_t sensorLableY;
	int16_t sensorDataH;
	int16_t sensorIcon;
	int16_t tempX;	// 靠左位置
	int16_t humiX;
	int16_t co2X;
	
	// GIF 区域
	int16_t gifX;
	int16_t gifY;
	int16_t gifW;
	int16_t gifH;
};

// 从 main.cpp 引用屏幕对象以创建 Sprite
// extern TFT_eSPI tft;

// extern TFT_eSprite *sensorSprite;
extern TFT_eSprite *dispSprite;

// extern DrawConfig drawCfg;
extern DisplayLayout dLayout;


// extern SensorData sData;

// void calcLayoutParams(int16_t w, int16_t h);
bool initSprite(TFT_eSPI* tft);
void drawDisplay(bool showQR = false, const String& text = "", int qrTextYOffset = 20);

void initDrawSensorData(TFT_eSPI* tft, const SensorData* initData);
void updateSensorData(TFT_eSprite* sprite, int id, float value, const DisplayLayout* dl);
void updateWeatherWind(TFT_eSprite* sprite, int windLevel, const DisplayLayout* dl);
void updateWeatherIcon(TFT_eSprite* sprite, const String& weatherText);
void updateIndoorIcon(TFT_eSprite* sprite);
void updateTimeArea(TFT_eSprite* sprite, const DisplayLayout* dl, bool force = false);
void safeDeleteSprite(TFT_eSprite* sprite); 
// 绘制电池状态（电压与充电状态）
void updateBatteryStatus(TFT_eSprite* sprite, const DisplayLayout* dl);
// int scanFrameFiles();
// void updateGifArea();
#endif // SCREEN_DISPLAY_H
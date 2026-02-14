#include <time.h>
#include "screenDisplay.h"
#include <Preferences.h> // 用于保存首次配置的时间（可选）
#include <string.h>
// ADC calibration
#include "esp_adc_cal.h"

// -------------------------- 配置项 --------------------------
// #define GMT_OFFSET_SEC  8 * 3600  // 东八区（GMT+8）
// #define DAYLIGHT_OFFSET 0         // 无夏令时
// #define RTC_PREFS_NAME  "rtc_time"// 存储时间的命名空间（用于掉电保存）

// // -------------------------- 全局变量 --------------------------
// Preferences prefs;
// bool isRtcConfigured = false;    // RTC 是否已配置

TFT_eSprite *dispSprite = nullptr;

uint16_t w = 0;
uint16_t h = 0;

// DrawConfig drawCfg = {5, 0, 0, 0, 0, 0, 0, 80};
DisplayLayout dLayout = {
	.time_date_X = 0,	// 靠左
	.timeY = 0,
	.dateY = 0,
	.sensorIconY = 0,
	.sensorDataY = 0,
	.sensorLableY = 0,
	.sensorDataH = 0,
	.sensorIcon = 48,
	.tempX = 0,	// 中心位置
	.humiX = 0,
	.co2X = 0,
	.gifX = 0,
	.gifY = 0,
	.gifW = 0,
	.gifH = 0,
};

static constexpr int16_t CO2_TEXT_X_SHIFT_PX = 0; // move CO2 text slightly to the right

static inline int16_t co2TextCenterX(const DisplayLayout* dl) {
	return (int16_t)(dl->co2X + dl->sensorIcon / 2 + CO2_TEXT_X_SHIFT_PX);
}

static void drawCo2ValueWithUnit(TFT_eSprite* sprite, int16_t centerX, int16_t y, int co2ppmValue) {
	if (!sprite) return;
	if (co2ppmValue < 0) co2ppmValue = 0;
	if (co2ppmValue > 9999) co2ppmValue = 5500;

	const String valueStr = String(co2ppmValue);
	const String unitStr = "ppm";

	sprite->setTextFont(MIDDLE_FONT_ID);
	const int16_t wValue = sprite->textWidth(valueStr, MIDDLE_FONT_ID);
	const int16_t wUnit = sprite->textWidth(unitStr, MIDDLE_FONT_ID);
	const int16_t totalW = (int16_t)(wValue + 2 + wUnit);
	const int16_t startX = (int16_t)(centerX - totalW / 2);

	sprite->setTextDatum(TL_DATUM);
	sprite->drawString(valueStr, startX, y);
	sprite->drawString(unitStr, (int16_t)(startX + wValue + 2), y);
}

const uint8_t qrCodeData[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0,
    0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0,
    0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 
    0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 
    0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 
    0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 
    0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 
    0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 
    0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

const int qrCodeSize = 32;
const int qrBlockSize = 5;



char timeStr[10] = {0};
String dateStr = "";

// static bool initSprite(TFT_eSprite* &sprite, TFT_eSPI* tft) {

// 	if (sprite) {
// 		Serial.println("Sprite already initialized!");
// 		return true;
// 	}
	
// 	sprite = new TFT_eSprite(tft);
// 	if (!sprite) {
// 		Serial.println("Failed to create sprite (out of memory)!");
// 		return false;
// 	}
// 	sprite->setColorDepth(16);
// 	sprite->createSprite(tft->width(), tft->height());
// 	sprite->fillSprite(TFT_BLACK);
// 	sprite->setTextColor(TFT_WHITE, TFT_BLACK);

// 	Serial.println("Sprite initialized successfully!");
// 	return true;
// }

bool initSprite(TFT_eSPI* tft) {

	if (!tft) {
		Serial.println("Error: TFT pointer is null");
		return false;
	}	

	if (dispSprite) {
		Serial.println("Sprite already initialized!");
		return true;
	}
	w = tft->width();
	h = tft->height();
	dispSprite = new TFT_eSprite(tft);
	if (!dispSprite) {
		Serial.println("Failed to create sprite (out of memory)!");
		return false;
	}
	dispSprite->setColorDepth(16);
	dispSprite->createSprite(w, h);
	dispSprite->fillSprite(TFT_BLACK);
	dispSprite->setTextColor(TFT_WHITE, TFT_BLACK);

	Serial.println("Sprite initialized successfully!");
	return true;
}

void drawDisplay(bool showQR, const String& text, int qrTextYOffset) {

	// 安全检查：Sprite未初始化直接返回
    if (dispSprite == nullptr) {
        Serial.println("Error: dispSprite is not initialized!");
        return;
    }

	dispSprite->fillScreen(TFT_BLACK);

	if (showQR) {
		if (qrCodeData == nullptr || qrCodeSize <= 0) {
			Serial.println("Error: QR code data is invalid!");
            return;
		}

		int qrTotalSize = qrCodeSize * qrBlockSize;  // 二维码总像素宽度
    	int xOffset = (w - qrTotalSize) / 2;   // 水平居中偏移
    	int yOffset = (h - qrTotalSize) / 2;  // 垂直居中偏移

		// 遍历二维码矩阵，绘制每个模块
		for (int y = 0; y < qrCodeSize; y++) {
			for (int x = 0; x < qrCodeSize; x++) {
				// 从矩阵中获取当前模块的颜色（0=白，1=黑）
				// 假设矩阵是按行存储的一维数组，index = y * matrixSize + x
				bool isBlack = qrCodeData[y * qrCodeSize + x] == 1;
				
				// 计算当前模块在Sprite中的坐标
				int drawX = xOffset + x * qrBlockSize;
				int drawY = yOffset + y * qrBlockSize;
				
				// 绘制模块（填充矩形）
				dispSprite->fillRect(
					drawX,          // 起始X
					drawY,          // 起始Y
					qrBlockSize,      // 宽度
					qrBlockSize,      // 高度
					isBlack ? TFT_BLACK : TFT_WHITE  // 颜色
				);
			}
		}

		if (text.length() > 0) {
			dispSprite->setTextDatum(TC_DATUM);
			dispSprite->setTextFont(MIDDLE_FONT_ID);
			dispSprite->drawString(text, w / 2, yOffset + qrTotalSize + qrTextYOffset);
		}
	} else {
		if (text.length() > 0) {
			dispSprite->setTextDatum(TC_DATUM);
			dispSprite->setTextFont(MIDDLE_FONT_ID);

			int fontH = dispSprite->fontHeight();
			int lineCount = 1;
			String temp = text;

			while (temp.indexOf('\n') != -1) {
				lineCount++;
				temp = temp.substring(temp.indexOf('\n') + 1);
			}

			// 计算文字整体居中的起始Y坐标
            int totalTextHeight = lineCount * fontH;
            int startY = (h - totalTextHeight) / 2;
            int lineIndex = 0;

            // 拆分并绘制多行文字
            temp = text;
			while(lineIndex != lineCount){
				String line = temp.substring(0, temp.indexOf('\n'));
				dispSprite->drawString(line, w / 2, startY + lineIndex * fontH);
				temp = temp.substring(temp.indexOf('\n') + 1);
				lineIndex++;
			} 
            
            // 绘制最后一行
            if (temp.length() > 0) {
                dispSprite->drawString(temp, w / 2, startY + lineIndex * fontH);
            }
		}
	}

	dispSprite->pushSprite(0, 0);		
}


// 将 __DATE__ 解析为 YYYY-MM-DD（__DATE__ 格式: "Nov 17 2025"）
String formatDate() {
  const char *dateStr = __DATE__; // 例如 "Nov 17 2025"
  char monStr[4];
  int day, year;
  sscanf(dateStr, "%3s %d %d", monStr, &day, &year);
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int monthNum = (strstr(months, monStr) - months) / 3 + 1;
  char buf[16];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, monthNum, day);
  return String(buf);
}

static bool getCurrentLocalDateTime(struct tm* outTm) {
	if (!outTm) return false;
	time_t now;
	time(&now);
	// simple validity gate: avoid showing 1970-01-01 after boot
	if (now <= 1700000000) return false;
	localtime_r(&now, outTm);
	return true;
}

static String formatDateFromTm(const struct tm& t) {
	char buf[16];
	snprintf(buf, sizeof(buf), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
	return String(buf);
}

static String formatTimeHHMMFromTm(const struct tm& t) {
	char buf[8];
	snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
	return String(buf);
}

void updateTimeArea(TFT_eSprite* sprite, const DisplayLayout* dl, bool force) {
	if (!sprite || !dl) return;

	static int lastMinute = -1;
	static int lastYday = -1;
	static bool lastValid = false;

	struct tm t;
	const bool valid = getCurrentLocalDateTime(&t);
	const int minuteNow = valid ? t.tm_min : -1;
	const int ydayNow = valid ? t.tm_yday : -1;

	if (!force && valid == lastValid && minuteNow == lastMinute && ydayNow == lastYday) {
		return;
	}

	lastValid = valid;
	lastMinute = minuteNow;
	lastYday = ydayNow;

	const String timeText = valid ? formatTimeHHMMFromTm(t) : String("--:--");
	const String dateText = valid ? formatDateFromTm(t) : String("----/--/--");

	// Compute the bounding area of the time+date block and clear it
	const int16_t xTime = (int16_t)(dl->time_date_X - sprite->textWidth("9", MIDDLE_FONT_ID));
	const int16_t yTime = dl->timeY;
	const int16_t timeW = sprite->textWidth("88:88", LARGE_FONT_ID);
	const int16_t timeH = sprite->fontHeight(LARGE_FONT_ID);

	const int16_t xDate = dl->time_date_X;
	const int16_t yDate = dl->dateY;
	const int16_t dateW = sprite->textWidth("8888-88-88", MIDDLE_FONT_ID);
	const int16_t dateH = sprite->fontHeight(MIDDLE_FONT_ID);

	int16_t left = xTime < xDate ? xTime : xDate;
	int16_t top = yTime;
	int16_t right = (xTime + timeW) > (xDate + dateW) ? (xTime + timeW) : (xDate + dateW);
	int16_t bottom = (yDate + dateH) > (yTime + timeH) ? (yDate + dateH) : (yTime + timeH);

	const int16_t pad = 4;
	left = (int16_t)(left - pad);
	top = (int16_t)(top - pad);
	right = (int16_t)(right + pad);
	bottom = (int16_t)(bottom + pad);
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right > (int16_t)w) right = (int16_t)w;
	if (bottom > (int16_t)h) bottom = (int16_t)h;

	const int16_t clearW = (int16_t)(right - left);
	const int16_t clearH = (int16_t)(bottom - top);
	if (clearW <= 0 || clearH <= 0) return;

	sprite->fillRect(left, top, clearW, clearH, TFT_BLACK);

	// Redraw time and date
	sprite->setTextDatum(TL_DATUM);
	sprite->setTextFont(LARGE_FONT_ID);
	sprite->drawString(timeText, xTime, yTime);
	sprite->setTextFont(MIDDLE_FONT_ID);
	sprite->drawString(dateText, xDate, yDate);

	// 在时间区域右侧绘制电池状态（充电/电量），放在时间与天气图标之间的空白区
	updateBatteryStatus(sprite, dl);

	// Push only the affected area
	sprite->pushSprite(left, top, left, top, clearW, clearH);
}



void calcLayoutParams() {
	// dateStr previously used compile-time __DATE__; keep a stable-width sample for layout.
	dateStr = "8888-88-88";

	// 时间区域
	int16_t timeW = dispSprite->textWidth("88:88", LARGE_FONT_ID);
	int16_t timeH = dispSprite->fontHeight(LARGE_FONT_ID);
	int16_t dateW = dispSprite->textWidth(dateStr, MIDDLE_FONT_ID);
	int16_t dateH = dispSprite->fontHeight(MIDDLE_FONT_ID);
	
	dLayout.time_date_X = w / 8;

	dLayout.timeY = (h / 2 - timeH - dateH) / 3;

	dLayout.dateY = dLayout.timeY * 2 + timeH;

	// 传感器区域

	dLayout.sensorDataH = dispSprite->fontHeight(MIDDLE_FONT_ID);

	int16_t gap = (h / 2 - dLayout.sensorIcon  - dLayout.sensorDataH ) / 4;

	dLayout.sensorIconY = h / 2 + gap;

	// dLayout.sensorIconY = h / 2 + sensorAreaGap;
	dLayout.sensorDataY = dLayout.sensorIconY + dLayout.sensorIcon + gap * 3 / 2;
	// dLayout.sensorLableY = dLayout.sensorDataY + dLayout.sensorDataH + sensorAreaGap / 2;

	gap = (w - dLayout.sensorIcon * 3) / 3;

	dLayout.tempX = gap / 2;
	dLayout.humiX = gap *3 / 2 + dLayout.sensorIcon;
	dLayout.co2X = w - gap / 2 - dLayout.sensorIcon;

	// int16_t tempDataW = dispSprite->textWidth("99.9 C", MIDDLE_FONT_ID);
	// int16_t humiDataW = dispSprite->textWidth("99.9%", MIDDLE_FONT_ID);
	// int16_t co2DataW = dispSprite->textWidth("9999", MIDDLE_FONT_ID);

}

void initDrawSensorData(TFT_eSPI* tft, const SensorData* initData) {
	
	// 若尚未创建，则在此创建 Sprite
	if (!dispSprite) {
		dispSprite = new TFT_eSprite(tft);
		if(!dispSprite) {
			Serial.println("Failed to create sprite!");
			while(1);
		}
		dispSprite->setColorDepth(16);
		dispSprite->createSprite(w, h);
		dispSprite->setTextColor(TFT_WHITE, TFT_BLACK);
	} 

	dispSprite->fillSprite(TFT_BLACK);

	calcLayoutParams();

	// 时间（实时：基于系统 time() / localtime_r，每分钟更新）
	updateTimeArea(dispSprite, &dLayout, true);
	// 在首次绘制时同时绘制电池状态
	updateBatteryStatus(dispSprite, &dLayout);

	// 传感器

	// Icon
	dispSprite->pushImage(dLayout.tempX, dLayout.sensorIconY, 48, 48, icon_temp);
	dispSprite->pushImage(dLayout.humiX, dLayout.sensorIconY, 48, 48, icon_humi);
	dispSprite->pushImage(dLayout.co2X, dLayout.sensorIconY, 48, 48, icon_CO2);


	// Data
	dispSprite->setTextFont(MIDDLE_FONT_ID);
	dispSprite->setTextDatum(TR_DATUM);

	int16_t xaa = dispSprite->textWidth("9", MIDDLE_FONT_ID) + dLayout.sensorIcon / 2;

	dLayout.tempX += xaa;
	dLayout.humiX += xaa;

	dispSprite->drawString(String(initData->temp, 1) , dLayout.tempX, dLayout.sensorDataY);
	dispSprite->drawString(String(initData->humi, 1) , dLayout.humiX, dLayout.sensorDataY);

	dispSprite->setTextDatum(TL_DATUM);
	dispSprite->drawString("C", dLayout.tempX + CIRCLE_RADIUS * 2, dLayout.sensorDataY);
	dispSprite->drawCircle(dLayout.tempX + CIRCLE_RADIUS, dLayout.sensorDataY + CIRCLE_RADIUS, CIRCLE_RADIUS, TFT_WHITE);
	dispSprite->drawString("%", dLayout.humiX, dLayout.sensorDataY);

	// CO2 value + unit (centered under icon, shifted right slightly)
	drawCo2ValueWithUnit(dispSprite, co2TextCenterX(&dLayout), dLayout.sensorDataY, (int)initData->co2);

	// // Label
	// dispSprite->setTextDatum(TC_DATUM);
	// dispSprite->setTextFont(SMALL_FONT_ID);
	// dispSprite->drawString("Temp", dLayout.tempX, dLayout.sensorLableY);
	// dispSprite->drawString("Humi", dLayout.humiX, dLayout.sensorLableY);
	// dispSprite->drawString("CO2(ppm)", dLayout.co2X, dLayout.sensorLableY);

	dispSprite->pushSprite(0, 0);
}

void updateSensorData(TFT_eSprite* sprite, int id, float value, const DisplayLayout* dl) {
	if (!sprite) return;

	char valueBuf[10];
	int16_t targetX = 0;
	// uint16_t color = TFT_WHITE;
	int16_t strWidth = 0;
	int16_t targetW = sprite->textWidth("9999", MIDDLE_FONT_ID);


	switch(id) {
	    case 1: // 温度
			// color = TFT_YELLOW;
			targetX = dl->tempX;
        	sprintf(valueBuf, "%.1f", value);
			break;
	    case 2: // 湿度
			// color = TFT_GREENYELLOW;
			targetX = dl->humiX;
        	sprintf(valueBuf, "%.1f", value);
			break;
	    case 3: // CO2
			// render as "<value>  ppm" with 2px spacing, centered under icon
			{
				const int16_t centerX = co2TextCenterX(dl);
				const int16_t y = dl->sensorDataY;
				int co2ppmValue = (int)value;
				if (co2ppmValue < 0) co2ppmValue = 0;
				if (co2ppmValue > 9999) co2ppmValue = 9999;

				sprite->setTextFont(MIDDLE_FONT_ID);
				const int16_t maxW = sprite->textWidth("9999 ppm", MIDDLE_FONT_ID);
				const String valueStr = String(co2ppmValue);
				const int16_t wValue = sprite->textWidth(valueStr, MIDDLE_FONT_ID);
				const int16_t wUnit = sprite->textWidth("ppm", MIDDLE_FONT_ID);
				const int16_t totalW = (int16_t)(wValue + 2 + wUnit);
				const int16_t clearW = totalW > maxW ? totalW : maxW;
				int16_t clearX = (int16_t)(centerX - clearW / 2 - 2);
				if (clearX < 0) clearX = 0;

				sprite->fillRect(clearX, y, (int16_t)(clearW + 4), dl->sensorDataH, TFT_BLACK);
				drawCo2ValueWithUnit(sprite, centerX, y, co2ppmValue);
				sprite->pushSprite(clearX, (int16_t)(y - 2), clearX, (int16_t)(y - 2), (int16_t)(clearW + 4), (int16_t)(dl->sensorDataH + 4));
			}
			return;
	    default:
			return;
	}

	sprite->fillRect(targetX - targetW, dl->sensorDataY, targetW, dl->sensorDataH, TFT_BLACK);

	targetW = sprite->textWidth(valueBuf, MIDDLE_FONT_ID);
	targetX -= targetW;
	sprite->fillRect(targetX, dl->sensorDataY, targetW, dl->sensorDataH, TFT_BLACK);
    sprite->setTextFont(MIDDLE_FONT_ID);
	sprite->setCursor(targetX, dl->sensorDataY);
	sprite->print(valueBuf);
	sprite->pushSprite(targetX - 2, dl->sensorDataY - 2, targetX - 2, dl->sensorDataY - 2, targetW + 4, dl->sensorDataH + 4);
}

void updateWeatherWind(TFT_eSprite* sprite, int windLevel, const DisplayLayout* dl) {
	if (!sprite || !dl) return;

	// Use the third slot area (same as CO2 position)
	const int16_t centerX = co2TextCenterX(dl);
	const int16_t y = dl->sensorDataY;

	char buf[16];
	if (windLevel < 0) {
		strcpy(buf, "--");
	} else {
		if (windLevel > 12) windLevel = 12;
		snprintf(buf, sizeof(buf), "Lv%d", windLevel);
	}

	const String text = String(buf);

	sprite->setTextFont(MIDDLE_FONT_ID);
	sprite->setTextDatum(TC_DATUM);

	// Clear wide enough to cover CO2's "9999 ppm" from the indoor view
	int16_t maxW = sprite->textWidth("Lv12", MIDDLE_FONT_ID);
	int16_t co2W = sprite->textWidth("9999 ppm", MIDDLE_FONT_ID);
	if (co2W > maxW) maxW = co2W;
	int16_t wText = sprite->textWidth(text, MIDDLE_FONT_ID);
	int16_t clearW = wText > maxW ? wText : maxW;
	int16_t clearX = centerX - clearW / 2 - 2;
	if (clearX < 0) clearX = 0;

	sprite->fillRect(clearX, y, clearW + 4, dl->sensorDataH, TFT_BLACK);
	sprite->drawString(text, centerX, y);
	sprite->pushSprite(clearX, y - 2, clearX, y - 2, clearW + 4, dl->sensorDataH + 4);
}

static void updateTopRightIcon(TFT_eSprite* sprite, const uint16_t* pixels, int iconW, int iconH) {
	if (!sprite || !pixels) return;

	static const uint16_t* lastPixels = nullptr;
	static int16_t lastX = 0, lastY = 0, lastW = 0, lastH = 0;

	if (pixels == lastPixels) return;

	// --- Position tuning knobs (keep consistent with previous weather icon placement) ---
	const int16_t kIconMarginRight = 25;
	const int16_t kIconMarginTop = 25;
	const int16_t kMinGapFromTime = 12;

	int16_t x = (int16_t)(w - iconW - kIconMarginRight);
	int16_t y = kIconMarginTop;

	// Avoid colliding with the time area on the left
	int16_t timeLeftX = dLayout.time_date_X - sprite->textWidth("9", MIDDLE_FONT_ID);
	int16_t timeRightX = timeLeftX + sprite->textWidth("88:88", LARGE_FONT_ID);
	int16_t minX = timeRightX + kMinGapFromTime;
	if (x < minX) x = minX;
	if (x < 0) x = 0;
	if (x > (int16_t)(w - iconW)) x = (int16_t)(w - iconW);
	if (y < 0) y = 0;

	// clear previous icon area (if any)
	if (lastW > 0 && lastH > 0) {
		sprite->fillRect(lastX, lastY, lastW, lastH, TFT_BLACK);
		sprite->pushSprite(lastX, lastY, lastX, lastY, lastW, lastH);
	}

	// draw into sprite and push updated region
	sprite->pushImage(x, y, iconW, iconH, pixels);
	sprite->pushSprite(x, y, x, y, iconW, iconH);

	lastPixels = pixels;
	lastX = x;
	lastY = y;
	lastW = iconW;
	lastH = iconH;
}

void updateWeatherIcon(TFT_eSprite* sprite, const String& weatherText) {
	if (!sprite) return;

	const char* wt = weatherText.c_str();
	const uint16_t* pixels = nullptr;

	if (wt && wt[0] != '\0') {
		// pass 1: exact match
		for (size_t i = 0; i < WEATHER_ICON_TABLE_COUNT; i++) {
			const char* label = WEATHER_ICON_TABLE[i].label;
			if (label && strcmp(label, wt) == 0) {
				pixels = WEATHER_ICON_TABLE[i].pixels;
				break;
			}
		}
		// pass 2: substring match (either direction)
		if (!pixels) {
			for (size_t i = 0; i < WEATHER_ICON_TABLE_COUNT; i++) {
				const char* label = WEATHER_ICON_TABLE[i].label;
				if (!label) continue;
				if (strstr(label, wt) != nullptr || strstr(wt, label) != nullptr) {
					pixels = WEATHER_ICON_TABLE[i].pixels;
					break;
				}
			}
		}
	}

	// fallback: "无"
	if (!pixels) {
		for (size_t i = 0; i < WEATHER_ICON_TABLE_COUNT; i++) {
			const char* label = WEATHER_ICON_TABLE[i].label;
			if (label && strcmp(label, "无") == 0) {
				pixels = WEATHER_ICON_TABLE[i].pixels;
				break;
			}
		}
	}

	if (!pixels) {
		static String lastMiss = "";
		if (weatherText != lastMiss) {
			Serial.printf("[weather-icon] no match for weatherText=%s\n", weatherText.c_str());
			lastMiss = weatherText;
		}
		return;
	}

	updateTopRightIcon(sprite, pixels, WEATHER_ICON_W, WEATHER_ICON_H);
}

void updateIndoorIcon(TFT_eSprite* sprite) {
	updateTopRightIcon(sprite, icon_indoor, 48, 48);
}

void safeDeleteSprite(TFT_eSprite* sprite) {
	if (sprite) {
		sprite->deleteSprite();
		delete sprite;
		sprite = nullptr;
	}
}

// --------------------- Battery display helpers ---------------------
static float readBatteryVoltage() {
	// Use esp_adc_cal to convert raw->mV for better accuracy
	static bool adcCalInited = false;
	static esp_adc_cal_characteristics_t adc_chars;
	if (!adcCalInited) {
		esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
		adcCalInited = true;
	}
	int raw = analogRead(BATTERY_PIN);
	uint32_t vpin_mV = esp_adc_cal_raw_to_voltage((uint32_t)raw, &adc_chars);
	float usedFactor = BATTERY_VOLTAGE_FACTOR;
#if defined(BATTERY_CALIBRATION_MV) && (BATTERY_CALIBRATION_MV > 0)
	if (vpin_mV > 0) usedFactor = ((float)BATTERY_CALIBRATION_MV) / ((float)vpin_mV);
#endif
	float vbatt = ((float)vpin_mV / 1000.0f) * usedFactor;
	return vbatt;
}

static bool readChargingState() {
	// Charging pin is HIGH when charging according to hardware doc
	return digitalRead(CHARGING_PIN) == HIGH;
}

void updateBatteryStatus(TFT_eSprite* sprite, const DisplayLayout* dl) {
	if (!sprite || !dl) return;

	// visual params
	const int16_t batW = 30; // 再短一点
	const int16_t batH = 14;
	const int16_t capW = 4; // battery cap width
	const int16_t margin = 10;
	const int16_t kIconMarginRight = 25;
	const int16_t kIconMarginTop = 25;
	const int16_t kMinGapFromTime = 12;

	// compute safe horizontal area between time and weather icon
	int16_t timeLeftX = dl->time_date_X - sprite->textWidth("9", MIDDLE_FONT_ID);
	int16_t timeRightX = timeLeftX + sprite->textWidth("88:88", LARGE_FONT_ID);
	int16_t weatherLeftX = (int16_t)(w - WEATHER_ICON_W - kIconMarginRight);
	int16_t availLeft = timeRightX + kMinGapFromTime;
	int16_t availRight = weatherLeftX - kMinGapFromTime - batW - capW;
	int16_t x = availLeft + margin;
	if (availRight > availLeft) {
		// center between available space
		x = availLeft + (availRight - availLeft) / 2;
	}
	if (x < 0) x = 0;
	if (x + batW + capW > (int16_t)w) x = (int16_t)w - batW - capW - 1;
	int16_t y = kIconMarginTop;

	// Read battery voltage using multiple samples and simple outlier rejection
	const int sampleCount = 16;
	uint32_t samples[sampleCount];
	for (int i = 0; i < sampleCount; ++i) {
		samples[i] = analogRead(BATTERY_PIN);
		delay(2);
	}
	// find min/max and sum others
	uint32_t minV = UINT32_MAX, maxV = 0;
	uint64_t sum = 0;
	for (int i = 0; i < sampleCount; ++i) {
		if (samples[i] < minV) minV = samples[i];
		if (samples[i] > maxV) maxV = samples[i];
		sum += samples[i];
	}
	// discard single min and max to reduce spikes
	sum -= minV;
	sum -= maxV;
	float avgRaw = (float)sum / (sampleCount - 2);

	// Use esp_adc_cal to convert raw->mV
	static bool adcCalInited = false;
	static esp_adc_cal_characteristics_t adc_chars;
	if (!adcCalInited) {
		esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
		adcCalInited = true;
	}
	uint32_t vpin_mV = esp_adc_cal_raw_to_voltage((uint32_t)avgRaw, &adc_chars);
	float vpin = (float)vpin_mV / 1000.0f;

	// Compute theoretical factor from resistor values (Vbatt = Vpin * (Rtop+Rbot)/Rbot)
	const float rtop = (float)BATTERY_R_TOP_OHMS;
	const float rbot = (float)BATTERY_R_BOTTOM_OHMS;
	const float factor_theoretical = (rtop + rbot) / rbot;

	// usedFactor: default to theoretical; if user provided BATTERY_CALIBRATION_MV, compute factor to match that one-point calibration
	float usedFactor = factor_theoretical;
#if defined(BATTERY_CALIBRATION_MV) && (BATTERY_CALIBRATION_MV > 0)
	if (vpin_mV > 0) usedFactor = ((float)BATTERY_CALIBRATION_MV) / ((float)vpin_mV);
#endif

	float vbatt = vpin * usedFactor;

	// 调试输出：打印细节信息，便于核对和调参
	Serial.printf("[bat-dbg] samples_min=%lu max=%lu avgRaw=%.1f vpin_mV=%lu vpin=%.3f factor_theo=%.4f usedFactor=%.4f vbatt=%.3f\n",
				  (unsigned long)minV, (unsigned long)maxV, avgRaw, (unsigned long)vpin_mV, vpin, factor_theoretical, usedFactor, vbatt);

	// charging detection: immediate on rising edge, debounced on falling edge
	static bool chargingState = false;
	static unsigned long lastFallingMs = 0;
	const unsigned long fallDebounceMs = 300; // wait before clearing charging state

	int raw = digitalRead(CHARGING_PIN);
	if (raw == HIGH) {
		// immediate set when charger present
		if (!chargingState) {
			chargingState = true;
		}
		lastFallingMs = 0;
	} else {
		// schedule clearing after debounce
		if (chargingState) {
			if (lastFallingMs == 0) lastFallingMs = millis();
			else if ((unsigned long)(millis() - lastFallingMs) > fallDebounceMs) {
				chargingState = false;
				lastFallingMs = 0;
			}
		}
	}
	const bool charging = chargingState;

	// small change detection for logging
	static float lastLoggedV = 0.0f;
	static bool lastLoggedCharging = false;
	if (fabs(vbatt - lastLoggedV) > 0.03f || charging != lastLoggedCharging) {
		Serial.printf("[bat] Vbat=%.3fV charging=%d\n", vbatt, charging ? 1 : 0);
		lastLoggedV = vbatt;
		lastLoggedCharging = charging;
	}

	// map voltage to percent (0..1)
	float pct = (vbatt - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
	if (pct < 0) pct = 0;
	if (pct > 1) pct = 1;

	// clear area first (include extra space for charging icon)
	const int16_t clearW = batW + capW + 16;
	const int16_t clearH = batH + 16;
	sprite->fillRect(x - 3, y - 3, clearW, clearH, TFT_BLACK);

	// outline
	sprite->drawRect(x, y, batW, batH, TFT_WHITE);
	// cap
	sprite->fillRect(x + batW, y + (batH - batH/2)/2, capW, batH/2, TFT_WHITE);

	// filled level: no numeric text, use white fill for filled portion, black for empty
	int16_t innerW = batW - 4;
	int16_t innerH = batH - 4;
	int16_t levelW = (int16_t)(innerW * pct + 0.5f);
	if (levelW < 0) levelW = 0;

	// background (empty = black)
	sprite->fillRect(x + 2, y + 2, innerW, innerH, 0x0000);
	// filled part (white)
	if (levelW > 0) sprite->fillRect(x + 2, y + 2, levelW, innerH, TFT_WHITE);

	// If nearly full, fill entire battery (solid white). If nearly empty, leave only border.
	if (pct >= 0.95f) {
		sprite->fillRect(x + 2, y + 2, innerW, innerH, TFT_WHITE);
	} else if (pct <= 0.05f) {
		// leave as border only (already cleared)
	}

	// charging indicator (larger lightning) when charging
	if (charging) {
		// larger lightning composed of blocks for visibility
		int16_t lx = x + batW + capW + 4;
		int16_t ly = y - 2;
		sprite->fillRect(lx + 5, ly + 0, 2, 5, TFT_YELLOW);
		sprite->fillRect(lx + 2, ly + 5, 7, 3, TFT_YELLOW);
		sprite->fillRect(lx + 4, ly + 8, 4, 4, TFT_YELLOW);
		// small white highlight for contrast
		sprite->fillRect(lx + 6, ly + 2, 1, 2, TFT_WHITE);
	}

	// push updated area
	int16_t pushX = x - 3;
	int16_t pushY = y - 3;
	if (pushX < 0) pushX = 0;
	if (pushY < 0) pushY = 0;
	sprite->pushSprite(pushX, pushY, pushX, pushY, clearW, clearH);
}

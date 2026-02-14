#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
// #include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include "project_config.h"
#include "WiFiManager.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_OFFSET, 60000);
Preferences prefs;

struct DateTime {
    String date;  // 格式: YYYY/MM/DD
    String time;  // 格式: HH:MM
} dt={"----/--/--", "--:--"};

void syncTimeFromNTP() {
    if (!isWiFiConnected()) {
        Serial.println("WiFi未连接，无法同步NTP时间");
        return;
    }

    Serial.println("开始NTP时间同步...");
    timeClient.begin();
    
    // 最多尝试5次获取时间
    int retryCount = 0;
    while (!timeClient.update() && retryCount < 5) {
        Serial.printf("第%d次尝试同步NTP时间...\n", retryCount + 1);
        timeClient.forceUpdate();
        delay(1000);
        retryCount++;
    }

    if (timeClient.update()) {
        time_t epochTime = timeClient.getEpochTime();
        struct tm timeinfo;
        gmtime_r(&epochTime, &timeinfo);
        
        // 保存到偏好设置
        prefs.begin("rtc", false);
        prefs.putULong("epoch", epochTime);
        prefs.end();
        
        Serial.printf("NTP同步成功，当前时间: %04d-%02d-%02d %02d:%02d\n",
                   timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        Serial.println("NTP时间同步失败");
    }
    
    timeClient.end();  // 释放资源
}

// void setLocalTime(int year, int month, int day, int hour, int minute, int second);

 DateTime getDateAndTime() {

    prefs.begin("rtc", true);
    time_t epoch = prefs.getULong("epoch", 0);
    prefs.end();

    if (epoch == 0) {
        return dt;  // 未设置时间时返回占位符
    }

    // 计算当前时间（考虑系统运行时间）
    uint32_t currentMillis = millis();
    int32_t elapsedSeconds = (int32_t)(currentMillis / 1000);
    if (elapsedSeconds > 0) {
        epoch += elapsedSeconds;
    }

    struct tm timeinfo;
    gmtime_r(&epoch, &timeinfo);
    
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d",
            timeinfo.tm_hour,
            timeinfo.tm_min);    
    char dateStr[11];
    sprintf(dateStr, "%04d/%02d/%02d",
            timeinfo.tm_year + 1900,  // tm_year是从1900年开始的年数
            timeinfo.tm_mon + 1,      // tm_mon从0开始
            timeinfo.tm_mday);

    dt.date = dateStr;
    dt.time = timeStr;

    return dt;
}

void loadTimeFromRTC() {
    prefs.begin("rtc", true);
    time_t savedEpoch = prefs.getULong("epoch", 0);
    prefs.end();

    if (savedEpoch == 0) {
        Serial.println("未找到保存的时间数据");
    } else {
        struct tm timeinfo;
        gmtime_r(&savedEpoch, &timeinfo);
        Serial.printf("加载保存的时间: %04d-%02d-%02d %02d:%02d:%02d\n",
                   timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
}

#endif
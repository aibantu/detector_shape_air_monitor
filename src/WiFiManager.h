#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "project_config.h"

bool loadHtmlFromFile();
// String urlEncode(const String& str);

bool checkWiFiConfig();
void startWiFiConnect();
bool autoConnectWiFi();
void startConfigAP();
bool waitForWiFiConfig();
void saveWiFiConfig(const char* ssid, const char* pass);
bool isWiFiConnected();
void clearWiFiConfig();

#endif
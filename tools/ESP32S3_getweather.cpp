#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

Preferences prefs;

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASS";

// 任选：IP 或域名
const char* API_BASE = "http://120.48.177.209:8000"; 
// const char* API_BASE = "https://banbot.cn";

static String deviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "esp32s3-" + mac;
}

static bool loadLoc(String &adcode, float &lat, float &lon, String &city) {
  prefs.begin("weather", true);
  adcode = prefs.getString("adcode", "");
  lat = prefs.getFloat("lat", 0);
  lon = prefs.getFloat("lon", 0);
  city = prefs.getString("city", "");
  prefs.end();
  return adcode.length() > 0 && lat != 0 && lon != 0;
}

static void saveLoc(const String &adcode, float lat, float lon, const String &city) {
  prefs.begin("weather", false);
  prefs.putString("adcode", adcode);
  prefs.putFloat("lat", lat);
  prefs.putFloat("lon", lon);
  prefs.putString("city", city);
  prefs.end();
}

static bool httpPostJson(const String &url, const String &body, String &respOut) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  if (code > 0) respOut = http.getString();
  http.end();
  return code == 200 || code == 202;
}

static void fetchWeather() {
  String adcode, city;
  float lat=0, lon=0;
  bool hasLoc = loadLoc(adcode, lat, lon, city);

  String url = String(API_BASE) + (hasLoc ? "/v1/device/weather" : "/v1/device/bootstrap");

  StaticJsonDocument<256> req;
  req["device_id"] = deviceId();
  if (hasLoc) {
    req["adcode"] = adcode;
    req["lat"] = lat;
    req["lon"] = lon;
    if (city.length()) req["city"] = city;
  } else {
    if (city.length()) req["city"] = city; // 可选
  }

  String body;
  serializeJson(req, body);

  String resp;
  if (!httpPostJson(url, body, resp)) {
    // TODO: 失败重试/离线显示
    return;
  }

  StaticJsonDocument<1536> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) return;

  const char* status = doc["status"] | "";
  bool cached = doc["cached"] | false;

  // 保存服务器返回的定位信息（首次/缺失时）
  if (doc["device"].is<JsonObject>()) {
    String newAd = doc["device"]["adcode"] | "";
    float newLat = doc["device"]["lat"] | 0;
    float newLon = doc["device"]["lon"] | 0;
    String newCity = doc["device"]["city"] | "";
    if (newAd.length() && newLat != 0 && newLon != 0) {
      saveLoc(newAd, newLat, newLon, newCity);
    }
  }

  // 如果 accepted，说明后台抓取中；可稍后再请求一次
  if (String(status) == "accepted") {
    delay(1500);
    // 第二次直接走 weather（即使还没保存loc也可以再bootstrap一次）
    fetchWeather();
    return;
  }

  // 解析 weather（紧凑字段）
  if (doc["weather"].is<JsonObject>()) {
    String provider = doc["weather"]["provider"] | "";
    float temp = doc["weather"]["temp"] | NAN;
    int humidity = doc["weather"]["humidity"] | -1;
    String w = doc["weather"]["weather"] | "";
    float ws = doc["weather"]["wind_speed"] | NAN;
    int wd = doc["weather"]["wind_deg"] | -1;

    // TODO: 将这些显示到屏幕/墨水屏
  }
}

static void setupNtp() {
  configTime(8*3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org"); // CST
  time_t now = 0;
  for (int i=0; i<30; i++) {
    time(&now);
    if (now > 1700000000) break;
    delay(200);
  }
}

static void scheduleLoop() {
  // 每次 loop 检查是否到“本小时已请求过”的状态
  static int lastHour = -1;
  static bool scheduled = false;
  static uint32_t fireMs = 0;

  time_t now; time(&now);
  struct tm t; localtime_r(&now, &t);

  if (t.tm_hour != lastHour) {
    lastHour = t.tm_hour;
    scheduled = true;
    uint32_t delaySec = esp_random() % 300; // 0~299s
    fireMs = millis() + delaySec * 1000UL;
  }

  if (scheduled && (int32_t)(millis() - fireMs) >= 0) {
    scheduled = false;
    fetchWeather();
  }
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  setupNtp();
}

void loop() {
  scheduleLoop();
  delay(200);
}
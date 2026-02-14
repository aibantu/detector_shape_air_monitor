#ifndef WEATHER_UTILS_H
#define WEATHER_UTILS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>
#include "readSensorData.h"

extern Preferences prefs;

// 方案B：Nginx 对外提供 80/443，因此不要带 :8000
static const char* API_BASE = "http://120.48.177.209";

static String deviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "esp32s3-" + mac;
}

static void ensurePrefsNamespace() {
  // On a fresh device, opening in read-only mode can fail with NOT_FOUND.
  if (prefs.begin("weather", true)) {
    prefs.end();
    return;
  }
  if (prefs.begin("weather", false)) {
    prefs.end();
  }
}

static bool loadLoc(String &adcode, float &lat, float &lon, String &city) {
  ensurePrefsNamespace();
  if (!prefs.begin("weather", true)) {
    return false;
  }

  const bool hasAdcode = prefs.isKey("adcode");
  const bool hasLat = prefs.isKey("lat");
  const bool hasLon = prefs.isKey("lon");
  const bool hasCity = prefs.isKey("city");

  adcode = hasAdcode ? prefs.getString("adcode") : "";
  lat = hasLat ? prefs.getFloat("lat") : 0;
  lon = hasLon ? prefs.getFloat("lon") : 0;
  city = hasCity ? prefs.getString("city") : "";

  prefs.end();
  return hasAdcode && hasLat && hasLon && adcode.length() > 0 && lat != 0 && lon != 0;
}

static bool saveLoc(const String &adcode, float lat, float lon, const String &city) {
  ensurePrefsNamespace();
  if (!prefs.begin("weather", false)) {
    return false;
  }
  prefs.putString("adcode", adcode);
  prefs.putFloat("lat", lat);
  prefs.putFloat("lon", lon);
  prefs.putString("city", city);
  prefs.end();
  return true;
}

static bool httpPostJson(const String &url, const String &body, String &respOut) {
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  if (!http.begin(url)) {
    Serial.printf("[weather] http.begin() failed: %s\n", url.c_str());
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "esp32s3-weather/1.0");

  int code = http.POST(body);
  if (code > 0) {
    respOut = http.getString();
  } else {
    Serial.printf("[weather] HTTP POST failed, err=%d (%s)\n",
                  code,
                  HTTPClient::errorToString(code).c_str());
  }
  http.end();
  return code == 200 || code == 202;
}

static bool isJsonNumber(const JsonVariantConst &v) {
  return !v.isNull() && (v.is<float>() || v.is<double>() || v.is<long>() || v.is<int>() || v.is<unsigned long>() || v.is<unsigned int>());
}

static bool isJsonString(const JsonVariantConst &v) {
  return !v.isNull() && v.is<const char*>();
}

static bool tryParseInt(const JsonVariantConst &v, long &out) {
  if (v.isNull()) return false;
  if (v.is<long>() || v.is<int>() || v.is<unsigned long>() || v.is<unsigned int>()) {
    out = v.as<long>();
    return true;
  }
  if (v.is<double>() || v.is<float>()) {
    double d = v.as<double>();
    if (isnan(d)) return false;
    out = lround(d);
    return true;
  }
  if (v.is<const char*>()) {
    const char *s = (const char*)v;
    if (!s || !*s) return false;

    // accept "3" or "3级" etc.
    while (*s == ' ' || *s == '\t') s++;
    char *endp = nullptr;
    long val = strtol(s, &endp, 10);
    if (endp == s) return false;
    out = val;
    return true;
  }
  return false;
}

// Extract compact weather object (what /v1/device/* returns)
static void applyCompactWeatherToSensor(const JsonObjectConst &wobj, SensorData *data) {
  if (!data) return;

  const bool hasTemp = isJsonNumber(wobj["temp"]);
  const bool hasHumi = isJsonNumber(wobj["humidity"]);
  const bool hasDeg  = isJsonNumber(wobj["wind_deg"]);

  // Server may return wind level (0~12). Some providers use windpower/wind_power.
  long lvl = -1;
  JsonVariantConst vLvl = wobj["wind_level"];
  if (vLvl.isNull()) vLvl = wobj["windLevel"];
  if (vLvl.isNull()) vLvl = wobj["wind_force"];
  if (vLvl.isNull()) vLvl = wobj["windForce"];
  if (vLvl.isNull()) vLvl = wobj["windpower"];
  if (vLvl.isNull()) vLvl = wobj["wind_power"];
  if (vLvl.isNull()) vLvl = wobj["windPower"];
  const bool hasWindLevel = tryParseInt(vLvl, lvl);

  if (hasTemp) data->outdoorTemp = (float)wobj["temp"].as<double>();
  if (hasHumi) data->outdoorHumi = (float)wobj["humidity"].as<double>();

  if (hasWindLevel) {
    if (lvl < 0) lvl = 0;
    if (lvl > 12) lvl = 12;
    data->windLevel = (int8_t)lvl;
  }

  String provider = wobj["provider"] | "";
  String weatherText = isJsonString(wobj["weather"]) ? String((const char*)wobj["weather"]) : String("");

  // Save weather text for icon matching/UI
  data->weatherText = weatherText;

  Serial.printf("[weather] provider=%s weather=%s temp=%s humi=%s wind_level=%s deg=%s\n",
                provider.c_str(),
                weatherText.length() ? weatherText.c_str() : "(null)",
                hasTemp ? String((double)data->outdoorTemp, 2).c_str() : "(null)",
                hasHumi ? String((double)data->outdoorHumi, 2).c_str() : "(null)",
                hasWindLevel ? String((int)lvl).c_str() : "(null)",
                hasDeg ? String(wobj["wind_deg"].as<int>()).c_str() : "(null)");
}

// Fallback: call /v1/weather to get full cached data and try to extract gaode temp etc from chosen_data.
static bool tryFallbackFullWeather(const String &adcode, float lat, float lon, const String &city, SensorData *data) {
  StaticJsonDocument<256> req;
  req["device_id"] = deviceId();
  if (adcode.length()) req["adcode"] = adcode;
  if (lat != 0 && lon != 0) { req["lat"] = lat; req["lon"] = lon; }
  if (city.length()) req["city"] = city;

  String body;
  serializeJson(req, body);

  String url = String(API_BASE) + "/v1/weather";
  String resp;
  if (!httpPostJson(url, body, resp)) return false;

  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, resp)) return false;

  if (!doc["data"].is<JsonObject>()) return false;
  JsonObject dataObj = doc["data"].as<JsonObject>();

  // chosen_data can be nested; we treat it as object
  if (!dataObj["chosen_data"].is<JsonObject>()) return false;
  JsonObject chosenRoot = dataObj["chosen_data"].as<JsonObject>();
  JsonObject chosen = chosenRoot;
  if (chosenRoot["current"].is<JsonObject>()) chosen = chosenRoot["current"].as<JsonObject>();
  else if (chosenRoot["now"].is<JsonObject>()) chosen = chosenRoot["now"].as<JsonObject>();

  bool filledAny = false;

  // gaode normalized keys
  JsonVariantConst vTemp = chosen["temp"];
  if (vTemp.isNull()) vTemp = chosen["temperature_c"];
  if (vTemp.isNull()) vTemp = chosen["temperature"];
  if (!vTemp.isNull() && (vTemp.is<double>() || vTemp.is<float>() || vTemp.is<long>() || vTemp.is<int>() || vTemp.is<const char*>())) {
    double t = vTemp.is<const char*>() ? atof((const char*)vTemp) : vTemp.as<double>();
    if (!isnan(t)) { data->outdoorTemp = (float)t; filledAny = true; }
  }

  JsonVariantConst vHum = chosen["humidity"];
  if (!vHum.isNull()) {
    double h = vHum.is<const char*>() ? atof((const char*)vHum) : vHum.as<double>();
    if (!isnan(h)) { data->outdoorHumi = (float)h; filledAny = true; }
  }

  // wind level: try common keys (numeric or string)
  {
    long wl = -1;
    JsonVariantConst vWl = chosen["wind_level"];
    if (vWl.isNull()) vWl = chosen["windLevel"];
    if (vWl.isNull()) vWl = chosen["windpower"];
    if (vWl.isNull()) vWl = chosen["wind_power"];
    if (vWl.isNull()) vWl = chosen["windForce"];
    if (tryParseInt(vWl, wl)) {
      if (wl < 0) wl = 0;
      if (wl > 12) wl = 12;
      data->windLevel = (int8_t)wl;
      filledAny = true;
    }
  }

  if (filledAny) {
    Serial.println("[weather] fallback /v1/weather applied (full cache)");
    return true;
  }
  Serial.println("[weather] fallback /v1/weather returned but no fields filled");
  return false;
}

static void fetchWeatherInternal(SensorData *data, int depth) {
  if (!data) return;

  String adcode, city;
  float lat = 0, lon = 0;
  bool hasLoc = loadLoc(adcode, lat, lon, city);

  const bool useWeatherApi = hasLoc;
  String url = String(API_BASE) + (useWeatherApi ? "/v1/device/weather" : "/v1/device/bootstrap");

  Serial.printf("[weather] hasLoc=%d url=%s\n", hasLoc ? 1 : 0, url.c_str());

  StaticJsonDocument<256> req;
  req["device_id"] = deviceId();
  if (hasLoc) {
    req["adcode"] = adcode;
    req["lat"] = lat;
    req["lon"] = lon;
    if (city.length()) req["city"] = city;
  } else {
    if (city.length()) req["city"] = city;
  }

  String body;
  serializeJson(req, body);
  Serial.printf("[weather] req=%s\n", body.c_str());

  String resp;
  if (!httpPostJson(url, body, resp)) {
    Serial.println("[weather] request failed (HTTP code not 200/202)");
    return;
  }

  if (resp.length()) {
    String head = resp;
    if (head.length() > 512) { head = head.substring(0, 512); head += "..."; }
    Serial.printf("[weather] resp(head)=%s\n", head.c_str());
  }

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.printf("[weather] JSON parse error: %s\n", err.c_str());
    return;
  }

  const char *status = doc["status"] | "";
  bool cached = doc["cached"] | false;
  Serial.printf("[weather] status=%s cached=%d\n", status, cached ? 1 : 0);

  bool savedLocNow = false;
  String newAd = "";
  float newLat = 0, newLon = 0;
  String newCity = "";

  if (doc["device"].is<JsonObject>()) {
    newAd = doc["device"]["adcode"] | "";
    newLat = doc["device"]["lat"] | 0;
    newLon = doc["device"]["lon"] | 0;
    newCity = doc["device"]["city"] | "";
    if (newAd.length() && newLat != 0 && newLon != 0) {
      savedLocNow = saveLoc(newAd, newLat, newLon, newCity);
      Serial.printf("[weather] saveLoc=%d adcode=%s lat=%.6f lon=%.6f city=%s\n",
                    savedLocNow ? 1 : 0,
                    newAd.c_str(), (double)newLat, (double)newLon, newCity.c_str());
    }
  }

  // bootstrap -> got location -> immediately fetch /weather
  if (!useWeatherApi && savedLocNow && depth < 2) {
    Serial.println("[weather] bootstrap got location; fetching /weather now...");
    delay(300);
    fetchWeatherInternal(data, depth + 1);
    return;
  }

  if (String(status) == "accepted" && depth < 3) {
    Serial.println("[weather] accepted: server is fetching, retrying soon...");
    delay(1500);
    fetchWeatherInternal(data, depth + 1);
    return;
  }

  if (!doc["weather"].is<JsonObject>()) {
    Serial.println("[weather] missing 'weather' object in response");
    return;
  }

  JsonObjectConst wobj = doc["weather"].as<JsonObjectConst>();
  applyCompactWeatherToSensor(wobj, data);

  // If key fields are still missing, do a one-time fallback call to /v1/weather
  const bool hasTemp = isJsonNumber(wobj["temp"]);
  const bool hasDeg  = isJsonNumber(wobj["wind_deg"]);
  const bool hasWind = isJsonNumber(wobj["wind_speed"]);
  const bool hasWindLevel = isJsonNumber(wobj["wind_level"]);
  const bool hasWeatherText = isJsonString(wobj["weather"]) && String((const char*)wobj["weather"]).length() > 0;

  if (depth < 1 && (!hasTemp || !hasWeatherText || (!hasWind && !hasWindLevel && !hasDeg))) {
    // Use device meta if we just bootstrapped
    String fad = hasLoc ? adcode : newAd;
    float flat = hasLoc ? lat : newLat;
    float flon = hasLoc ? lon : newLon;
    String fcity = hasLoc ? city : newCity;

    if (fad.length() || (flat != 0 && flon != 0)) {
      Serial.println("[weather] compact fields missing; trying fallback /v1/weather...");
      if (tryFallbackFullWeather(fad, flat, flon, fcity, data)) {
        return;
      }
    }
  }
}

static void fetchWeather(SensorData *data) {
  fetchWeatherInternal(data, 0);
}

static void setupNtp() {
  static bool started = false;
  if (started) return;
  started = true;
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
}

static void scheduleLoop(SensorData *data) {
  static int lastHour = -1;
  static bool scheduled = false;
  static uint32_t fireMs = 0;

  time_t now;
  time(&now);
  struct tm t;
  localtime_r(&now, &t);

  if (t.tm_hour != lastHour) {
    lastHour = t.tm_hour;
    scheduled = true;
    uint32_t delaySec = esp_random() % 300;
    fireMs = millis() + delaySec * 1000UL;
  }

  if (scheduled && (int32_t)(millis() - fireMs) >= 0) {
    scheduled = false;
    fetchWeather(data);
  }
}

#endif

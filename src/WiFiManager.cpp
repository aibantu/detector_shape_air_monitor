#include "WiFIManager.h"
// #include <ESPAsyncWebServer.h>
// #include <ESPAsyncDNSServer.h>
#include <Preferences.h>
#include <SPIFFS.h>

IPAddress apIP(192, 168, 4, 1); //固定AP网关IP

//初始化同步服务器
WebServer server(80);
DNSServer dnsServer;

//存储文件
extern "C" Preferences prefs;  // 核心修改：添加 extern 声明
Preferences prefs;
bool configReceived = false;
String savedSSID, savedPASS;

String html = "";

bool isScanning = false;



// 生成WiFi列表网页（极简版）
void generateWiFiListHtml() {
  html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-size:18px;margin:20px;}";
  html += ".wifi-item{margin:10px;padding:10px;border:1px solid #ccc;}";
  html += ".pass-form{display:none;margin:10px 0 20px 20px;}</style>";
  html += "</head><body>";
  html += "<h2>选择你的WiFi</h2>";

  // 扫描WiFi
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).isEmpty()) continue;
    html += "<div class='wifi-item'>";
    html += WiFi.SSID(i) + " (信号：" + WiFi.RSSI(i) + "dBm)";
    html += "<button onclick='showPass(\"" + WiFi.SSID(i) + "\")'>选择</button>";
    html += "<div class='pass-form' id='form_" + WiFi.SSID(i) + "'>";
    html += "<form action='/save' method='GET'>";
    html += "<input type='hidden' name='ssid' value='" + WiFi.SSID(i) + "'>";
    html += "密码：<input type='password' name='pass' required>";
    html += "<input type='submit' value='确定'>";
    html += "</form></div></div>";
  }

  html += "<script>";
  html += "function showPass(ssid) {";
  html += "var forms = document.getElementsByClassName('pass-form');";
  html += "for(var i=0;i<forms.length;i++) forms[i].style.display='none';";
  html += "document.getElementById('form_'+ssid).style.display='block';";
  html += "}";
  html += "</script></body></html>";

//   return html;
}

// String generateWiFiListContent() {
//     String listContent = "";
//     // 同步扫描WiFi（和你原代码一致，保证能拿到列表）
//     int n = WiFi.scanNetworks();
//     if (n == 0) {
//         listContent = "<p>未发现可用WiFi网络</p>";
//         return listContent;
//     }

//     for (int i = 0; i < n; i++) {
//         if (WiFi.SSID(i).isEmpty()) continue;
//         // 转义WiFi名称中的特殊字符（解决引号/空格导致的JS解析错误）
//         String ssid = WiFi.SSID(i);
//         ssid.replace("\"", "\\\""); // 转义双引号
//         ssid.replace("'", "\\'");   // 转义单引号

//         listContent += "<div class='wifi-item'>";
//         listContent += ssid;
//         listContent += "<button onclick='showPass(\"" + ssid + "\")'>选择</button>";
//         listContent += "<div class='pass-form' id='form_" + ssid + "'>";
//         listContent += "<form action='/save' method='GET'>";
//         listContent += "<input type='hidden' name='ssid' value='" + ssid + "'>";
//         listContent += "密码：<input type='password' name='pass' required>";
//         listContent += "<input type='submit' value='确定'>";
//         listContent += "</form></div></div>";
//     }
//     return listContent;
// }

// URL编码辅助函数
String escapeJson(const String& input) {
    String output;
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}

// 生成WiFi列表JSON数据
String getWifiListJson() {
    // 避免并发扫描冲突
    if (isScanning) {
        // 返回扫描中状态，前端可显示加载动画
        return "{\"status\":\"scanning\",\"message\":\"正在扫描WiFi，请稍候\"}";
    }
    isScanning = true;
    String wifiJson = "[";
    // WiFi.mode(WIFI_STA); 

    // WiFi.scanDelete();

    // int scanResult = WiFi.scanNetworks(true, false, false, 3000);

    // unsigned long scanStart = millis();
    // while (WiFi.scanComplete() == -1 && millis() - scanStart < 5000) {
    //     delay(50); // 短延时，让出CPU给Web服务器
    //     server.handleClient();
    // }

    // int networkCount = WiFi.scanComplete();
    int networkCount = WiFi.scanNetworks();

    Serial.print("实际扫描到的网络数量: ");  // 新增调试
    Serial.println(networkCount);  // 新增调试  
    if (networkCount == -2) {
        // 扫描被取消，返回错误        
        wifiJson = "{\"status\":\"error\",\"message\":\"WiFi扫描被取消，请重试\"}";
    } else if (networkCount == 0) {
        // 无可用网络
        wifiJson = "{\"status\":\"empty\",\"message\":\"未扫描到可用WiFi网络\"}";
    } else {
        // 正常扫描结果，生成JSON
    
        for (int i = 0; i < networkCount; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.isEmpty()) continue;
            
            if (i > 0) wifiJson += ",";
            // wifiJson += "{\"ssid\":\"" + escapeJson(WiFi.SSID(i)) + "\",\"rssi\":" + WiFi.RSSI(i) + ",";
            wifiJson += "{";
            wifiJson += "\"ssid\":\"" + escapeJson(ssid) + "\",";
            wifiJson += "\"encrypted\":" + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true") ;
            wifiJson += "}";
        }
        wifiJson += "]";
        // 包装成带状态的JSON，方便前端处理
        wifiJson = "{\"status\":\"success\",\"data\":" + wifiJson + "}";
    }

    WiFi.scanDelete();
    isScanning = false;
    
    return wifiJson;
}

bool loadHtmlFromFile() {

    if (!SPIFFS.exists("/index.html")) {
        Serial.println("index.html does not exist");
        return false;
    }
    
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        Serial.println("Failed to open index.html");
        return false;
    }

    html = file.readString();
    file.close();

    // String wifiListContent = generateWiFiListContent();

    // html.replace("{{WIFI_LIST_CONTENT}}", wifiListContent);

    return true;
}

//DNS劫持和重定向功能
void setupCaptivePortal() {    

    // 定义完整的配置页URL（核心：用完整IP而非相对路径）
    String configUrl = "http://" + apIP.toString() + "";

    //处理根路径请求
    server.on("/", HTTP_GET, [](){        
        server.send(200, "text/html", html);
        server.client().flush();
    });

    server.on("/api/wifilist", HTTP_GET, []() {
        // 设置CORS头，允许前端JavaScript访问
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.sendHeader("Cache-Control", "no-cache"); // 强制前端不缓存
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "0");
        
        String jsonData = getWifiListJson();
        server.send(200, "application/json", jsonData);
        
        Serial.println("发送WiFi列表JSON数据: " + jsonData);
    });

    // 处理配置保存请求
    server.on("/save", HTTP_POST, []() {

        server.sendHeader("Content-Type", "application/json");
        if (server.hasArg("ssid") && server.hasArg("pass")) {
            savedSSID = server.arg("ssid");
            savedPASS = server.arg("pass");
            configReceived = true;
            // server.send(200, "text/plain", "配置成功，设备将重启");
            server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"配置成功，设备将重启\"}");
        } else {
            // server.send(400, "text/plain", "参数错误");
            server.send(200, "application/json", "{\"status\":\"error\", \"message\":\"参数错误\"}");
        }
        server.client().flush();
    });

    //重定向其他Http请求到配置页面
    server.onNotFound([configUrl]() {
        // 容错：忽略非法请求（解决Invalid request错误）

        if (server.client().available()) {
        server.client().flush(); // 清空乱码请求，避免解析错误
        }
        String requestPath = server.uri();
        // 静默处理常见的无效请求
        if (requestPath.equals("/favicon.ico") || 
            requestPath.equals("/apple-touch-icon.png") ||
            requestPath.equals("/robots.txt") ||
            requestPath.startsWith("/.") ||
            requestPath.endsWith(".php") ||
            requestPath.endsWith(".asp")) {
            server.send(404, "text/plain", "");
            return;
        }

        if (server.uri().startsWith("/api/")) {
            server.send(404, "application/json", "{\"status\":\"error\", \"message\":\"API not found\"}");
            return;
        }
        // 重定向到配置页
        server.sendHeader("Location", configUrl, true);
        server.send(302, "text/plain", "Redirecting to config page");
    });
}

bool checkWiFiConfig() {
    prefs.begin("wifi", false);
    bool hasSsid = prefs.isKey("ssid");
    bool hasPass = prefs.isKey("pass");
    String ssid = hasSsid ? prefs.getString("ssid") : "";
    prefs.end();
    ssid.trim();
    return hasSsid && hasPass && !ssid.isEmpty();
}

void startWiFiConnect() {
    prefs.begin("wifi", false);
    String ssid = prefs.getString("ssid");
    String pass = prefs.getString("pass");
    prefs.end();

    ssid.trim();

    if (ssid.isEmpty()) {
        Serial.println("ssid is empty");
        return;
        
    } 
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Connecting to WiFi: " + ssid);    
}

// 阻塞屏幕显示
bool autoConnectWiFi() {
    prefs.begin("wifi", false);
    String ssid = prefs.getString("ssid");
    String pass = prefs.getString("pass");
    prefs.end();

    if (ssid.isEmpty()) return false;

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    // 优化连接逻辑：最多尝试5次，每次等待5秒
    for (int attempt = 0; attempt < 5; attempt++) {
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 50) { // 5秒超时
            delay(100);
            timeout++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        // 连接失败，重试前断开
        WiFi.disconnect();
        delay(500);
        WiFi.begin(ssid.c_str(), pass.c_str());
    }
    // int timeout = 0;
    // while (WiFi.status() != WL_CONNECTED && timeout < 10) {
    //     delay(1000);
    //     timeout++;
    // }
   
    return WiFi.status() == WL_CONNECTED;
}

void startConfigAP() {
    // generateWiFiListHtml();

    if (!loadHtmlFromFile()) {
        Serial.println("Fallback to default HTML");
        // 加载默认极简HTML（避免空白页面）
        generateWiFiListHtml();
    }

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, "");

    //配置DNS服务器，将所有域名解析到AP的IP地址
    if (dnsServer.start(53, "*", apIP)) {
        Serial.println("DNS服务器启动成功");
    } else {
        Serial.println("DNS服务器启动失败");
    }



    //设置强制门户
    setupCaptivePortal();
    
    server.begin();
}

bool waitForWiFiConfig() {
    unsigned long start = millis();
    while (!configReceived && millis() - start < 120000) { // 2分钟超时
        // vTaskDelay(pdMS_TO_TICKS(100));
        dnsServer.processNextRequest(); // 处理DNS请求
        server.handleClient();          // 处理HTTP请求
        delay(100);
    }
    if (configReceived) {
        saveWiFiConfig(savedSSID.c_str(), savedPASS.c_str());
        dnsServer.stop();
        server.stop();
        return true;
    }
    return false;
}

void saveWiFiConfig(const char* ssid, const char* pass) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void clearWiFiConfig() {
    prefs.begin("wifi", false);
    prefs.clear();  // 清空WiFi命名空间
    prefs.end();
    Serial.println("WiFi配置已清除！");
    delay(1000);
    // ESP.restart();
}
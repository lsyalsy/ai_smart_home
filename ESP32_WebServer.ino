/*
 * ESP32 轻量级 Web 服务器（手机浏览器访问）
 * --------------------------------------------------
 * 功能：
 *   1. 连接 WiFi（失败时自动切换为 AP 热点模式）
 *   2. 提供 Web 控制面板，手机浏览器访问即可：
 *        - 点亮 / 熄灭板载 LED
 *        - 实时查看 GPIO 状态与模拟输入
 *        - 简单鉴权（可选）
 *   3. 支持 mDNS：手机可直接访问 http://esp32.local
 *
 * 烧录工具：Arduino IDE / PlatformIO
 * 依赖库  ：无需第三方库（使用 ESP32 Arduino Core 自带 WebServer）
 *
 * 接线说明：
 *   - 板载 LED 多为 GPIO2（部分板子为内置 LED，无需接线）
 *   - 模拟输入示例：电位器接 GPIO34（ADC1 通道）
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ====================== 配置区 ======================
const char* WIFI_SSID     = "YOUR_SSID";        // 替换为你的 WiFi 名称
const char* WIFI_PASSWORD = "YOUR_PASSWORD";    // 替换为你的 WiFi 密码

const char* AP_SSID       = "ESP32_Setup";      // AP 模式热点名称
const char* AP_PASSWORD   = "12345678";         // AP 模式密码（至少 8 位）

const char* HOST_NAME     = "esp32";            // mDNS 主机名 -> http://esp32.local
const char* HTTP_USER     = "admin";            // Basic Auth 用户名（留空则不鉴权）
const char* HTTP_PASS     = "admin";            // Basic Auth 密码

const uint8_t LED_PIN     = 2;                  // 板载 LED 引脚
const uint8_t ANALOG_PIN  = 34;                 // 模拟输入引脚（ADC1）
// ===================================================

WebServer server(80);
bool ledState = false;
unsigned long startMillis = 0;

// ---------- 工具函数：鉴权 ----------
bool checkAuth() {
  if (strlen(HTTP_USER) == 0) return true;            // 未设置用户名则跳过
  return server.authenticate(HTTP_USER, HTTP_PASS);
}

// ---------- 首页 HTML ----------
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ESP32 控制面板</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
  body {
    font-family: -apple-system, "PingFang SC", "Microsoft YaHei", sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    padding: 20px;
    color: #333;
  }
  .card {
    background: #fff;
    border-radius: 16px;
    padding: 24px 20px;
    margin-bottom: 18px;
    box-shadow: 0 10px 30px rgba(0,0,0,.15);
  }
  h1 { font-size: 22px; text-align: center; margin-bottom: 4px; color: #2c3e50; }
  .sub { text-align: center; font-size: 12px; color: #999; margin-bottom: 18px; }
  .row { display: flex; align-items: center; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #f0f0f0; }
  .row:last-child { border-bottom: none; }
  .label { font-size: 15px; color: #555; }
  .value { font-size: 15px; font-weight: 600; color: #2c3e50; }
  .btn-group { display: flex; gap: 12px; margin-top: 12px; }
  .btn {
    flex: 1; padding: 14px 0; font-size: 16px; border: none; border-radius: 12px;
    color: #fff; cursor: pointer; transition: transform .1s;
  }
  .btn:active { transform: scale(0.97); }
  .btn-on  { background: linear-gradient(135deg, #11998e, #38ef7d); }
  .btn-off { background: linear-gradient(135deg, #eb3349, #f45c43); }
  .dot { width: 14px; height: 14px; border-radius: 50%; display: inline-block; margin-right: 6px; vertical-align: middle; }
  .dot.on  { background: #38ef7d; box-shadow: 0 0 8px #38ef7d; }
  .dot.off { background: #ccc; }
  .footer { text-align: center; color: rgba(255,255,255,.8); font-size: 12px; margin-top: 10px; }
</style>
</head>
<body>
  <div class="card">
    <h1>ESP32 控制面板</h1>
    <div class="sub">轻量级 Web 服务器</div>

    <div class="row">
      <span class="label">LED 状态</span>
      <span class="value"><span id="dot" class="dot off"></span><span id="ledTxt">未知</span></span>
    </div>
    <div class="row">
      <span class="label">模拟输入 (GPIO34)</span>
      <span class="value" id="adc">--</span>
    </div>
    <div class="row">
      <span class="label">运行时长</span>
      <span class="value" id="uptime">--</span>
    </div>
    <div class="row">
      <span class="label">内存剩余</span>
      <span class="value" id="heap">--</span>
    </div>

    <div class="btn-group">
      <button class="btn btn-on"  onclick="ctrl('on')">点亮 LED</button>
      <button class="btn btn-off" onclick="ctrl('off')">熄灭 LED</button>
    </div>
  </div>

  <div class="footer">ESP32 · Powered by Arduino Core</div>

<script>
function refresh() {
  fetch('/status').then(r => r.json()).then(d => {
    document.getElementById('ledTxt').textContent = d.led ? '点亮' : '熄灭';
    document.getElementById('dot').className = 'dot ' + (d.led ? 'on' : 'off');
    document.getElementById('adc').textContent = d.adc + ' / 4095';
    document.getElementById('uptime').textContent = d.uptime + ' 秒';
    document.getElementById('heap').textContent = d.heap + ' B';
  }).catch(() => {});
}
function ctrl(cmd) {
  fetch('/' + cmd).then(() => refresh());
}
refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)HTML";

// ---------- 路由处理 ----------
void handleRoot() {
  if (!checkAuth()) return server.requestAuthentication();
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleLedOn() {
  if (!checkAuth()) return server.requestAuthentication();
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/plain", "LED ON");
}

void handleLedOff() {
  if (!checkAuth()) return server.requestAuthentication();
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "LED OFF");
}

void handleStatus() {
  if (!checkAuth()) return server.requestAuthentication();
  String json = "{";
  json += "\"led\":" + String(ledState ? "true" : "false") + ",";
  json += "\"adc\":" + String(analogRead(ANALOG_PIN)) + ",";
  json += "\"uptime\":" + String((millis() - startMillis) / 1000) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap());
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "404 Not Found");
}

// ---------- WiFi 连接 ----------
bool connectWiFi() {
  Serial.printf("[WiFi] 正在连接 %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] 已连接，IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("[WiFi] 连接失败，切换到 AP 模式");
  return false;
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("[AP] 热点已启动: ");
  Serial.print(AP_SSID);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());
}

// ---------- 初始化 ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n==== ESP32 轻量级 Web 服务器 ====");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  startMillis = millis();

  if (!connectWiFi()) {
    startAP();
  }

  // mDNS：可通过 http://esp32.local 访问
  if (MDNS.begin(HOST_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mDNS] 启动成功: http://%s.local\n", HOST_NAME);
  }

  // 注册路由
  server.on("/",        HTTP_GET, handleRoot);
  server.on("/on",      HTTP_GET, handleLedOn);
  server.on("/off",     HTTP_GET, handleLedOff);
  server.on("/status",  HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("[HTTP] 服务器已启动，端口 80");
}

// ---------- 主循环 ----------
void loop() {
  server.handleClient();

  // WiFi 断线自动重连
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] 连接丢失，尝试重连...");
      WiFi.reconnect();
    }
  }
}

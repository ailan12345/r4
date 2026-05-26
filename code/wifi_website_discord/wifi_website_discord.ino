#include "WiFi.h"
#include "DHT.h"
#include "Arduino_LED_Matrix.h"

// 設定 Wi-Fi 帳號密碼
const char* ssid = "SSIDSSID";
const char* password = "password";

// 設定 Discord Webhook 資訊
const char* discord_server = "discord.com";
const char* discord_path = "/api/webhooks/123456790/dd-aa-cc-bb";


// 設定定時發送時間 (每 30 分鐘發送一次)
unsigned long lastDiscordTime = 0;
const unsigned long discordInterval = 1 * 60 * 1000;

// 實例網頁伺服器服務
WiFiServer server(80);  // 連接埠 80（標準網頁 HTTP 通訊埠）
// 實例 LED 矩陣
ArduinoLEDMatrix matrix;

// LED 矩陣圖案
const uint32_t wifi_ok[] = {
  0x200,
  0x40082101,
  0x200c0000,
  66
};

// 溫溼度感測 腳位設定
#define DHTPIN 2
#define DHTTYPE DHT11

// 實例感測器物件
DHT dht(DHTPIN, DHTTYPE);

// 發送 HTTPS POST 請求至 Discord
void sendToDiscord(float temp, float humi, String triggerType) {
  WiFiSSLClient sslClient;  // 實例 WiFi 專用安全連線客戶端

  Serial.println("正在連線至 Discord 伺服器...");

  // Discord 預設 HTTPS 埠號為 443
  if (sslClient.connect(discord_server, 443)) {
    Serial.println("連線成功，正在組成 JSON 數據...");

    // 建立 Discord 要求的 JSON 內容
    // Discord developers doc https://docs.discord.com/developers/resources/message#embed-object
    String jsonPayload = "{\"content\": \" \", \"embeds\": [{"
                         "\"title\": \"溫濕度數據報告 ("
                         + triggerType + ")\","
                                         "\"color\": 15258703,"
                                         "\"fields\": ["
                                         "{\"name\": \"溫度\", \"value\": \""
                         + String(temp, 1) + " °C\", \"inline\": true},"
                                             "{\"name\": \"濕度\", \"value\": \""
                         + String(humi, 1) + " %\", \"inline\": true}"
                                             "],"
                                             "\"footer\": {\"text\": \"Arduino R4 WiFi 自動推送\"}"
                                             "}]}";

    // 手動發送標準 HTTP POST 請求標頭
    sslClient.print("POST ");
    sslClient.print(discord_path);
    sslClient.println(" HTTP/1.1");

    sslClient.print("Host: ");
    sslClient.println(discord_server);

    sslClient.println("Content-Type: application/json");
    sslClient.println("Connection: close");

    sslClient.print("Content-Length: ");
    sslClient.println(jsonPayload.length());

    sslClient.println();             // 標頭結束空行
    sslClient.println(jsonPayload);  // 發送主體

    Serial.println("數據已送出，正在等待回應...");

    // 讀取伺服器回傳狀態（除錯用）
    while (sslClient.connected()) {
      String line = sslClient.readStringUntil('\n');
      if (line == "\r") {
        break;  // 讀到空行代表標頭結束
      }
      if (line.startsWith("HTTP/1.1")) {
        Serial.print("Discord 伺服器回應: ");
        Serial.println(line);
      }
    }
    sslClient.stop();  // 斷開連線
  } else {
    Serial.println("無法連線到 Discord 伺服器 (HTTPS 連線失敗)");
  }
}

// Wi-Fi 連線函式
void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  Serial.print("正在連接到 Wi-Fi: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  while (WiFi.localIP() == INADDR_NONE || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    Serial.print("等待 IP...");
    delay(1000);
  }
  Serial.println("\nWi-Fi 連線成功！");
  Serial.print("Arduino IP: ");
  Serial.println(WiFi.localIP());

  matrix.loadFrame(wifi_ok);
}


// 初始化設定
void setup() {
  Serial.begin(115200);  // 通訊速率
  matrix.begin();        // 初始化啟動 LED 矩陣
  dht.begin();           // 初始化啟動 DHT11 溫溼度感測器
  setup_wifi();
  server.begin();  // 初始化啟動網頁伺服器服務

  // 開機成功後先發送一次測試
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    sendToDiscord(t, h, "我開機了~");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDiscordTime >= discordInterval) {
    lastDiscordTime = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      sendToDiscord(t, h, "定時報告");
    }
  }


  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    // 💡 處理背景數據更新
    if (request.indexOf("GET /data") >= 0) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (isnan(h) || isnan(t)) {
        t = 0.0;
        h = 0.0;
      }

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"temp\":");
      client.print(t);
      client.print(",\"humi\":");
      client.print(h);
      client.print("}");
    }
    // 處理網頁端按鈕點擊
    else if (request.indexOf("GET /trigger_discord") >= 0) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (isnan(h) || isnan(t)) {
        t = 0.0;
        h = 0.0;
      }

      sendToDiscord(t, h, "手動觸發");

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"status\":\"success\"}");
    }
    // 如果是一般瀏覽網頁 (GET /)
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=utf-8");
      client.println("Connection: close");
      client.println();

      client.println("<!DOCTYPE html><html>");
      client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1' charset='utf-8'>");
      client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
      client.println(".box { padding: 20px; font-size: 30px; margin: 20px; border-radius: 10px; color: white; }");
      client.println(".temp { background-color: #ff9800; }");
      client.println(".humi { background-color: #00bcd4; }");
      client.println(".btn { background-color: #7289da; color: white; padding: 15px 25px; font-size: 18px; border: none; border-radius: 5px; cursor: pointer; transition: 0.3s; margin-top: 10px;}");
      client.println(".btn:hover { background-color: #5b73c7; }</style>");

      client.println("<script>");
      client.println("setInterval(function() {");
      client.println("  fetch('/data').then(response => response.json()).then(data => {");
      client.println("    document.getElementById('t_val').innerText = data.temp;");
      client.println("    document.getElementById('h_val').innerText = data.humi;");
      client.println("  }).catch(err => console.log(err));");
      client.println("}, 2000);");

      client.println("function triggerDiscord() {");
      client.println("  var btn = document.getElementById('dc_btn');");
      client.println("  btn.innerText = '發送中...'; btn.disabled = true;");
      client.println("  fetch('/trigger_discord').then(res => res.json()).then(data => {");
      client.println("    btn.innerText = '發送成功！';");
      client.println("    setTimeout(() => { btn.innerText = 'POST Discord'; btn.disabled = false; }, 3000);");
      client.println("  }).catch(err => { alert('發送失敗'); btn.innerText = 'POST Discord'; btn.disabled = false; });");
      client.println("}");
      client.println("</script>");
      client.println("</head>");

      client.println("<body><h1>Arduino R4 溫濕度計</h1>");
      client.println("<div class='box temp'>溫度: <span id='t_val'>--.-</span> °C</div>");
      client.println("<div class='box humi'>濕度: <span id='h_val'>--.-</span> %</div>");
      client.println("<button id='dc_btn' class='btn' onclick='triggerDiscord()'>POST Discord</button>");
      client.println("</body></html>");
    }

    client.stop();
  }
}

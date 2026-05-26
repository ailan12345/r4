#include "WiFi.h"
#include "DHT.h"
#include "Arduino_LED_Matrix.h"

// 設定 Wi-Fi 帳號密碼
const char* ssid = "SSIDSSID";
const char* password = "password";


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
  server.begin();    // 初始化啟動網頁伺服器服務
}

void loop() {

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
      client.println("</script>");
      client.println("</head>");

      client.println("<body><h1>Arduino R4 溫濕度計</h1>");
      client.println("<div class='box temp'>溫度: <span id='t_val'>--.-</span> °C</div>");
      client.println("<div class='box humi'>濕度: <span id='h_val'>--.-</span> %</div>");
      client.println("</body></html>");
    }

    client.stop();
  }
}

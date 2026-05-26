#include "WiFi.h"
#include <PubSubClient.h>
#include "DHT.h"
#include "Arduino_LED_Matrix.h"

// Wi-Fi
const char* ssid = "SSIDSSID";
const char* password = "password";

// MQTT 設定
const char* mqtt_server = "broker.hivemq.com";  // 公共 MQTT 伺服器
const int mqtt_port = 1883;

// MQTT 主題名稱
const char* topic_pub = "/ailan/20260517/temp/humi";    // 發布（上傳）
const char* topic_sub = "/ailan/20260517/led/control";  // 訂閱（接收）

// 溫溼度感測 腳位設定
#define DHTPIN 2
#define DHTTYPE DHT11

// 實例感測器物件
DHT dht(DHTPIN, DHTTYPE);

// 實例 LED 矩陣
ArduinoLEDMatrix matrix;

// 實例網路
WiFiClient espClient;

// 將網路物件傳入 MQTT 函式庫
PubSubClient client(espClient);

// 紀錄上一次發送訊息的時間
unsigned long lastMsg = 0;

// LED 矩陣圖案
const uint32_t led_on[] = {
  0xfff801bf,
  0xda05a05b,
  0xfd801fff,
  66
};
const uint32_t led_off[] = { 0x0, 0x0, 0x0, 63 };


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
}

// MQTT 訊息接收回呼函式
// topic：收到訊息的主題名稱
// payload：收到的訊息內容（原始資料為 Byte 陣列）
// length：訊息的長度
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == topic_sub) {
    if (message == "1") {
      matrix.loadFrame(led_on);
    } else if (message == "0") {
      matrix.loadFrame(led_off);
    }
  }
}

// MQTT 斷線重連函式
void reconnect() {
  while (!client.connected()) {
    // 利用隨機亂數（random）產生一個隨機的客戶端 ID（例如：ArduinoR4Client-a3f2）。
    // 這是因為 MQTT 伺服器規定不允許兩個裝置使用同一個 ID 連線，否則會互相踢下線。
    String clientId = "ArduinoR4Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      // 連線成功後，必須強迫重新訂閱控制主題
      client.subscribe(topic_sub);
    } else {
      delay(5000);
    }
  }
}


// 初始化設定
void setup() {
  Serial.begin(115200);  // 通訊速率
  matrix.begin();        // 初始化啟動 LED 矩陣
  dht.begin();           // 初始化啟動 DHT11 溫溼度感測器
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);  // 綁定回呼邏輯
}

void loop() {
  //每次循環一開頭，先檢查 MQTT 連線是否正常
  if (!client.connected()) {
    reconnect();
  }

  // MQTT 的核心心跳。它必須在 loop 裡高頻率被執行，負責在背景維持與伺服器的連線、發送心跳包、並檢查有沒有新訊息。
  client.loop();

  // 取得開機至今的毫秒數
  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      String payload = "{\"temp\":" + String(t, 1) + ",\"humi\":" + String(h, 0) + "}";
      client.publish(topic_pub, payload.c_str());
    }
  }
}
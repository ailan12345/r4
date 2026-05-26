#include "WiFi.h"
#include "Arduino_LED_Matrix.h"

// 設定 Wi-Fi 帳號密碼
const char* ssid = "SSIDSSID";
const char* password = "password";

// 實例 LED 矩陣
ArduinoLEDMatrix matrix;

// LED 矩陣圖案
const uint32_t wifi_ok[] = {
    0x200,
    0x40082101,
    0x200c0000,
    66
};

// Wi-Fi 連線函式
void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  Serial.print("正在連接到 Wi-Fi: ");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print(".");}
  while (WiFi.localIP() == INADDR_NONE || WiFi.localIP() == IPAddress(0,0,0,0)) {
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
  Serial.begin(115200);
  matrix.begin();
  setup_wifi();
}

void loop() {

}
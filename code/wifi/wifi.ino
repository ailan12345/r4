#include "WiFi.h"

// 設定 Wi-Fi 帳號密碼
const char* ssid = "SSIDSSID";
const char* password = "password";

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
}

// 初始化設定
void setup() {
  Serial.begin(115200);
  setup_wifi();

}

void loop() {

}
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>
#include "types.h"
#include "web_interface.h"
#include "deauth.h"
#include "definitions.h"
#include "applejuice.h"
#include "rgb.h"
// // WiFi 凭据
// const char* ssid = "HUAWEI"; // 替换为你的 WiFi SSID
// const char* password = "12345678"; // 替换为你的 WiFi 密码

// 服务器实例
WebServer server(80);
WebSocketsServer webSocket(81);

// NeoPixel 配置
#define LED_PIN 48     // RGB LED 的数据引脚，需根据硬件调整
#define NUM_LEDS 1     // LED 数量
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 全局变量
int curr_channel = 1;

void setup() {
  Serial.begin(115200);

  // 初始化 NeoPixel
  strip.begin();
  strip.show();
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // 初始红色
  strip.setBrightness(10);

  applejuice_init();
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  // 初始化功能
  applejuice_init();
  start_web_interface();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent); // 定义在 rgb.cpp 中
}

void loop() {
  if (deauth_type == DEAUTH_TYPE_ALL) {
    if (curr_channel > CHANNEL_MAX) curr_channel = 1;
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
    curr_channel++;
    delay(10);
  } else {
    web_interface_handle_client();
    web_interface_handle_client();
  }
}
#include "spiffs_cmp.h"

#include <ArduinoJson.h>

String wifi_ssid = "";
String wifi_pass = "";

void SPIFFS_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS: Error Mount SPIFFS");
    return;
  }
}

void SPIFFS_getWiFiCred() {
  //* WiFi Setup Configuration
  String wifi_buffer;
  DynamicJsonDocument wifi_cred(256);

  File file = SPIFFS.open("/wifi_cred.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return;
  }

  while (file.available()) {
    wifi_buffer += file.readString();
  }

  deserializeJson(wifi_cred, wifi_buffer);

  wifi_ssid = wifi_cred["ssid"].as<String>();
  wifi_pass = wifi_cred["pass"].as<String>();

  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);
}
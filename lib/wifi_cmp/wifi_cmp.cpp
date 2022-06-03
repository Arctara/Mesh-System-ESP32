#include "wifi_cmp.h"

//* Static IP Configuration
IPAddress IP_ADDRESS(192, 168, 5, 1);
IPAddress GATEWAY(192, 168, 5, 1);
IPAddress NETMASK(255, 255, 255, 0);

bool isOfflineMode = false;

void WIFI_initStation(String ssid, String pass) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println();
  Serial.print("WiFi Station: Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() <= 15000) {
    Serial.print(".");
    delay(300);
  }
}

bool WIFI_isOfflineMode() { return isOfflineMode; }

void WIFI_setOfflineMode(bool newCondition) { isOfflineMode = newCondition; }

void WIFI_printOfflineMessage() {
  Serial.println(
      "GLOBAL: Tidak dapat terhubung dengan konfigurasi WiFi yang "
      "diberikan.");
  Serial.println("GLOBAL: Beralih ke mode Offline.");
  Serial.println();
}

void WIFI_printStationIP() {
  Serial.print("WiFi Station: Terhubung ke WiFi dengan IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void WIFI_initAP() {
  WiFi.softAPConfig(IP_ADDRESS, GATEWAY, NETMASK);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_DISCOVERABLE, AP_MAX_CONNECTION);
}

void WIFI_printAPIP() {
  Serial.print("WiFi AP: IP Address Access Point: ");
  Serial.println(WiFi.softAPIP());
}
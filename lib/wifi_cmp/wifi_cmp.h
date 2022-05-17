#ifndef WIFI_CMP
#define WIFI_CMP

#include <WiFi.h>

//$ Access Point Configuration
#define AP_SSID "ALiVe_AP"
#define AP_PASS "LeTS_ALiVe"
#define AP_CHANNEL 1
#define AP_DISCOVERABLE 0
#define AP_MAX_CONNECTION 10

void WIFI_initStation(String ssid, String pass);
bool WIFI_isOfflineMode();
void WIFI_printOfflineMessage();
void WIFI_printStationIP();
void WIFI_initAP();

#endif
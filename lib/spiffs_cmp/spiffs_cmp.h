#ifndef SPIFFS_CMP
#define SPIFFS_CMP

#include <SPIFFS.h>

extern String wifi_ssid;
extern String wifi_pass;

void SPIFFS_init();
void SPIFFS_getWiFiCred();

#endif
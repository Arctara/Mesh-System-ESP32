#ifndef SPIFFS_CMP
#define SPIFFS_CMP

#include <SPI.h>
#include <SPIFFS.h>

extern String wifi_ssid;
extern String wifi_pass;

void SPIFFS_init();
void SPIFFS_initWiFiFile();

void SPIFFS_getWiFiCred();

void SPIFFS_setWiFiCred(String ssid, String pass);

#endif
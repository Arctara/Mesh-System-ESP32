#ifndef SPIFFS_CMP
#define SPIFFS_CMP

#include <SPI.h>
#include <SPIFFS.h>

extern String wifi_ssid;
extern String wifi_pass;

void SPIFFS_init();
void SPIFFS_initWiFiFile();
void SPIFFS_initLampData();
void SPIFFS_initPlugData();
void SPIFFS_initScheduleData();
void SPIFFS_initSensorData();

void SPIFFS_getWiFiCred();
void SPIFFS_setWiFiCred(String ssid, String pass);

String SPIFFS_getLampData();
void SPIFFS_setLampData(String source);

String SPIFFS_getPlugData();
void SPIFFS_setPlugData(String source);

String SPIFFS_getScheduleData();
void SPIFFS_setScheduleData(String source);

String SPIFFS_getSensorData();
void SPIFFS_setSensorData(String source);

#endif
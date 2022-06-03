#include "system_cmp.h"

void SYSTEM_init() { Serial.begin(115200); }

void SYSTEM_firstRunWizard() {
  SPIFFS_initWiFiFile();
  SPIFFS_initLampData();
  SPIFFS_initPlugData();
  SPIFFS_initScheduleData();
  SPIFFS_initSensorData();
}

void SYSTEM_restart() { ESP.restart(); }
#include "system_cmp.h"

void SYSTEM_init() { Serial.begin(115200); }

void SYSTEM_firstRunWizard() {
  // initWiFiFile();
  // initMainFile();
}

void SYSTEM_restart() { ESP.restart(); }
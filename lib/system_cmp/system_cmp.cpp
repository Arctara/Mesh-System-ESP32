#include "system_cmp.h"

void SYSTEM_init() {
  Serial.begin(115200);
  pinMode(ORANGE_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
}

void SYSTEM_firstRunWizard() {
  SPIFFS_initWiFiFile();
  SPIFFS_initLampData();
  SPIFFS_initPlugData();
  SPIFFS_initScheduleData();
  SPIFFS_initSensorData();
}

void SYSTEM_restart() { ESP.restart(); }

void SYSTEM_blinkLED(int ledPin) {
  digitalWrite(ledPin, HIGH);
  delay(50);
  digitalWrite(ledPin, LOW);
}

void SYSTEM_turnLED(int ledPin, bool condition) {
  digitalWrite(ledPin, condition);
}
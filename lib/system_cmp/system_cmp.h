#ifndef SYSTEM_CMP
#define SYSTEM_CMP

#include <Arduino.h>

#include "spiffs_cmp.h"
#include "global_cmp.h"

void SYSTEM_init();
void SYSTEM_firstRunWizard();
void SYSTEM_restart();
void SYSTEM_blinkLED(int ledPin);
void SYSTEM_turnLED(int ledPin, bool condition);

#endif
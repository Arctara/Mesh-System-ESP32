#ifndef SYSTEM_CMP
#define SYSTEM_CMP

#include <Arduino.h>

#include "spiffs_cmp.h"

void SYSTEM_init();
void SYSTEM_firstRunWizard();
void SYSTEM_restart();

#endif
#ifndef EEPROM_CMP
#define EEPROM_CMP

#include <EEPROM.h>

#include "system_cmp.h"

//$ Define EEPROM Size
#define EEPROM_SIZE 512
//$ Config Address Saved in EEPROM
#define HAS_INIT 0

void EEPROM_init();
void EEPROM_isFirstRun();

#endif
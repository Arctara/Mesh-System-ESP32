#include "eeprom_cmp.h"

void EEPROM_init() { EEPROM.begin(EEPROM_SIZE); }

void EEPROM_isFirstRun() {
  char hasInit = EEPROM.read(HAS_INIT);
  if (hasInit == 255) {
    Serial.println("Initializing Everything");

    SYSTEM_firstRunWizard();

    EEPROM.write(HAS_INIT, 1);
    EEPROM.commit();
  }
}
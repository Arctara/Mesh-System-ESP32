#include "global_cmp.h"

String lampLoc = "homes/" + WiFi.macAddress() + "/lamps";
String sensorLoc = "homes/" + WiFi.macAddress() + "/sensors";
String plugLoc = "homes/" + WiFi.macAddress() + "/plugs";

String currentLightReading = "";
String currentMovementReading = "";
String currentMoistureReading = "";

unsigned long prevMovementMillis = 0;
unsigned long interval = 60000;
unsigned long prevInterval = 0;
bool hasExtended = false;

//* Global Function Definition
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
#include "time_cmp.h"

RTC_DS3231 rtc;

void TIME_init() {
  if (!rtc.begin()) {
    Serial.println("GLOBAL: RTC Not Connected");
  }
}

DateTime TIME_now() { return rtc.now(); }
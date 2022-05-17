#include "time_cmp.h"

RTC_DS3231 rtc;

unsigned long prevMillis = 0;

void TIME_init() {
  if (!rtc.begin()) {
    Serial.println("GLOBAL: RTC Not Connected");
  }
}

DateTime TIME_now() { return rtc.now(); }

bool TIME_tick(unsigned long duration) {
  return millis() - prevMillis >= duration;
}

void TIME_update() { prevMillis = millis(); }

void TIME_printClock() {
  Serial.println("================================================");
  Serial.println(String(rtc.now().hour()) + " : " + String(rtc.now().minute()));
  Serial.println("================================================");
}
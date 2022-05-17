#ifndef SCHEDULE_CMP
#define SCHEDULE_CMP

#include <Arduino.h>
#include <ArduinoJson.h>

#include <List.hpp>

#include "global_cmp.h"

struct schedule {
  String id;
  bool active = false;
  bool subActive = false;
  bool moreSubActive = false;
  String target;
  String targetId;
  String trigger;
  String timeMode;
  String startHour;
  String startMinute;
  String stopHour;
  String stopMinute;
  String lengthOn;
  String lengthOff;
  String delay;
  String sensorId;
  String activeCondition;
  unsigned long prevMillisOn = 0;
  unsigned long prevMillisOff = 0;
};

extern List<schedule> schedules;

void SCHEDULE_build(String source);

#endif
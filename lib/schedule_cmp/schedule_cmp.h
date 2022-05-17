#ifndef SCHEDULE_CMP
#define SCHEDULE_CMP

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>

#include <List.hpp>

#include "firebase_cmp.h"
#include "global_cmp.h"
#include "time_cmp.h"
#include "websocket_cmp.h"
#include "wifi_cmp.h"

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
void SCHEDULE_update(int index, schedule schedule);
void SCHEDULE_turnDevice(schedule schedule, bool condition);

bool SCHEDULE_isTimeBased(schedule schedule);
bool SCHEDULE_isInterval(schedule schedule);
bool SCHEDULE_isDelay(schedule schedule);

bool SCHEDULE_inTime(schedule schedule);
bool SCHEDULE_outTime(schedule schedule);

bool SCHEDULE_isActive(schedule schedule);

bool SCHEDULE_isTargetLamp(schedule schedule);
bool SCHEDULE_isTargetPlug(schedule schedule);

bool SCHEDULE_isSubActive(schedule schedule);
bool SCHEDULE_isMoreSubActive(schedule schedule);

bool SCHEDULE_isInLengthOn(schedule schedule);
bool SCHEDULE_isInLengthOff(schedule schedule);

bool SCHEDULE_isOutDelayTime(schedule schedule);

void SCHEDULE_checkIfDelayable(String deviceId);
void SCHEDULE_cancelDelay(String deviceId);

void SCHEDULE_removeAll();

#endif
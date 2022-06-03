#ifndef SCHEDULE_CMP
#define SCHEDULE_CMP

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <ESPAsyncWebServer.h>

#include <List.hpp>

#include "firebase_cmp.h"
#include "global_cmp.h"
#include "time_cmp.h"
#include "spiffs_cmp.h"
#include "websocket_cmp.h"
#include "wifi_cmp.h"

struct scheduleData {
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

extern List<scheduleData> schedules;

void SCHEDULE_build(String source);
void SCHEDULE_update(int index, scheduleData schedule);
void SCHEDULE_turnDevice(scheduleData schedule, bool condition);

bool SCHEDULE_isTimeBased(scheduleData schedule);
bool SCHEDULE_isInterval(scheduleData schedule);
bool SCHEDULE_isDelay(scheduleData schedule);

bool SCHEDULE_inTime(scheduleData schedule);
bool SCHEDULE_outTime(scheduleData schedule);

bool SCHEDULE_isActive(scheduleData schedule);

bool SCHEDULE_isTargetLamp(scheduleData schedule);
bool SCHEDULE_isTargetPlug(scheduleData schedule);

bool SCHEDULE_isSubActive(scheduleData schedule);
bool SCHEDULE_isMoreSubActive(scheduleData schedule);

bool SCHEDULE_isInLengthOn(scheduleData schedule);
bool SCHEDULE_isInLengthOff(scheduleData schedule);

bool SCHEDULE_isOutDelayTime(scheduleData schedule);

void SCHEDULE_checkIfDelayable(String deviceId);
void SCHEDULE_cancelDelay(String deviceId);

void SCHEDULE_removeAll();

#endif
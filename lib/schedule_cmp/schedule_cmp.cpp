#include "schedule_cmp.h"

List<scheduleData> schedules;

void SCHEDULE_build(String source) {
  schedules.clear();
  schedules.removeAll();
  DynamicJsonDocument listJson(1024);
  DynamicJsonDocument sD(1024);
  deserializeJson(listJson, source);
  JsonArray arrays = listJson.as<JsonArray>();
  listJson.clear();
  int i = 0;
  for (JsonVariant v : arrays) {
    if (v.as<String>() != "null") {
      scheduleData s;
      deserializeJson(sD, v.as<String>());
      s.id = String(i);
      s.target = sD["target"].as<String>();
      s.targetId = sD["targetId"].as<String>();
      s.socketId = sD["socketId"].as<String>();
      s.trigger = sD["trigger"].as<String>();
      if (s.trigger == "time") {
        s.timeMode = sD["timeMode"].as<String>();
        if (s.timeMode == "timeBased") {
          s.startHour = getValue(sD["timeStart"].as<String>(), ':', 0);
          s.startMinute = getValue(sD["timeStart"].as<String>(), ':', 1);
          s.stopHour = getValue(sD["timeStop"].as<String>(), ':', 0);
          s.stopMinute = getValue(sD["timeStop"].as<String>(), ':', 1);
        } else if (s.timeMode == "interval") {
          s.startHour = getValue(sD["timeStart"].as<String>(), ':', 0);
          s.startMinute = getValue(sD["timeStart"].as<String>(), ':', 1);
          s.stopHour = getValue(sD["timeStop"].as<String>(), ':', 0);
          s.stopMinute = getValue(sD["timeStop"].as<String>(), ':', 1);
          s.lengthOn = sD["lengthOn"].as<String>();
          s.lengthOff = sD["lengthOff"].as<String>();
        } else if (s.timeMode == "delay") {
          s.delay = sD["delay"].as<String>();
        }
      } else if (s.trigger == "light" || s.trigger == "movement" ||
                 s.trigger == "moisture") {
        s.sensorId = sD["sensorId"].as<String>();
        s.activeCondition = sD["activeCondition"].as<String>();
      }
      schedules.add(s);
      sD.clear();
    }
    i++;
  }
}

void SCHEDULE_update(int index, scheduleData schedule) {
  schedules.remove(index);
  schedules.addAtIndex(index, schedule);
}

bool SCHEDULE_isTimeBased(scheduleData schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "timeBased";
}

bool SCHEDULE_isInterval(scheduleData schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "interval";
}

bool SCHEDULE_isDelay(scheduleData schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "delay";
}

bool SCHEDULE_isLightSensor(scheduleData schedule) {
  return schedule.trigger == "light";
}

bool SCHEDULE_isMovementSensor(scheduleData schedule) {
  return schedule.trigger == "movement";
}

bool SCHEDULE_isMoistureSensor(scheduleData schedule) {
  return schedule.trigger == "moisture";
}

bool SCHEDULE_inTime(scheduleData schedule) {
  return schedule.startHour.toInt() == TIME_now().hour() &&
         schedule.startMinute.toInt() == TIME_now().minute();
}

bool SCHEDULE_outTime(scheduleData schedule) {
  return schedule.stopHour.toInt() == TIME_now().hour() &&
         schedule.stopMinute.toInt() == TIME_now().minute();
}

bool SCHEDULE_isActive(scheduleData schedule) { return schedule.active; }

bool SCHEDULE_isTargetLamp(scheduleData schedule) {
  return schedule.target == "lamp";
}
bool SCHEDULE_isTargetPlug(scheduleData schedule) {
  return schedule.target == "plug";
}

bool SCHEDULE_isSubActive(scheduleData schedule) { return schedule.subActive; }

bool SCHEDULE_isMoreSubActive(scheduleData schedule) {
  return schedule.moreSubActive;
}

bool SCHEDULE_isInLengthOn(scheduleData schedule) {
  return millis() - schedule.prevMillisOn <=
         (schedule.lengthOn.toInt() * 60000l);
}

bool SCHEDULE_isInLengthOff(scheduleData schedule) {
  return millis() - schedule.prevMillisOff <=
         (schedule.lengthOff.toInt() * 60000l);
}

bool SCHEDULE_isOutDelayTime(scheduleData schedule) {
  return millis() - schedule.prevMillisOn >= (schedule.delay.toInt() * 60000l);
}

bool SCHEDULE_isLightTriggered(scheduleData schedule, String data) {
  return (schedule.activeCondition == "p" && data == "Pagi/Sore" &&
          (TIME_now().hour() >= 5 && TIME_now().hour() <= 12)) ||
         (schedule.activeCondition == "si" && data == "Siang") ||
         (schedule.activeCondition == "so" && data == "Pagi/Sore" &&
          (TIME_now().hour() >= 15 && TIME_now().hour() <= 18)) ||
         (schedule.activeCondition == "m" && data == "Malam");
}

bool SCHEDULE_isMovementTriggered(scheduleData schedule, String data) {
  return (schedule.activeCondition == "ag" && data == "Ada Gerakan") ||
         (schedule.activeCondition == "tag" && data == "Tidak ada gerakan");
}

bool SCHEDULE_isMoistureTriggered(scheduleData schedule, String data) {
  return (schedule.activeCondition == "k" && data == "Kering") ||
         (schedule.activeCondition == "b" && data == "Basah") ||
         (schedule.activeCondition == "tba" && data == "Terlalu banyak air");
}

void SCHEDULE_turnDevice(scheduleData schedule, bool condition) {
  if (SCHEDULE_isTargetLamp(schedule)) {
    WS_turn(schedule.targetId, condition);
    if (!WIFI_isOfflineMode()) {
      FIREBASE_turnLamp(schedule.targetId, condition);
    } else {
      FIREBASE_printOfflineMessage();
    }
  }
  if (SCHEDULE_isTargetPlug(schedule)) {
    WS_turn(schedule.targetId + "/" + schedule.socketId, condition);
    if (!WIFI_isOfflineMode()) {
      FIREBASE_turnPlug(schedule.targetId, schedule.socketId, condition);
    } else {
      FIREBASE_printOfflineMessage();
    }
  }
}

void SCHEDULE_checkIfDelayable(String deviceId) {
  bool delayAble = true;
  for (int i = 0; i < schedules.getSize(); i++) {
    if (schedules[i].trigger == "time" &&
        (schedules[i].timeMode == "timeBased" ||
         schedules[i].timeMode == "interval") &&
        schedules[i].targetId == deviceId) {
      delayAble = false;
      break;
    }
  }
  if (delayAble) {
    for (int i = 0; i < schedules.getSize(); i++) {
      if (SCHEDULE_isDelay(schedules[i]) && schedules[i].targetId == deviceId) {
        if (!SCHEDULE_isActive(schedules[i])) {
          scheduleData currSchedule = schedules[i];
          currSchedule.active = true;
          currSchedule.prevMillisOn = millis();
          SCHEDULE_update(i, currSchedule);
          Serial.println("GLOBAL: Delay Schedule Start!");
        }
      }
    }
  } else {
    Serial.println("This device contain schedule other than delay");
    Serial.println("Delay won't be applied...");
  }
}

void SCHEDULE_cancelDelay(String deviceId) {
  for (int i = 0; i < schedules.getSize(); i++) {
    if (SCHEDULE_isDelay(schedules[i]) && schedules[i].targetId == deviceId) {
      if (SCHEDULE_isActive(schedules[i])) {
        scheduleData currSchedule = schedules[i];
        currSchedule.active = false;
        SCHEDULE_update(i, currSchedule);
      }
    }
  }
}

void SCHEDULE_removeAll() {
  schedules.clear();
  schedules.removeAll();
}
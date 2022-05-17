#include "schedule_cmp.h"

List<schedule> schedules;

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
      schedule s;
      deserializeJson(sD, v.as<String>());
      s.id = String(i);
      s.target = sD["target"].as<String>();
      s.targetId = sD["targetId"].as<String>();
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

void SCHEDULE_update(int index, schedule schedule) {
  schedules.remove(index);
  schedules.addAtIndex(index, schedule);
}

bool SCHEDULE_isTimeBased(schedule schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "timeBased";
}

bool SCHEDULE_isInterval(schedule schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "interval";
}

bool SCHEDULE_isDelay(schedule schedule) {
  return schedule.trigger == "time" && schedule.timeMode == "delay";
}

bool SCHEDULE_inTime(schedule schedule) {
  return schedule.startHour.toInt() == TIME_now().hour() &&
         schedule.startMinute.toInt() == TIME_now().minute();
}

bool SCHEDULE_outTime(schedule schedule) {
  return schedule.stopHour.toInt() == TIME_now().hour() &&
         schedule.stopMinute.toInt() == TIME_now().minute();
}

bool SCHEDULE_isActive(schedule schedule) { return schedule.active; }

bool SCHEDULE_isTargetLamp(schedule schedule) {
  return schedule.target == "lamp";
}
bool SCHEDULE_isTargetPlug(schedule schedule) {
  return schedule.target == "plug";
}

bool SCHEDULE_isSubActive(schedule schedule) { return schedule.subActive; }

bool SCHEDULE_isMoreSubActive(schedule schedule) {
  return schedule.moreSubActive;
}

bool SCHEDULE_isInLengthOn(schedule schedule) {
  return millis() - schedule.prevMillisOn <=
         (schedule.lengthOn.toInt() * 60000l);
}

bool SCHEDULE_isInLengthOff(schedule schedule) {
  return millis() - schedule.prevMillisOff <=
         (schedule.lengthOff.toInt() * 60000l);
}

bool SCHEDULE_isOutDelayTime(schedule schedule) {
  return millis() - schedule.prevMillisOn >= (schedule.delay.toInt() * 60000l);
}

void SCHEDULE_turnDevice(schedule schedule, bool condition) {
  if (SCHEDULE_isTargetLamp(schedule)) {
    WS_turn(schedule.targetId, condition);
    if (!WIFI_isOfflineMode()) {
      FIREBASE_turnLamp(schedule.targetId, condition);
    } else {
      FIREBASE_printOfflineMessage();
    }
  }
  if (SCHEDULE_isTargetPlug(schedule)) {
    WS_turn(schedule.targetId + "/all", condition);
    if (!WIFI_isOfflineMode()) {
      FIREBASE_turnPlug(schedule.targetId, condition);
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
          schedule currSchedule = schedules[i];
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
        schedule currSchedule = schedules[i];
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
#include "schedule_cmp.h"

List<schedule> schedules;

void SCHEDULE_build(String source) {
  schedules.clear();
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
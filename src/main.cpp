//$ Include Library
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <RTClib.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>

#include <List.hpp>

#include "display_cmp.h"
#include "eeprom_cmp.h"
#include "firebase_cmp.h"
#include "global_cmp.h"
#include "schedule_cmp.h"
#include "spiffs_cmp.h"
#include "system_cmp.h"
#include "time_cmp.h"
#include "websocket_cmp.h"
#include "wifi_cmp.h"

void setup() {
  SYSTEM_init();
  TIME_init();
  EEPROM_init();
  DISPLAY_init();
  SPIFFS_init();

  EEPROM_isFirstRun();
  SPIFFS_getWiFiCred();

  WIFI_initStation(wifi_ssid, wifi_pass);

  if (WIFI_isOfflineMode()) {
    WIFI_printOfflineMessage();
  } else {
    WIFI_printStationIP();
  }

  WIFI_initAP();

  WIFI_printAPIP();

  if (WIFI_isOfflineMode()) {
    FIREBASE_init();
    FIREBASE_initStream();
    FIREBASE_initDeviceData();

    FIREBASE_getSchedule();
  }

  DISPLAY_printQRCode();
}

void loop() {
  WS_loop();

  if (FIREBASE_isTokenExpired()) {
    FIREBASE_printTokenExpiredMessage();
    SYSTEM_restart();
  }

  DateTime now = TIME_now();
  if (TIME_tick(5000)) {
    TIME_update();
    TIME_printClock();

    for (int i = 0; i < schedules.getSize(); i++) {
      if (schedules[i].trigger == "time" &&
          schedules[i].timeMode == "timeBased") {
        schedule timeBased = schedules[i];
        if (schedules[i].startHour.toInt() == now.hour() &&
            schedules[i].startMinute.toInt() == now.minute()) {
          if (schedules[i].active == false) {
            Serial.println("Waktu masuk jadwal ID" + schedules[i].id);
            Serial.println("Target: " + schedules[i].targetId);
            timeBased.active = true;
            schedules.remove(i);
            schedules.addAtIndex(i, timeBased);
            if (schedules[i].target == "lamp") {
              target = schedules[i].targetId;
              conditionToSendWebsocket = true;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                Firebase.RTDB.setBool(
                    &fbdo, lampLoc + "/" + schedules[i].targetId + "/condition",
                    true);
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
            if (schedules[i].target == "plug") {
              target = schedules[i].targetId + "/all";
              conditionToSendWebsocket = true;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                for (int i = 0; i < SOCKET_COUNT; i++) {
                  Firebase.RTDB.setBool(&fbdo,
                                        plugLoc + "/" + schedules[i].targetId +
                                            "/sockets/" + "socket-" +
                                            (String)i + "/feedback",
                                        true);
                }
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
          }
        }
        if (schedules[i].stopHour.toInt() == now.hour() &&
            schedules[i].stopMinute.toInt() == now.minute()) {
          if (schedules[i].active == true) {
            Serial.println("Waktu keluar jadwal ID" + schedules[i].id);
            Serial.println("Target: " + schedules[i].targetId);
            timeBased.active = false;
            schedules.remove(i);
            schedules.addAtIndex(i, timeBased);
            if (schedules[i].target == "lamp") {
              target = schedules[i].targetId;
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                Firebase.RTDB.setBool(
                    &fbdo, lampLoc + "/" + schedules[i].targetId + "/condition",
                    false);
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
            if (schedules[i].target == "plug") {
              target = schedules[i].targetId + "/all";
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                for (int i = 0; i < SOCKET_COUNT; i++) {
                  Firebase.RTDB.setBool(&fbdo,
                                        plugLoc + "/" + schedules[i].targetId +
                                            "/sockets/" + "socket-" +
                                            (String)i + "/feedback",
                                        false);
                }
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
          }
        }
      }
      if (schedules[i].trigger == "time" &&
          schedules[i].timeMode == "interval") {
        schedule interval = schedules[i];
        if (schedules[i].startHour.toInt() == now.hour() &&
            schedules[i].startMinute.toInt() == now.minute()) {
          if (schedules[i].active == false) {
            Serial.println("Activing Device");
            interval.prevMillisOn = millis();
            interval.active = true;
            schedules.remove(i);
            schedules.addAtIndex(i, interval);
          }
        }
        if ((schedules[i].stopHour.toInt() == now.hour() &&
             schedules[i].stopMinute.toInt() == now.minute())) {
          if (schedules[i].active == true) {
            Serial.println("Deactivating Device");
            interval.active = false;
            schedules.remove(i);
            schedules.addAtIndex(i, interval);
            if (schedules[i].target == "lamp") {
              target = schedules[i].targetId;
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                Firebase.RTDB.setBool(
                    &fbdo, lampLoc + "/" + schedules[i].targetId + "/condition",
                    false);
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
            if (schedules[i].target == "plug") {
              target = schedules[i].targetId + "/all";
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                for (int i = 0; i < SOCKET_COUNT; i++) {
                  Firebase.RTDB.setBool(&fbdo,
                                        plugLoc + "/" + schedules[i].targetId +
                                            "/sockets/" + "socket-" +
                                            (String)i + "/feedback",
                                        false);
                }
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
          }
        }
        if (schedules[i].active == true) {
          Serial.println("Interval Schedule Active");
          if (schedules[i].subActive == false) {
            Serial.println("Interval device Off");
            if (millis() - schedules[i].prevMillisOn <=
                (schedules[i].lengthOn.toInt() * 60000l)) {
              Serial.println("In ON Millis");
              if (schedules[i].moreSubActive == false) {
                Serial.println("Interval: Active...");
                interval.moreSubActive = true;
                schedules.remove(i);
                schedules.addAtIndex(i, interval);
                if (schedules[i].target == "lamp") {
                  target = schedules[i].targetId;
                  conditionToSendWebsocket = true;
                  WS_sendMessage();
                  if (WIFI_isOfflineMode()) {
                    Firebase.RTDB.setBool(
                        &fbdo,
                        lampLoc + "/" + schedules[i].targetId + "/condition",
                        true);
                  } else {
                    Serial.println("GLOBAL: Can't send data to Firebase.");
                    Serial.println("GLOBAL: Reason => Offline Mode.");
                  }
                }
                if (schedules[i].target == "plug") {
                  target = schedules[i].targetId + "/all";
                  conditionToSendWebsocket = true;
                  WS_sendMessage();
                  if (WIFI_isOfflineMode()) {
                    for (int i = 0; i < SOCKET_COUNT; i++) {
                      Firebase.RTDB.setBool(
                          &fbdo,
                          plugLoc + "/" + schedules[i].targetId + "/sockets/" +
                              "socket-" + (String)i + "/feedback",
                          true);
                    }
                  } else {
                    Serial.println("GLOBAL: Can't send data to Firebase.");
                    Serial.println("GLOBAL: Reason => Offline Mode.");
                  }
                }
              }
            } else {
              Serial.println("Not in ON Millis");
              interval.prevMillisOn = millis();
              interval.prevMillisOff = millis();
              interval.moreSubActive = false;
              interval.subActive = true;
              schedules.remove(i);
              schedules.addAtIndex(i, interval);
            }
          }
          if (schedules[i].subActive == true) {
            Serial.println("Interval device On");
            if (millis() - schedules[i].prevMillisOff <=
                (schedules[i].lengthOff.toInt() * 60000l)) {
              Serial.println("In OFF Millis");
              if (schedules[i].moreSubActive == false) {
                Serial.println("Interval: Inactive...");
                interval.moreSubActive = true;
                schedules.remove(i);
                schedules.addAtIndex(i, interval);
                if (schedules[i].target == "lamp") {
                  target = schedules[i].targetId;
                  conditionToSendWebsocket = false;
                  WS_sendMessage();
                  if (WIFI_isOfflineMode()) {
                    Firebase.RTDB.setBool(
                        &fbdo,
                        lampLoc + "/" + schedules[i].targetId + "/condition",
                        false);
                  } else {
                    Serial.println("GLOBAL: Can't send data to Firebase.");
                    Serial.println("GLOBAL: Reason => Offline Mode.");
                  }
                }
                if (schedules[i].target == "plug") {
                  target = schedules[i].targetId + "/all";
                  conditionToSendWebsocket = false;
                  WS_sendMessage();
                  if (WIFI_isOfflineMode()) {
                    for (int i = 0; i < SOCKET_COUNT; i++) {
                      Firebase.RTDB.setBool(
                          &fbdo,
                          plugLoc + "/" + schedules[i].targetId + "/sockets/" +
                              "socket-" + (String)i + "/feedback",
                          false);
                    }
                  } else {
                    Serial.println("GLOBAL: Can't send data to Firebase.");
                    Serial.println("GLOBAL: Reason => Offline Mode.");
                  }
                }
              }
            } else {
              Serial.println("Not in OFF Millis");
              interval.prevMillisOn = millis();
              interval.prevMillisOff = millis();
              interval.moreSubActive = false;
              interval.subActive = false;
              schedules.remove(i);
              schedules.addAtIndex(i, interval);
            }
          }
        }
      }
      if (schedules[i].trigger == "time" && schedules[i].timeMode == "delay") {
        schedule delaySchedule = schedules[i];
        if (millis() - schedules[i].prevMillisOn >=
            (schedules[i].delay.toInt() * 60000l)) {
          if (schedules[i].active == true) {
            Serial.println("Turning off");
            delaySchedule.active = false;
            schedules.remove(i);
            schedules.addAtIndex(i, delaySchedule);
            if (schedules[i].target == "lamp") {
              target = schedules[i].targetId;
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                Firebase.RTDB.setBool(
                    &fbdo, lampLoc + "/" + schedules[i].targetId + "/condition",
                    false);
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
            if (schedules[i].target == "plug") {
              target = schedules[i].targetId + "/all";
              conditionToSendWebsocket = false;
              WS_sendMessage();
              if (WIFI_isOfflineMode()) {
                for (int i = 0; i < SOCKET_COUNT; i++) {
                  Firebase.RTDB.setBool(&fbdo,
                                        plugLoc + "/" + schedules[i].targetId +
                                            "/sockets/" + "socket-" +
                                            (String)i + "/feedback",
                                        false);
                }
              } else {
                Serial.println("GLOBAL: Can't send data to Firebase.");
                Serial.println("GLOBAL: Reason => Offline Mode.");
              }
            }
          }
        }
      }
    }
  }

  if (firebaseDataChanged) {
    firebaseDataChanged = false;
    String head = getValue(receivedDataFirebase.dataPath, '/', 3);
    String neck = getValue(receivedDataFirebase.dataPath, '/', 4);
    String currentData = receivedDataFirebase.data;

    if (head == "lamps") {
      for (int i = 1; i <= LAMP_COUNT; i++) {
        if (neck == "lamp-" + (String)i) {
          if (currentData == "true") {
            target = "lamp-" + (String)i;
            conditionToSendWebsocket = true;
            bool delayAble = true;
            WS_sendMessage();
            for (int i = 0; i < schedules.getSize(); i++) {
              if (schedules[i].trigger == "time" &&
                  (schedules[i].timeMode == "timeBased" ||
                   schedules[i].timeMode == "interval") &&
                  schedules[i].targetId == target) {
                delayAble = false;
                break;
              }
            }
            if (delayAble) {
              for (int i = 0; i < schedules.getSize(); i++) {
                if (schedules[i].trigger == "time" &&
                    schedules[i].timeMode == "delay" &&
                    schedules[i].targetId == target) {
                  schedule currSchedule = schedules[i];
                  currSchedule.active = true;
                  currSchedule.prevMillisOn = millis();
                  schedules.remove(i);
                  schedules.addAtIndex(i, currSchedule);
                  Serial.println("GLOBAL: Delay Schedule Start!");
                }
              }
            } else {
              Serial.println("This device contain schedule other than delay");
              Serial.println("Delay won't be applied...");
            }
            Serial.println("GLOBAL: Lampu " + (String)i + " menyala!");
          } else {
            target = "lamp-" + (String)i;
            conditionToSendWebsocket = false;
            WS_sendMessage();
            for (int i = 0; i < schedules.getSize(); i++) {
              if (schedules[i].trigger == "time" &&
                  schedules[i].timeMode == "delay" &&
                  schedules[i].targetId == target) {
                schedule currSchedule = schedules[i];
                currSchedule.active = false;
                schedules.remove(i);
                schedules.addAtIndex(i, currSchedule);
              }
            }
            Serial.println("GLOBAL: Lampu " + (String)i + " mati!");
          }
        }
      }
    } else if (head == "plugs") {
      String deviceSubName = getValue(receivedDataFirebase.dataPath, '/', 6);

      for (int i = 1; i <= PLUG_COUNT; i++) {
        if (neck == "plug-" + (String)i) {
          for (int j = 1; j <= SOCKET_COUNT; j++) {
            if (deviceSubName == "socket-" + (String)j) {
              if (currentData == "true") {
                target = "plug-" + (String)i + "/socket-" + (String)j;
                conditionToSendWebsocket = true;
                WS_sendMessage();
                bool delayAble = true;
                for (int i = 0; i < schedules.getSize(); i++) {
                  if (schedules[i].trigger == "time" &&
                      (schedules[i].timeMode == "timeBased" ||
                       schedules[i].timeMode == "interval") &&
                      schedules[i].targetId == target) {
                    delayAble = false;
                    break;
                  }
                }
                if (delayAble) {
                  for (int i = 0; i < schedules.getSize(); i++) {
                    if (schedules[i].trigger == "time" &&
                        schedules[i].timeMode == "delay" &&
                        schedules[i].targetId == target) {
                      schedule currSchedule = schedules[i];
                      currSchedule.active = true;
                      currSchedule.prevMillisOn = millis();
                      schedules.remove(i);
                      schedules.addAtIndex(i, currSchedule);
                      Serial.println("GLOBAL: Delay Schedule Start!");
                    }
                  }
                } else {
                  Serial.println(
                      "This device contain schedule other than delay");
                  Serial.println("Delay won't be applied...");
                }
                Serial.println("GLOBAL: Stopkontak " + (String)i +
                               " => Socket " + (String)j + " menyala!");
              } else {
                target = "plug-" + (String)i + "/socket-" + (String)j;
                conditionToSendWebsocket = false;
                WS_sendMessage();
                for (int i = 0; i < schedules.getSize(); i++) {
                  if (schedules[i].trigger == "time" &&
                      schedules[i].timeMode == "delay" &&
                      schedules[i].targetId == neck) {
                    schedule currSchedule = schedules[i];
                    currSchedule.active = false;
                    schedules.remove(i);
                    schedules.addAtIndex(i, currSchedule);
                  }
                }
                Serial.println("GLOBAL: Stopkontak " + (String)i +
                               " => Socket " + (String)j + " mati!");
              }
            }
          }
        }
      }
    } else if (head == "schedules") {
      Serial.println(receivedDataFirebase.dataPath);
      Serial.println();
      String schedId = getValue(receivedDataFirebase.dataPath, '/', 4);
      bool newSched = true;

      Serial.println("Loop Start");
      for (int i = 0; i < schedules.getSize(); i++) {
        Serial.println(schedules[i].id);
        if (schedules[i].id == schedId) {
          newSched = false;
          break;
        }
      }
      Serial.println("Loop End");
      Serial.println();

      if (!newSched) {
        Serial.println("GLOBAL: Old Schedule. '3')");
      } else {
        Serial.println("GLOBAL: New Schedule! >_<");
      }
      FIREBASE_getSchedule();
    }
  }
}
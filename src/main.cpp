//$ Include Library
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <RTClib.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <Wire.h>

#include <List.hpp>

#include "display_cmp.h"
#include "eeprom_cmp.h"
#include "firebase_cmp.h"
#include "schedule_cmp.h"
#include "global_cmp.h"
#include "spiffs_cmp.h"
#include "system_cmp.h"
#include "time_cmp.h"
#include "wifi_cmp.h"
#include "websocket_cmp.h"

void setup() {
  SYSTEM_init();
  TIME_init();
  EEPROM_init();
  DISPLAY_init();
  SPIFFS_init();

  EEPROM_isFirstRun();
  SPIFFS_getWiFiCred();

  WIFI_initStation(wifi_ssid, wifi_pass);

  if (WiFi.status() == WL_CONNECTED) {
    WIFI_setOfflineMode(false);
  } else {
    WIFI_setOfflineMode(true);
  }

  if (WIFI_isOfflineMode()) {
    WIFI_printOfflineMessage();
    WIFI_setOfflineMode(true);
    SYSTEM_turnLED(ORANGE_LED, true);
    SYSTEM_turnLED(BLUE_LED, false);
  } else {
    WIFI_printStationIP();
    WIFI_setOfflineMode(false);
    SYSTEM_turnLED(ORANGE_LED, false);
    SYSTEM_turnLED(BLUE_LED, true);
  }

  WIFI_initAP();

  WIFI_printAPIP();

  if (!WIFI_isOfflineMode()) {
    FIREBASE_init();
    FIREBASE_initStream();
    FIREBASE_initDeviceData();

    FIREBASE_getSchedule();
  }

  if (WIFI_isOfflineMode()) {
    Serial.println("Schedule data from SPIFFS");
    SCHEDULE_build(SPIFFS_getScheduleData());
  }

  DISPLAY_printQRCode();
  WS_init();
}

void loop() {
  WS_loop();

  if (TIME_tick(5000)) {
    TIME_update();
    TIME_printClock();

    for (int i = 0; i < schedules.getSize(); i++) {
      if (SCHEDULE_isTimeBased(schedules[i])) {
        scheduleData timeBased = schedules[i];

        if (SCHEDULE_inTime(timeBased)) {
          if (!SCHEDULE_isActive(timeBased)) {
            timeBased.active = true;
            SCHEDULE_update(i, timeBased);
            SCHEDULE_turnDevice(timeBased, true);
          }
        }
        if (SCHEDULE_outTime(timeBased)) {
          if (SCHEDULE_isActive(timeBased)) {
            timeBased.active = false;
            SCHEDULE_update(i, timeBased);
            SCHEDULE_turnDevice(timeBased, false);
          }
        }
      }
      if (SCHEDULE_isInterval(schedules[i])) {
        scheduleData interval = schedules[i];

        if (SCHEDULE_inTime(interval)) {
          if (!SCHEDULE_isActive(interval)) {
            interval.prevMillisOn = millis();
            interval.active = true;
            SCHEDULE_update(i, interval);
          }
        }
        if (SCHEDULE_outTime(interval)) {
          if (SCHEDULE_isActive(interval)) {
            interval.active = false;
            SCHEDULE_update(i, interval);
            SCHEDULE_turnDevice(interval, false);
          }
        }
        if (SCHEDULE_isActive(interval)) {
          if (!SCHEDULE_isSubActive(interval)) {
            if (SCHEDULE_isInLengthOn(interval)) {
              if (!SCHEDULE_isMoreSubActive(interval)) {
                interval.moreSubActive = true;
                SCHEDULE_update(i, interval);
                SCHEDULE_turnDevice(interval, true);
              }
            } else {
              interval.prevMillisOn = millis();
              interval.prevMillisOff = millis();
              interval.moreSubActive = false;
              interval.subActive = true;
              SCHEDULE_update(i, interval);
            }
          }
          if (SCHEDULE_isSubActive(interval)) {
            if (SCHEDULE_isInLengthOff(interval)) {
              if (!SCHEDULE_isMoreSubActive(interval)) {
                interval.moreSubActive = true;
                SCHEDULE_update(i, interval);
                SCHEDULE_turnDevice(interval, false);
              }
            } else {
              interval.prevMillisOn = millis();
              interval.prevMillisOff = millis();
              interval.moreSubActive = false;
              interval.subActive = false;
              SCHEDULE_update(i, interval);
            }
          }
        }
      }
      if (SCHEDULE_isDelay(schedules[i])) {
        scheduleData delaySchedule = schedules[i];

        if (SCHEDULE_isOutDelayTime(delaySchedule)) {
          if (SCHEDULE_isActive(delaySchedule)) {
            delaySchedule.active = false;
            delaySchedule.subActive = true;
            SCHEDULE_update(i, delaySchedule);
          }
        }

        if (!SCHEDULE_isActive(delaySchedule)) {
          if (SCHEDULE_isSubActive(delaySchedule)) {
            delaySchedule.subActive = false;
            SCHEDULE_update(i, delaySchedule);
            SCHEDULE_turnDevice(delaySchedule, false);
          }
        }
      }

      if (SCHEDULE_isLightSensor(schedules[i])) {
        scheduleData lightSchedule = schedules[i];
        if (SCHEDULE_isLightTriggered(lightSchedule, currentLightReading)) {
          if (!SCHEDULE_isActive(lightSchedule)) {
            lightSchedule.active = true;
            SCHEDULE_update(i, lightSchedule);
            SCHEDULE_turnDevice(lightSchedule, true);
          }
        } else {
          if (SCHEDULE_isActive(lightSchedule)) {
            lightSchedule.active = false;
            SCHEDULE_update(i, lightSchedule);
            SCHEDULE_turnDevice(lightSchedule, false);
          }
        }
      }

      if (SCHEDULE_isMovementSensor(schedules[i])) {
        scheduleData movementSchedule = schedules[i];
        if (SCHEDULE_isMovementTriggered(movementSchedule,
                                         currentMovementReading)) {
          if (!SCHEDULE_isActive(movementSchedule)) {
            movementSchedule.active = true;
            SCHEDULE_update(i, movementSchedule);
            prevMovementMillis = millis();
            SCHEDULE_turnDevice(movementSchedule, true);
            Serial.println("Movement Detected!");
            Serial.println("Activating Movement Schedule...");
          }
        }

        if (movementSchedule.active == true) {
          if (millis() - prevMovementMillis > (interval - 15000ul) &&
              millis() - prevMovementMillis <= interval) {
            if (currentMovementReading == "Ada Gerakan") {
              if (!hasExtended) {
                Serial.println("Movement still detected!");
                Serial.println("Extending time...");
                prevInterval = interval;
                interval += 60000ul;
                hasExtended = true;
              }
            }
          }

          if (millis() - prevMovementMillis > prevInterval &&
              millis() - prevMovementMillis <= interval && hasExtended) {
            Serial.println("Time has extended!");
            hasExtended = false;
          }

          if (millis() - prevMovementMillis > interval) {
            Serial.println("No movement detected so far");
            Serial.println("Deactivating Movement Schedule...");
            movementSchedule.active = false;
            SCHEDULE_update(i, movementSchedule);
            SCHEDULE_turnDevice(movementSchedule, false);
          }
        }
      }

      if (SCHEDULE_isMoistureSensor(schedules[i])) {
        scheduleData moistureSchedule = schedules[i];
        if (SCHEDULE_isMoistureTriggered(moistureSchedule,
                                         currentMoistureReading)) {
          if (!SCHEDULE_isActive(moistureSchedule)) {
            moistureSchedule.active = true;
            SCHEDULE_update(i, moistureSchedule);
            SCHEDULE_turnDevice(moistureSchedule, true);
          }
        } else {
          if (SCHEDULE_isActive(moistureSchedule)) {
            moistureSchedule.active = false;
            SCHEDULE_update(i, moistureSchedule);
            SCHEDULE_turnDevice(moistureSchedule, false);
          }
        }
      }
      delay(1);
    }
  }

  if (!WIFI_isOfflineMode()) {
    if (FIREBASE_isTokenExpired()) {
      FIREBASE_printTokenExpiredMessage();
      SYSTEM_restart();
    }

    if (firebaseDataChanged) {
      SYSTEM_blinkLED(GREEN_LED);
      firebaseDataChanged = false;
      String head = getValue(receivedDataFirebase.dataPath, '/', 3);
      String neck = getValue(receivedDataFirebase.dataPath, '/', 4);
      String currentData = receivedDataFirebase.data;

      if (head == "lamps") {
        for (int i = 1; i <= LAMP_COUNT; i++) {
          if (neck == "lamp-" + (String)i) {
            if (currentData == "true") {
              WS_turn(neck, true);
              SCHEDULE_checkIfDelayable(neck);
              Serial.println("GLOBAL: Lampu " + (String)i + " menyala!");
            } else {
              WS_turn(neck, false);
              SCHEDULE_cancelDelay(neck);
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
                  WS_turn(neck + "/" + deviceSubName, true);
                  SCHEDULE_checkIfDelayable(neck);
                  Serial.println("GLOBAL: Stopkontak " + (String)i +
                                 " => Socket " + (String)j + " menyala!");
                } else {
                  WS_turn(neck + "/" + deviceSubName, false);
                  SCHEDULE_cancelDelay(neck);
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
          delay(1);
        }

        Serial.println("Loop End");
        Serial.println();

        if (!newSched) {
          Serial.println("GLOBAL: Old Schedule. '3')");
        } else {
          Serial.println("GLOBAL: New Schedule! >_<");
        }

        FIREBASE_getSchedule();
      } else if (head == "offlineMode") {
        if (currentData == "true") {
          Serial.println("Switching to Offline Mode");
          Serial.println("Getting schedule data from SPIFFS");
          SYSTEM_turnLED(BLUE_LED, false);
          SYSTEM_turnLED(ORANGE_LED, true);
          SCHEDULE_build(SPIFFS_getScheduleData());
        }
      }
    }
  }
  delay(5);
}
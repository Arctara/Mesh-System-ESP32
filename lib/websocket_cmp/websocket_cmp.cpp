#include <ArduinoJson.h>

#include "websocket_cmp.h"

//* WebSocket Configuration
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String target = "";
bool conditionToSendWebsocket = "";

void WS_init() {
  ws.onEvent(WS_onEvent);
  server.addHandler(&ws);
  server.begin();
}

void WS_loop() { ws.cleanupClients(); }

void WS_sendMessage() {
  DynamicJsonDocument dataToSendWebsocket(256);

  dataToSendWebsocket["from"] = "center";
  dataToSendWebsocket["to"] = target;
  dataToSendWebsocket["condition"] = String(conditionToSendWebsocket);

  String sendData;
  serializeJson(dataToSendWebsocket, sendData);
  Serial.println(sendData);

  ws.textAll(sendData);
}

void WS_onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      SYSTEM_blinkLED(GREEN_LED);
      Serial.printf("Websocket: Websocket client #%u connected from %s\n",
                    client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      SYSTEM_blinkLED(GREEN_LED);
      Serial.printf("Websocket: Websocket client #%u disconnected\n",
                    client->id());
      break;
    case WS_EVT_DATA:
      SYSTEM_blinkLED(GREEN_LED);
      WS_handleMessage(arg, data, len);
      break;
    default:
      break;
  }
}

void WS_turn(String device, bool condition) {
  target = device;
  conditionToSendWebsocket = condition;
  WS_sendMessage();
}

void WS_handleMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT) {
    data[len] = 0;

    DynamicJsonDocument receivedDataWebsocket(256);

    deserializeJson(receivedDataWebsocket, data);

    String from = receivedDataWebsocket["from"].as<String>();
    String to = receivedDataWebsocket["to"].as<String>();

    if (to == "center") {
      Serial.println("! Data for Center!");
      for (int i = 1; i <= PLUG_COUNT; i++) {
        if (from == "plug-" + (String)i) {
          Serial.println("  > Data from Plug " + (String)i + "! (Plg" +
                         (String)i + ")");

          String socket = receivedDataWebsocket["socket"].as<String>();
          bool condition = receivedDataWebsocket["condition"].as<bool>();

          Serial.println("    ==> " + socket + " = " + condition);

          if (!WIFI_isOfflineMode()) {
            Firebase.RTDB.setBool(
                &fbdo,
                plugLoc + "/" + from + "/sockets/" + socket + "/feedback",
                condition);
          } else {
            Serial.println("GLOBAL: Can't send data to Firebase.");
            Serial.println("GLOBAL: Reason => Offline Mode.");
          }
        }
      }

      for (int i = 1; i <= LAMP_COUNT; i++) {
        if (from == "lamp-" + (String)i) {
          Serial.println("  > Data from Lamp " + (String)i + "! (lmp)");

          String feedback = receivedDataWebsocket["feedback"].as<String>();

          Serial.println("    ==> " + feedback);
        }
      }

      for (int i = 1; i <= SENSOR_COUNT; i++) {
        if (from == "sensor-" + (String)i) {
          String sensorType = receivedDataWebsocket["sensorType"].as<String>();
          String data = receivedDataWebsocket["data"].as<String>();
          String icon = "";

          if (sensorType == "lightSensor") {
            if (data == "Siang") {
              icon = "sun";
            } else if (data == "Malam") {
              icon = "moon";
            } else {
              icon = "sun";
            }

            currentLightReading = data;

            Serial.println("  > Data from Light Sensor! (LiSn)");
          } else if (sensorType == "movementSensor") {
            icon = "human";

            currentMovementReading

                Serial.println("  > Data from Movement Sensor! (MvmSn)");
          } else if (sensorType == "moistureSensor") {
            if (data == "Terlalu banyak air") {
              icon = "wetter";
            } else if (data == "Basah") {
              icon = "wet";
            } else {
              icon = "dry";
            }

            currentMoistureReading = data;

            Serial.println("  > Data from Moisture Sensor! (MstSn)");
          }

          Serial.println("    ==> " + data);

          if (!WIFI_isOfflineMode()) {
            Firebase.RTDB.setString(
                &fbdo, sensorLoc + "/sensor-" + (String)i + "/sensorType",
                sensorType);
            Firebase.RTDB.setString(
                &fbdo, sensorLoc + "/sensor-" + (String)i + "/reading", data);
            Firebase.RTDB.setString(
                &fbdo, sensorLoc + "/sensor-" + (String)i + "/icon", icon);
          } else {
            Serial.println("GLOBAL: Can't send data to Firebase.");
            Serial.println("GLOBAL: Reason => Offline Mode.");
          }
        }
      }

      if (from == "mobile") {
        Serial.println("  > Data from Mobile Application! (MobApp)");
        String event = receivedDataWebsocket["event"].as<String>();
        if (event == "wifi-set") {
          String ssid = receivedDataWebsocket["ssid"].as<String>();
          String pass = receivedDataWebsocket["pass"].as<String>();

          SPIFFS_setWiFiCred(ssid, pass);
        }
        if (event == "echo") {
          String message = receivedDataWebsocket["message"].as<String>();

          Serial.println("    Echo Request!");
          Serial.println("    Message => " + message);
          ws.textAll(message);
        }
        if (event == "requestLampData") {
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"lamp\", \"data\": ";
          String lampData = SPIFFS_getLampData();
          String closing = "}";
          String dataToSend = identity + lampData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "requestPlugData") {
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"plug\", \"data\": ";
          String plugData = SPIFFS_getPlugData();
          String closing = "}";
          String dataToSend = identity + plugData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "requestScheduleData") {
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"schedule\", \"lampData\": ";
          String lampData = SPIFFS_getLampData();
          String middleOne = ", \"plugData\": ";
          String plugData = SPIFFS_getPlugData();
          String middleTwo = ", \"sensorData\": ";
          String sensorData = SPIFFS_getSensorData();
          String middleThree = ", \"scheduleData\": ";
          String scheduleData = SPIFFS_getScheduleData();
          String closing = "}";
          String dataToSend = identity + lampData + middleOne + plugData +
                              middleTwo + sensorData + middleThree +
                              scheduleData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "requestMakeScheduleData") {
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"makeSchedule\", \"lampData\": ";
          String lampData = SPIFFS_getLampData();
          String middleOne = ", \"plugData\": ";
          String plugData = SPIFFS_getPlugData();
          String middleTwo = ", \"sensorData\": ";
          String sensorData = SPIFFS_getSensorData();
          String closing = "}";
          String dataToSend = identity + lampData + middleOne + plugData +
                              middleTwo + sensorData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "requestSensorData") {
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"sensor\", \"data\": ";
          String plugData = SPIFFS_getSensorData();
          String closing = "}";
          String dataToSend = identity + plugData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "turnLamp") {
          DynamicJsonDocument lampsData(1024);
          String head = receivedDataWebsocket["lampId"].as<String>();
          String condition = receivedDataWebsocket["condition"].as<String>();
          bool cond = condition == "true" ? true : false;
          deserializeJson(lampsData, SPIFFS_getLampData());
          lampsData[head]["condition"] = condition;
          String serialize;
          serializeJson(lampsData, serialize);
          SPIFFS_setLampData(serialize);
          WS_turn(head, cond);
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"lamp\", \"data\": ";
          String lampData = SPIFFS_getLampData();
          String closing = "}";
          String dataToSend = identity + lampData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "turnPlug") {
          DynamicJsonDocument plugsData(1024);
          String head = receivedDataWebsocket["plugId"].as<String>();
          String neck = receivedDataWebsocket["socket"].as<String>();
          String condition = receivedDataWebsocket["condition"].as<String>();
          bool cond = condition == "true" ? true : false;
          deserializeJson(plugsData, SPIFFS_getPlugData());
          plugsData[head]["sockets"][neck]["condition"] = condition;
          String serialize;
          serializeJson(plugsData, serialize);
          SPIFFS_setPlugData(serialize);
          WS_turn(head + "/" + neck, cond);
          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"plug\", \"data\": ";
          String plugData = SPIFFS_getPlugData();
          String closing = "}";
          String dataToSend = identity + plugData + closing;

          ws.textAll(dataToSend);
        }
        if (event == "addSchedule") {
          DynamicJsonDocument newSchedule(1024);
          String scheduleList = SPIFFS_getScheduleData();
          int strLength = scheduleList.length();
          String finalData;
          deserializeJson(newSchedule,
                          receivedDataWebsocket["schedule"].as<String>());
          String newScheduleStr;
          serializeJson(newSchedule, newScheduleStr);
          if (strLength > 2) {
            scheduleList.remove(strLength - 1, 1);
            finalData = scheduleList + ", " + newScheduleStr + "]";
          } else {
            finalData = "[" + newScheduleStr + "]";
          }
          SPIFFS_setScheduleData(finalData);
          SCHEDULE_build(SPIFFS_getScheduleData());
        }
        if (event == "removeSchedule") {
          String allList = SPIFFS_getScheduleData();
          int indexOfLastSchedule = allList.lastIndexOf("}, {");
          if (indexOfLastSchedule > 0) {
            allList.remove(indexOfLastSchedule + 1);
            allList += "]";
            SPIFFS_setScheduleData(allList);
            SCHEDULE_build(SPIFFS_getScheduleData());
          } else {
            allList = "[]";
            SPIFFS_setScheduleData(allList);
            SCHEDULE_build(SPIFFS_getScheduleData());
          }
        }
        if (event == "goOnline") {
          Serial.println("Switching to Online Mode");
          WIFI_setOfflineMode(false);
          Serial.println("Getting Online Schedule");
          FIREBASE_getSchedule();
          Serial.println("Finished");
          SYSTEM_turnLED(ORANGE_LED, false);
          SYSTEM_turnLED(BLUE_LED, true);
        }
        if (event == "requestDeviceData") {
          String apName = AP_SSID;
          String apPass = AP_PASS;
          String stationIP = WiFi.localIP().toString();
          String apIP = WiFi.softAPIP().toString();

          String identity =
              "{\"from\":\"center\", \"to\": \"mobile\", \"dataType\": "
              "\"deviceData\", \"data\": ";
          String deviceData = "{\"apName\":\"" + apName + "\", \"apPass\": \"" +
                              apPass + "\", \"serverName\": \"" +
                              WiFi.macAddress() + "\", \"stationIP\": \"" +
                              stationIP + "\", \"apIP\": \"" + apIP + "\"}";
          String closing = "}";
          String dataToSend = identity + deviceData + closing;

          ws.textAll(dataToSend);
        }
      }
    }
  }
}
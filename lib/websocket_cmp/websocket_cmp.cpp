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
      Serial.printf("Websocket: Websocket client #%u connected from %s\n",
                    client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("Websocket: Websocket client #%u disconnected\n",
                    client->id());
      break;
    case WS_EVT_DATA:
      WS_handleMessage(arg, data, len);
      break;
    default:
      break;
  }
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

            for (int i = 0; i < schedules.getSize(); i++) {
              if (schedules[i].trigger == "light") {
                if ((schedules[i].activeCondition == "p" &&
                     data == "Pagi/Sore" &&
                     (TIME_now().hour() >= 5 && TIME_now().hour() <= 12)) ||
                    (schedules[i].activeCondition == "si" && data == "Siang") ||
                    (schedules[i].activeCondition == "so" &&
                     data == "Pagi/Sore" &&
                     (TIME_now().hour() >= 15 && TIME_now().hour() <= 18)) ||
                    (schedules[i].activeCondition == "m" && data == "Malam")) {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = true;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
                                                  "/sockets/" + "socket-" +
                                                  (String)i + "/feedback",
                                              true);
                      }
                    } else {
                      Serial.println("GLOBAL: Can't send data to Firebase.");
                      Serial.println("GLOBAL: Reason => Offline Mode.");
                    }
                  }
                } else {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = false;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
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

            Serial.println("  > Data from Light Sensor! (LiSn)");
          } else if (sensorType == "movementSensor") {
            icon = "human";

            for (int i = 0; i < schedules.getSize(); i++) {
              if (schedules[i].trigger == "movement") {
                if ((schedules[i].activeCondition == "ag" &&
                     data == "Ada Gerakan") ||
                    (schedules[i].activeCondition == "tag" &&
                     data == "Tidak ada gerakan")) {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = true;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
                                                  "/sockets/" + "socket-" +
                                                  (String)i + "/feedback",
                                              true);
                      }
                    } else {
                      Serial.println("GLOBAL: Can't send data to Firebase.");
                      Serial.println("GLOBAL: Reason => Offline Mode.");
                    }
                  }
                } else {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = false;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
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

            Serial.println("  > Data from Movement Sensor! (MvmSn)");
          } else if (sensorType == "moistureSensor") {
            if (data == "Terlalu banyak air") {
              icon = "wetter";
            } else if (data == "Basah") {
              icon = "wet";
            } else {
              icon = "dry";
            }

            for (int i = 0; i < schedules.getSize(); i++) {
              if (schedules[i].trigger == "moisture") {
                if ((schedules[i].activeCondition == "k" && data == "Kering") ||
                    (schedules[i].activeCondition == "b" && data == "Basah") ||
                    (schedules[i].activeCondition == "tba" &&
                     data == "Terlalu banyak air")) {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = true;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
                                                  "/sockets/" + "socket-" +
                                                  (String)i + "/feedback",
                                              true);
                      }
                    } else {
                      Serial.println("GLOBAL: Can't send data to Firebase.");
                      Serial.println("GLOBAL: Reason => Offline Mode.");
                    }
                  }
                } else {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = false;
                    WS_sendMessage();
                    if (!WIFI_isOfflineMode()) {
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
                    if (!WIFI_isOfflineMode()) {
                      for (int i = 0; i < SOCKET_COUNT; i++) {
                        Firebase.RTDB.setBool(&fbdo,
                                              plugLoc + "/" +
                                                  schedules[i].targetId +
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
      }
    }
  }
}

void WS_turn(String device, bool condition) {
  target = device;
  conditionToSendWebsocket = condition;
  WS_sendMessage();
}
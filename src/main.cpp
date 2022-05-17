//$ Include Library
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#include <RTClib.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <SSD1306.h>
#include <WiFi.h>
#include <Wire.h>
#include <qrcode.h>

#include <List.hpp>

//$ Firebase Addons
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include "system_cmp.h"

//$ ESP32 Pinout
#define LED1_PIN 12
#define LED2_PIN 13
#define LED3_PIN 14
#define SCL_PIN 21
#define SDA_PIN 22

//$ Access Point Configuration
#define AP_SSID "ALiVe_AP"
#define AP_PASS "LeTS_ALiVe"
#define AP_CHANNEL 1
#define AP_DISCOVERABLE 0
#define AP_MAX_CONNECTION 10

//$ Firebase Address
#define API_KEY "AIzaSyBmp6mQthJY_3AZcA-PF0wtMPkm-SgDgEo"
#define DATABASE_URL \
  "https://"         \
  "home-automation-eee43-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "hardware@alive.me"
#define USER_PASSWORD \
  "V2hhdCBhcmUgeW91IGxvb2tpbmcgZm9yIGJ1ZGR5PwotIE1lcnphIEJvbGl2YXI="

//$ Define EEPROM Size
#define EEPROM_SIZE 512

//$ Config Address Saved in EEPROM
#define HAS_INIT 0

#define LAMP_COUNT 255
#define PLUG_COUNT 255
#define SOCKET_COUNT 5
#define SENSOR_COUNT 255

SSD1306 display(0x3C, 21, 22);
QRcode qrcode(&display);

//* Firebase Data Configuration
FirebaseData stream;
FirebaseData fbdo;

//* Firebase User Configuration
FirebaseAuth auth;
FirebaseConfig config;

//* Static IP Configuration
IPAddress IP_ADDRESS(192, 168, 5, 1);
IPAddress GATEWAY(192, 168, 5, 1);
IPAddress NETMASK(255, 255, 255, 0);

//* WiFi Setup Configuration
String wifi_buffer;
DynamicJsonDocument wifi_cred(256);
String wifi_ssid;
String wifi_pass;

//* WebSocket Configuration
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//* RTC Initialization
RTC_DS3231 rtc;
unsigned long prevMillis = 0;

//* Firebase Variable
unsigned long sendDataPrevMillis = 0;
int count = 0;
volatile boolean firebaseDataChanged = false;

//* Firebase Write Location
const String lampLoc = "homes/" + WiFi.macAddress() + "/lamps";
const String sensorLoc = "homes/" + WiFi.macAddress() + "/sensors";
const String plugLoc = "homes/" + WiFi.macAddress() + "/plugs";

//* Struct Used for Firebase Data
struct streamData {
  String streamPath;
  String dataPath;
  String dataType;
  String eventType;
  String data;
};

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

List<schedule> schedules;

//* Initialize Struct Data
streamData receivedDataFirebase;

//* Global Variable Declaration

String target;
boolean conditionToSendWebsocket;
boolean isOfflineMode = false;

//* Global Function Declaration
String getValue(String data, char separator, int index);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void sendMessage();
void settingWiFiCred(String ssid, String pass);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWiFiFile();
void initMainFile();
void getWiFiCredFromSPIFFS();
void getSchedulesData();

//* VOID SETUP
void setup() {
  SYSTEM_init();

  if (!rtc.begin()) {
    Serial.println("GLOBAL: RTC Not Connected");
  }

  EEPROM.begin(EEPROM_SIZE);

  display.init();
  display.write("Hello!");
  display.display();

  qrcode.init();

  char hasInit = EEPROM.read(HAS_INIT);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS: Error Mount SPIFFS");
    return;
  }

  if (hasInit == 255) {
    Serial.println("Initializing Everything");

    initWiFiFile();
    initMainFile();

    EEPROM.write(HAS_INIT, 1);
    EEPROM.commit();
  }

  getWiFiCredFromSPIFFS();

  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  Serial.println();
  Serial.print("WiFi Station: Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() <= 15000) {
    Serial.print(".");
    delay(300);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(
        "GLOBAL: Tidak dapat terhubung dengan konfigurasi WiFi yang "
        "diberikan.");
    Serial.println("GLOBAL: Beralih ke mode Offline.");
    isOfflineMode = true;
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi Station: Terhubung ke WiFi dengan IP Address: ");
    Serial.println(WiFi.localIP());
  }

  Serial.println();

  WiFi.softAPConfig(IP_ADDRESS, GATEWAY, NETMASK);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_DISCOVERABLE, AP_MAX_CONNECTION);

  String apName = AP_SSID;
  String apPass = AP_PASS;
  String stationIP = WiFi.localIP().toString();
  String apIP = WiFi.softAPIP().toString();

  Serial.print("WiFi AP: IP Address Access Point: ");
  Serial.println(WiFi.softAPIP());

  initWebSocket();
  server.begin();

  if (!isOfflineMode) {
    config.api_key = API_KEY;

    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    config.database_url = DATABASE_URL;

    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    if (!Firebase.RTDB.beginStream(&stream, "/"))
      Serial.printf("Firebase: Stream begin error, %s\n\n",
                    stream.errorReason().c_str());

    Firebase.RTDB.setStreamCallback(&stream, streamCallback,
                                    streamTimeoutCallback);

    if (Firebase.RTDB.getString(&fbdo,
                                "homes/" + WiFi.macAddress() + "/macAddress")) {
      if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
        Serial.println(fbdo.to<String>());
      }
    } else {
      Serial.println(
          "GLOBAL: Device isn't registered in Firebase, registering...");
      Firebase.RTDB.setString(&fbdo,
                              "homes/" + WiFi.macAddress() + "/macAddress",
                              WiFi.macAddress());
    }

    Serial.println();
    getSchedulesData();
  }

  display.clear();
  String dataToSend = "{\"apName\":\"" + apName + "\", \"apPass\": \"" +
                      apPass + "\", \"serverName\": \"" + WiFi.macAddress() +
                      "\", \"stationIP\": \"" + stationIP + "\", \"apIP\": \"" +
                      apIP + "\"}";
  qrcode.create(dataToSend);
}

//* VOID LOOP
void loop() {
  ws.cleanupClients();

  DateTime now = rtc.now();
  if (Firebase.isTokenExpired() == true) {
    Serial.println("Firebase Token Expired");
    Serial.println("Restarting system to get new token...");
    ESP.restart();
  }

  if (millis() - prevMillis >= 5000) {
    prevMillis = millis();

    Serial.println("================================================");
    Serial.println(String(now.hour()) + " : " + String(now.minute()));
    Serial.println("================================================");

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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
                  sendMessage();
                  if (!isOfflineMode) {
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
                  sendMessage();
                  if (!isOfflineMode) {
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
                  sendMessage();
                  if (!isOfflineMode) {
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
                  sendMessage();
                  if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
              sendMessage();
              if (!isOfflineMode) {
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
            sendMessage();
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
            sendMessage();
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
                sendMessage();
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
                sendMessage();
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
      getSchedulesData();
    }
  }
}

//* Global Function Definition
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void streamCallback(FirebaseStream data) {
  receivedDataFirebase.streamPath = data.streamPath();
  receivedDataFirebase.dataPath = data.dataPath();
  receivedDataFirebase.dataType = data.dataType();
  receivedDataFirebase.eventType = data.eventType();
  receivedDataFirebase.data = data.stringData();

  firebaseDataChanged = true;
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Firebase: Stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("Firebase: Error code => %d, Error reason => %s\n\n",
                  stream.httpCode(), stream.errorReason().c_str());
}

void sendMessage() {
  DynamicJsonDocument dataToSendWebsocket(256);

  dataToSendWebsocket["from"] = "center";
  dataToSendWebsocket["to"] = target;
  dataToSendWebsocket["condition"] = String(conditionToSendWebsocket);

  String sendData;
  serializeJson(dataToSendWebsocket, sendData);
  Serial.println(sendData);

  ws.textAll(sendData);
}

void settingWiFiCred(String ssid, String pass) {
  File write = SPIFFS.open("/wifi_cred.json", FILE_WRITE);

  if (!write) {
    Serial.println("Error Open File 1");
    return;
  }

  String dataToWrite =
      "{\"ssid\": \"" + ssid + "\", \"pass\": \"" + pass + "\"}";

  if (write.print(dataToWrite)) {
    Serial.println("SPIFFS: Write Success");
  } else {
    Serial.println("SPIFFS: Write Failed");
  }

  write.close();
  ESP.restart();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
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

          if (!isOfflineMode) {
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
                     (rtc.now().hour() >= 5 && rtc.now().hour() <= 12)) ||
                    (schedules[i].activeCondition == "si" && data == "Siang") ||
                    (schedules[i].activeCondition == "so" &&
                     data == "Pagi/Sore" &&
                     (rtc.now().hour() >= 15 && rtc.now().hour() <= 18)) ||
                    (schedules[i].activeCondition == "m" && data == "Malam")) {
                  if (schedules[i].target == "lamp") {
                    target = schedules[i].targetId;
                    conditionToSendWebsocket = true;
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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
                    sendMessage();
                    if (!isOfflineMode) {
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

          if (!isOfflineMode) {
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

          settingWiFiCred(ssid, pass);
        }
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
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
      handleWebSocketMessage(arg, data, len);
      break;
    default:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initWiFiFile() {
  File file = SPIFFS.open("/wifi_cred.json", FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  String message =
      "{\"ssid\": \"ZTE_2.4G_bcr2p4\", \"pass\": \"tokinyong_2Sm2HVMq\"}";
  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void initMainFile() {
  File file = SPIFFS.open("/main.json", FILE_WRITE);
  if (!file) {
    Serial.println("- Failed to open file for writing");
    return;
  }

  String message = "{\"lamps\": {}, \"plugs\": {}, \"sensors\": {}}";

  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void getWiFiCredFromSPIFFS() {
  File file = SPIFFS.open("/wifi_cred.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return;
  }

  while (file.available()) {
    wifi_buffer += file.readString();
  }

  deserializeJson(wifi_cred, wifi_buffer);

  wifi_ssid = wifi_cred["ssid"].as<String>();
  wifi_pass = wifi_cred["pass"].as<String>();

  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);
}

void getSchedulesData() {
  if (Firebase.RTDB.getString(&fbdo,
                              "homes/" + WiFi.macAddress() + "/schedules")) {
    schedules.clear();
    DynamicJsonDocument listJson(1024);
    DynamicJsonDocument sD(1024);
    deserializeJson(listJson, fbdo.to<String>());
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
    Serial.println(i);
  } else {
    Serial.println("GLOBAL: No Schedule.");
  }
}
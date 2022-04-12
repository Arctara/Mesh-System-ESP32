/*
    PINOUT
      PIN 12 = LED 1
      PIN 13 = LED 2
      PIN 14 = LED 3
      PIN 21 = SCL
      PIN 22 = SDA
*/

//$ Include Library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <painlessMesh.h>

//$ Firebase Addons
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

//$ Mesh Configuration
// #define MESH_PREFIX "ALiVe_MESH"
// #define MESH_PASSWORD "TmlhdCBzZWthbGkgYW5kYSBtZW5kZWNvZGUgaW5pIC1NZXJ6YQ=="
// #define MESH_PORT 5555

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
#define USER_EMAIL "Merza.bolivar@Gmail.com"
#define USER_PASSWORD "iniPassword?"

// //$ OLED SCREEN
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

// #define OLED_RESET -1
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// #define NUMFLAKES 10

//* Firebase Configuration
FirebaseData stream;
FirebaseData fbdo;

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

//* Firebase Variable
unsigned long sendDataPrevMillis = 0;
int count = 0;
volatile bool dataChanged = false;

//* Firebase Write Location
const String movementSensorLoc = "sensors/sensor-1";
const String lightSensorLoc = "sensors/sensor-2";
const String moistureSensorLoc = "sensors/sensor-3";
const String plugLoc = "plugs";

//*Mesh Configuration
Scheduler userScheduler;
painlessMesh mesh;
int nodeNumber = 0;

struct streamData {
  String streamPath;
  String dataPath;
  String dataType;
  String eventType;
  String data;
};

streamData receivedData;

DynamicJsonDocument meshSendData(256);
DynamicJsonDocument meshReceivedData(256);
String target;
bool conditionToSend;

//! ##### START FUNCTION DECLARATION #####

//! >> START PURE FUNCTION DECLARATION <<
String getValue(String data, char separator, int index);
//! >> END PURE FUNCTION DECLARATION <<

//! >> START FIREBASE FUNCTION DECLARATION <<
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
//! >> END FIREBASE FUNCTION DECLARATION

//! >> START WEBSOCKET FUNCTION DECLARATION <<
void sendMessage();
void spiffsWriteData(char *data);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len);
//! >> END WEBSOCKET FUNCTION DECLARATION <<

//! >> START MESH FUNCTION DECLARATION <<
// void sendMessage();

// //$ Needed for painless mesh library
// void receivedCallback(uint32_t from, String &msg);
// void newConnectionCallback(uint32_t nodeId);
// void changedConnectionCallback();
// void nodeTimeAdjustedCallback(int32_t offset);
//! >> END MESH FUNCTION DECLARATION <<

//! ##### END FUNCTION DECLARATION #####

//! >>>>> START VOID SETUP <<<<<
void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("Error Mount SPIFFS");
    return;
  }

  File file = SPIFFS.open("/wifi_cred.json");

  if (!file) {
    Serial.println("Error Open File 2");
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

  // mesh.setDebugMsgTypes(ERROR | STARTUP);

  // mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA);
  // mesh.onReceive(&receivedCallback);
  // mesh.onNewConnection(&newConnectionCallback);
  // mesh.onChangedConnections(&changedConnectionCallback);
  // mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // mesh.stationManual("ZTE_2.4G_bcr2p4", "tokinyong_2Sm2HVMq");
  // mesh.setRoot(true);
  // mesh.setContainsRoot(true);

  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  Serial.println();
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
  }
  Serial.println();

  Serial.print("Terhubung ke WiFi dengan IP Address: ");
  Serial.println(WiFi.localIP());

  WiFi.softAPConfig(IP_ADDRESS, GATEWAY, NETMASK);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_DISCOVERABLE, AP_MAX_CONNECTION);

  Serial.print("IP Address Access Point: ");
  Serial.println(WiFi.softAPIP());

  initWebSocket();
  server.begin();

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&stream, "/"))
    Serial.printf("Stream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&stream, streamCallback,
                                  streamTimeoutCallback);
}
//! >>>>> END VOID SETUP <<<<<

//! >>>>> START VOID LOOP <<<<<
void loop() {
  ws.cleanupClients();
  // mesh.update();
  if (dataChanged) {
    dataChanged = false;
    String deviceType = getValue(receivedData.dataPath, '/', 1);
    String deviceName = getValue(receivedData.dataPath, '/', 2);
    String deviceCondition = receivedData.data;

    if (deviceType == "lamps") {
      if (deviceName == "lamp-1") {
        if (deviceCondition == "true") {
          target = "lamp-1";
          conditionToSend = true;
          sendMessage();
          Serial.println("Lampu 1 menyala!");
        } else {
          target = "lamp-1";
          conditionToSend = false;
          sendMessage();
          Serial.println("Lampu 1 mati!");
        }
      }
    } else if (deviceType == "plugs") {
      if (deviceName == "plug-1") {
        if (deviceCondition == "true") {
          target = "plug-1";
          conditionToSend = true;
          sendMessage();
          Serial.println("Stopkontak 1 menyala!");
        } else {
          target = "plug-1";
          conditionToSend = false;
          sendMessage();
          Serial.println("Stopkontak 1 mati!");
        }
      } else if (deviceName == "plug-2") {
        if (deviceCondition == "true") {
          target = "plug-2";
          conditionToSend = true;
          sendMessage();
          Serial.println("Stopkontak 2 menyala!");
        } else {
          target = "plug-2";
          conditionToSend = false;
          sendMessage();
          Serial.println("Stopkontak 2 mati!");
        }
      } else if (deviceName == "plug-3") {
        if (deviceCondition == "true") {
          target = "plug-3";
          conditionToSend = true;
          sendMessage();
          Serial.println("Stopkontak 3 menyala!");
        } else {
          target = "plug-3";
          conditionToSend = false;
          sendMessage();
          Serial.println("Stopkontak 3 mati!");
        }
      } else if (deviceName == "plug-4") {
        if (deviceCondition == "true") {
          target = "plug-4";
          conditionToSend = true;
          sendMessage();
          Serial.println("Stopkontak 4 menyala!");
        } else {
          target = "plug-4";
          conditionToSend = false;
          sendMessage();
          Serial.println("Stopkontak 4 mati!");
        }
      } else if (deviceName == "plug-5") {
        if (deviceCondition == "true") {
          target = "plug-5";
          conditionToSend = true;
          sendMessage();
          Serial.println("Stopkontak 5 menyala!");
        } else {
          target = "plug-5";
          conditionToSend = false;
          sendMessage();
          Serial.println("Stopkontak 5 mati!");
        }
      }
    }
  }
}
//! >>>>> END VOID LOOP <<<<<

//! ##### START FUNCTION DEFINITION #####

//! >> START PURE FUNCTION <<
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
//! >> END PURE FUNCTION <<

//! >> START FIREBASE FUNCTION DEFINITION <<
void streamCallback(FirebaseStream data) {
  receivedData.streamPath = data.streamPath();
  receivedData.dataPath = data.dataPath();
  receivedData.dataType = data.dataType();
  receivedData.eventType = data.eventType();
  receivedData.data = data.stringData();

  dataChanged = true;
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("Error code: %d, Error reason: %s\n\n", stream.httpCode(),
                  stream.errorReason().c_str());
}
//! >> END FIREBASE FUNCTION DEFINITION <<

//! >> START WEBSOCKET FUNCTION DEFINITION <<
void sendMessage() {
  meshSendData["from"] = "center";
  meshSendData["to"] = target;
  meshSendData["condition"] = String(conditionToSend);

  String sendData;
  serializeJson(meshSendData, sendData);
  Serial.println(sendData);

  ws.textAll(sendData);
}

void spiffsWriteData(char *data) {
  File write = SPIFFS.open("/wifi_cred.json", FILE_WRITE);

  if (!write) {
    Serial.println("Error Open File 1");
    return;
  }

  if (write.print(data)) {
    Serial.println("Write Success");
  } else {
    Serial.println("Write Failed");
  }

  write.close();
  ESP.restart();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT) {
    data[len] = 0;

    deserializeJson(meshReceivedData, data);

    String from = meshReceivedData["from"].as<String>();
    String to = meshReceivedData["to"].as<String>();

    if (to == "center") {
      Serial.println("! Data for this device!");
      if (from == "plugFamily") {
        Serial.println("  > Data from Plug Family!");

        String child = meshReceivedData["child"].as<String>();
        bool condition = meshReceivedData["condition"].as<bool>();

        Serial.println("    ==> " + child + " . " + condition);

        Firebase.RTDB.setBool(&fbdo, plugLoc + "/" + child + "/condition",
                              condition);
      }

      if (from == "light-sensor") {
        Serial.println("  > Data from Light Sensor!");

        String data = meshReceivedData["data"].as<String>();
        String icon = "";

        if (data == "Siang") {
          icon = "sun";
        } else if (data == "Malam") {
          icon = "moon";
        } else {
          icon = "sun";
        }

        Serial.println("    ==> " + data);

        Firebase.RTDB.setString(&fbdo, lightSensorLoc + "/reading", data);
        Firebase.RTDB.setString(&fbdo, lightSensorLoc + "/icon", icon);
      }

      if (from == "movement-sensor") {
        Serial.println("  > Data from Movement Sensor!");

        String data = meshReceivedData["data"].as<String>();
        String icon = "human";

        Serial.println("    ==> " + data);

        Firebase.RTDB.setString(&fbdo, movementSensorLoc + "/reading", data);
        Firebase.RTDB.setString(&fbdo, movementSensorLoc + "/icon", icon);
      }

      if (from == "moisture-sensor") {
        Serial.println("  > Data from Moisture Sensor!");

        String data = meshReceivedData["data"].as<String>();
        String icon = "";

        if (data == "Terlalu banyak air") {
          icon = "wetter";
        } else if (data == "Basah") {
          icon = "wet";
        } else {
          icon = "dry";
        }

        Serial.println("    ==> " + data);

        Firebase.RTDB.setString(&fbdo, moistureSensorLoc + "/reading", data);
        Firebase.RTDB.setString(&fbdo, moistureSensorLoc + "/icon", icon);
      }
    }
    // spiffsWriteData((char *)data);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("Websocket client #%u connected from %s\n", client->id(),
                    client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("Websocket client #%u disconnected\n", client->id());
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
//! >> END WEBSOCKET FUNCTION DEFINITION <<

//! >> START MESH FUNCTION DEFINITION <<
// void sendMessage() {
//   meshSendData["from"] = "center";
//   meshSendData["to"] = target;
//   meshSendData["condition"] = conditionToSend;
//   String sendData;
//   serializeJson(meshSendData, sendData);
//   Serial.println(sendData);
//   mesh.sendBroadcast(sendData);
// }

// //$ Needed for painless mesh library
// void receivedCallback(uint32_t from, String &msg) {
//   Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
//   deserializeJson(meshReceivedData, msg);
// }

// void newConnectionCallback(uint32_t nodeId) {
//   Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
// }

// void changedConnectionCallback() { Serial.printf("Changed connections\n"); }

// void nodeTimeAdjustedCallback(int32_t offset) {
//   Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),
//   offset);
// }
//! >> END MESH FUNCTION DEFINITION <<

//! ##### END FUNCTION DEFINITION #####
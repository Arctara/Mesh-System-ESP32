//$ Include Library
#include <Arduino.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <SSD1306.h>
#include <WiFi.h>
#include <Wire.h>
#include <painlessMesh.h>
#include <qrcode.h>

//$ Firebase Addons
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

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
#define LAMP_COUNTER 1
#define PLUG_COUNTER 2
#define SENSOR_COUTNER 3

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

DynamicJsonDocument main_data(1024);
DynamicJsonDocument lamps(1024);
DynamicJsonDocument plugs(1024);
DynamicJsonDocument sensors(1024);
String main_buffer;

//* WebSocket Configuration
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//* Firebase Variable
unsigned long sendDataPrevMillis = 0;
int count = 0;
volatile boolean firebaseDataChanged = false;

//* Firebase Write Location
const String movementSensorLoc =
    "homes/" + WiFi.macAddress() + "/sensors/sensor-1";
const String lightSensorLoc =
    "homes/" + WiFi.macAddress() + "/sensors/sensor-2";
const String moistureSensorLoc =
    "homes/" + WiFi.macAddress() + "/sensors/sensor-3";
const String plugLoc = "homes/" + WiFi.macAddress() + "/plugs";

//* Struct Used for Firebase Data
struct streamData {
  String streamPath;
  String dataPath;
  String dataType;
  String eventType;
  String data;
};

//* Initialize Struct Data
streamData receivedDataFirebase;

//* Global Variable Declaration
DynamicJsonDocument dataToSendWebsocket(256);
DynamicJsonDocument receivedDataWebsocket(256);
String target;
boolean conditionToSendWebsocket;
boolean isOfflineMode = false;
char lampCounter, plugCounter, sensorCounter;

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
void getMainDataFromSPIFFS();

//* VOID SETUP
void setup() {
  Serial.begin(115200);

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
    EEPROM.write(LAMP_COUNTER, 1);
    EEPROM.write(PLUG_COUNTER, 1);
    EEPROM.write(SENSOR_COUTNER, 1);
    EEPROM.commit();
  }

  lampCounter = EEPROM.read(LAMP_COUNTER);
  plugCounter = EEPROM.read(PLUG_COUNTER);
  sensorCounter = EEPROM.read(SENSOR_COUTNER);

  getWiFiCredFromSPIFFS();
  getMainDataFromSPIFFS();

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

  display.clear();
  String dataToSend = "{\"apName\":\"" + apName + "\", \"apPass\": \"" +
                      apPass + "\", \"serverName\": \"" + WiFi.macAddress() +
                      "\", \"stationIP\": \"" + stationIP + "\", \"apIP\": \"" +
                      apIP + "\"}";
  qrcode.create(dataToSend);

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
  }
}

//* VOID LOOP
void loop() {
  ws.cleanupClients();

  if (firebaseDataChanged) {
    firebaseDataChanged = false;
    String deviceType = getValue(receivedDataFirebase.dataPath, '/', 3);
    String deviceName = getValue(receivedDataFirebase.dataPath, '/', 4);
    String deviceCondition = receivedDataFirebase.data;

    if (deviceType == "lamps") {
      if (deviceName == "lamp-1") {
        if (deviceCondition == "true") {
          target = "lamp-1";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Lampu 1 menyala!");
        } else {
          target = "lamp-1";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Lampu 1 mati!");
        }
      } else if (deviceName == "lamp-2") {
        if (deviceCondition == "true") {
          target = "lamp-2";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Lampu 2 menyala!");
        } else {
          target = "lamp-2";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Lampu 2 mati!");
        }
      } else if (deviceName == "lamp-3") {
        if (deviceCondition == "true") {
          target = "lamp-3";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Lampu 3 menyala!");
        } else {
          target = "lamp-3";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Lampu 3 mati!");
        }
      } else if (deviceName == "lamp-4") {
        if (deviceCondition == "true") {
          target = "lamp-4";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Lampu 4 menyala!");
        } else {
          target = "lamp-4";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Lampu 4 mati!");
        }
      } else if (deviceName == "lamp-5") {
        if (deviceCondition == "true") {
          target = "lamp-5";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Lampu 5 menyala!");
        } else {
          target = "lamp-5";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Lampu 5 mati!");
        }
      }
    } else if (deviceType == "plugs") {
      String deviceSubName = getValue(receivedDataFirebase.dataPath, '/', 6);

      if (deviceName == "plug-1") {
        if (deviceSubName == "socket-1") {
          if (deviceCondition == "true") {
            target = "plug-1/socket-1";
            conditionToSendWebsocket = true;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 1 menyala!");
          } else {
            target = "plug-1/socket-1";
            conditionToSendWebsocket = false;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 1 mati!");
          }
        } else if (deviceSubName == "socket-2") {
          if (deviceCondition == "true") {
            target = "plug-1/socket-2";
            conditionToSendWebsocket = true;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 2 menyala!");
          } else {
            target = "plug-1/socket-2";
            conditionToSendWebsocket = false;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 2 mati!");
          }
        } else if (deviceSubName == "socket-3") {
          if (deviceCondition == "true") {
            target = "plug-1/socket-3";
            conditionToSendWebsocket = true;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 3 menyala!");
          } else {
            target = "plug-1/socket-3";
            conditionToSendWebsocket = false;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 3 mati!");
          }
        } else if (deviceSubName == "socket-4") {
          if (deviceCondition == "true") {
            target = "plug-1/socket-4";
            conditionToSendWebsocket = true;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 4 menyala!");
          } else {
            target = "plug-1/socket-4";
            conditionToSendWebsocket = false;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 4 mati!");
          }
        } else if (deviceSubName == "socket-5") {
          if (deviceCondition == "true") {
            target = "plug-1/socket-5";
            conditionToSendWebsocket = true;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 5 menyala!");
          } else {
            target = "plug-1/socket-5";
            conditionToSendWebsocket = false;
            sendMessage();
            Serial.println("GLOBAL: Stopkontak 1 => Socket 5 mati!");
          }
        }
      }
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

    deserializeJson(receivedDataWebsocket, data);

    String from = receivedDataWebsocket["from"].as<String>();
    String to = receivedDataWebsocket["to"].as<String>();

    if (to == "center") {
      Serial.println("! Data for Center!");
      if (from == "plug-1") {
        Serial.println("  > Data from Plug! (Plg)");

        String socket = receivedDataWebsocket["socket"].as<String>();
        bool condition = receivedDataWebsocket["condition"].as<bool>();

        Serial.println("    ==> " + socket + " = " + condition);

        if (!isOfflineMode) {
          Firebase.RTDB.setBool(
              &fbdo, plugLoc + "/" + from + "/sockets/" + socket + "/feedback",
              condition);
        } else {
          Serial.println("GLOBAL: Can't send data to Firebase.");
          Serial.println("GLOBAL: Reason => Offline Mode.");
        }
      }

      if (from == "lamp-1") {
        Serial.println("  > Data from Lamp! (lmp)");

        String feedback = receivedDataWebsocket["feedback"].as<String>();

        Serial.println("    ==> " + feedback);
      }

      if (from == "light-sensor") {
        Serial.println("  > Data from Light Sensor! (LiSn)");

        String data = receivedDataWebsocket["data"].as<String>();
        String icon = "";

        if (data == "Siang") {
          icon = "sun";
        } else if (data == "Malam") {
          icon = "moon";
        } else {
          icon = "sun";
        }

        Serial.println("    ==> " + data);

        if (!isOfflineMode) {
          Firebase.RTDB.setString(&fbdo, lightSensorLoc + "/reading", data);
          Firebase.RTDB.setString(&fbdo, lightSensorLoc + "/icon", icon);
        } else {
          Serial.println("GLOBAL: Can't send data to Firebase.");
          Serial.println("GLOBAL: Reason => Offline Mode.");
        }
      }

      if (from == "movement-sensor") {
        Serial.println("  > Data from Movement Sensor! (MvmSn)");

        String data = receivedDataWebsocket["data"].as<String>();
        String icon = "human";

        Serial.println("    ==> " + data);

        if (!isOfflineMode) {
          Firebase.RTDB.setString(&fbdo, movementSensorLoc + "/reading", data);
          Firebase.RTDB.setString(&fbdo, movementSensorLoc + "/icon", icon);
        } else {
          Serial.println("GLOBAL: Can't send data to Firebase.");
          Serial.println("GLOBAL: Reason => Offline Mode.");
        }
      }

      if (from == "moisture-sensor") {
        Serial.println("  > Data from Moisture Sensor! (MstSn)");

        String data = receivedDataWebsocket["data"].as<String>();
        String icon = "";

        if (data == "Terlalu banyak air") {
          icon = "wetter";
        } else if (data == "Basah") {
          icon = "wet";
        } else {
          icon = "dry";
        }

        Serial.println("    ==> " + data);

        if (!isOfflineMode) {
          Firebase.RTDB.setString(&fbdo, moistureSensorLoc + "/reading", data);
          Firebase.RTDB.setString(&fbdo, moistureSensorLoc + "/icon", icon);
        } else {
          Serial.println("GLOBAL: Can't send data to Firebase.");
          Serial.println("GLOBAL: Reason => Offline Mode.");
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

void getMainDataFromSPIFFS() {
  File file = SPIFFS.open("/main.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File");
    return;
  }

  while (file.available()) {
    main_buffer += file.readString();
  }

  deserializeJson(main_data, main_buffer);
}
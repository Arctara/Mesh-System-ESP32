//$ Include Library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
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

//$ Bluetooth UUID
#define SERVICE_UUID "44e6b26b-0386-44d5-8848-197c988837e9"
#define CHARACTERISTIC_UUID "bc08d402-14bb-4002-b0ca-b47b4eb78df7"

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
#define USER_EMAIL "Merza.bolivar@Gmail.com"
#define USER_PASSWORD "iniPassword?"

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

//* Firebase Variable
unsigned long sendDataPrevMillis = 0;
int count = 0;
volatile boolean firebaseDataChanged = false;

//* Firebase Write Location
const String movementSensorLoc = "sensors/sensor-1";
const String lightSensorLoc = "sensors/sensor-2";
const String moistureSensorLoc = "sensors/sensor-3";
const String plugLoc = "plugs";

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
boolean bluetoothDataReceived;
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

//* VOID SETUP
void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS: Error Mount SPIFFS");
    return;
  }

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

  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  Serial.println();
  Serial.print("WiFi Station: Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() <= 15000) {
    Serial.print(".");
    delay(300);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi Station: Terhubung ke WiFi dengan IP Address: ");
    Serial.println(WiFi.localIP());
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println(
        "GLOBAL: Tidak dapat terhubung dengan konfigurasi WiFi yang "
        "diberikan.");
    Serial.println("GLOBAL: Beralih ke mode Offline.");
    isOfflineMode = true;
    WiFi.mode(WIFI_AP);

    BLEDevice::init("ALiVe_Server");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pCharacteristic->setValue("Hello World says Me!");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now it can be read from phone!");
  }

  Serial.println();

  WiFi.softAPConfig(IP_ADDRESS, GATEWAY, NETMASK);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_DISCOVERABLE, AP_MAX_CONNECTION);

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
  }
}

//* VOID LOOP
void loop() {
  ws.cleanupClients();

  if (firebaseDataChanged) {
    firebaseDataChanged = false;
    String deviceType = getValue(receivedDataFirebase.dataPath, '/', 1);
    String deviceName = getValue(receivedDataFirebase.dataPath, '/', 2);
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
      }
    } else if (deviceType == "plugs") {
      if (deviceName == "plug-1") {
        if (deviceCondition == "true") {
          target = "plug-1";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 1 menyala!");
        } else {
          target = "plug-1";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 1 mati!");
        }
      } else if (deviceName == "plug-2") {
        if (deviceCondition == "true") {
          target = "plug-2";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 2 menyala!");
        } else {
          target = "plug-2";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 2 mati!");
        }
      } else if (deviceName == "plug-3") {
        if (deviceCondition == "true") {
          target = "plug-3";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 3 menyala!");
        } else {
          target = "plug-3";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 3 mati!");
        }
      } else if (deviceName == "plug-4") {
        if (deviceCondition == "true") {
          target = "plug-4";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 4 menyala!");
        } else {
          target = "plug-4";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 4 mati!");
        }
      } else if (deviceName == "plug-5") {
        if (deviceCondition == "true") {
          target = "plug-5";
          conditionToSendWebsocket = true;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 5 menyala!");
        } else {
          target = "plug-5";
          conditionToSendWebsocket = false;
          sendMessage();
          Serial.println("GLOBAL: Stopkontak 5 mati!");
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
      if (from == "plugFamily") {
        Serial.println("  > Data from Plug Family! (PlFm)");

        String child = receivedDataWebsocket["child"].as<String>();
        bool condition = receivedDataWebsocket["condition"].as<bool>();

        Serial.println("    ==> " + child + " = " + condition);

        if (!isOfflineMode) {
          Firebase.RTDB.setBool(&fbdo, plugLoc + "/" + child + "/condition",
                                condition);
        } else {
          Serial.println("GLOBAL: Can't send data to Firebase.");
          Serial.println("GLOBAL: Reason => Offline Mode.");
        }
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
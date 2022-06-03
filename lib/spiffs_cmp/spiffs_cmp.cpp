#include "spiffs_cmp.h"

#include <ArduinoJson.h>

String wifi_ssid = "";
String wifi_pass = "";

void SPIFFS_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS: Error Mount SPIFFS");
    return;
  }
}

void SPIFFS_initWiFiFile() {
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
void SPIFFS_initLampData() {
  File file = SPIFFS.open("/lamp.json", FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  String message =
      "{\"lamp-1\": {\"condition\": \"true\", \"name\": \"Lampu Kamar\"}}";
  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void SPIFFS_initPlugData() {
  File file = SPIFFS.open("/plug.json", FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  String message =
      "{\"plug-1\": {\"name\": \"Stopkontak Kamar\",\"sockets\": "
      "{\"socket-1\": {\"condition\": \"false\",\"feedback\": "
      "\"false\",\"name\": \"socket-1\"},\"socket-2\": {\"condition\" : "
      "\"false\", \"feedback\" : \"false\",\"name\" : "
      "\"socket-2\"},\"socket-3\": {\"condition\" : \"false\", \"feedback\" : "
      "\"false\", \"name\" : \"socket-3\"},\"socket-4\": {\"condition\" : "
      "\"false\", \"feedback\" : \"false\", \"name\" : "
      "\"socket-4\"},\"socket-5\" : {\"condition\" : \"false\", \"feedback\" : "
      "\"false\", \"name\" : \"socket-5\"}}}}";
  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void SPIFFS_initScheduleData() {
  File file = SPIFFS.open("/schedule.json", FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  String message = "[]";
  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void SPIFFS_initSensorData() {
  File file = SPIFFS.open("/sensor.json", FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  String message =
      "{\"sensor-1\": {\"icon\": \"sun\",\"name\": \"Sensor "
      "Cahaya\",\"reading\": \"Siang\",\"sensorType\": \"lightSensor\"}, "
      "\"sensor-2\" : {\"icon\": \"human\",\"name\": \"Sensor "
      "Gerak\",\"reading\" : \"Tidak ada gerakan\",\"sensorType\" : "
      "\"movementSensor\"}, \"sensor-3\" : {\"icon\" : \"wet\",\"name\" : "
      "\"Sensor Kelembaban Tanah\",\"reading\" : \"Basah\",\"sensorType\" : "
      "\"moistureSensor\"}}";
  if (file.print(message)) {
    Serial.println("- File Written");
  } else {
    Serial.println("- Write Failed");
  }
}

void SPIFFS_getWiFiCred() {
  String wifi_buffer;
  DynamicJsonDocument wifi_cred(256);

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

void SPIFFS_setWiFiCred(String ssid, String pass) {
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

String SPIFFS_getLampData() {
  String lampBuffer;

  File file = SPIFFS.open("/lamp.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return "null";
  }

  while (file.available()) {
    lampBuffer += file.readString();
  }

  return lampBuffer;
}
void SPIFFS_setLampData(String source) {
  File write = SPIFFS.open("/lamp.json", FILE_WRITE);

  if (!write) {
    Serial.println("Error Open File lamp");
    return;
  }

  if (write.print(source)) {
    Serial.println("SPIFFS: Write Success");
  } else {
    Serial.println("SPIFFS: Write Failed");
  }

  write.close();
}

String SPIFFS_getPlugData() {
  String plugBuffer;

  File file = SPIFFS.open("/plug.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return "null";
  }

  while (file.available()) {
    plugBuffer += file.readString();
  }

  return plugBuffer;
}
void SPIFFS_setPlugData(String source) {
  File write = SPIFFS.open("/plug.json", FILE_WRITE);

  if (!write) {
    Serial.println("Error Open File lamp");
    return;
  }

  if (write.print(source)) {
    Serial.println("SPIFFS: Write Success");
  } else {
    Serial.println("SPIFFS: Write Failed");
  }

  write.close();
}

String SPIFFS_getScheduleData() {
  String scheduleBuffer;

  File file = SPIFFS.open("/schedule.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return "null";
  }

  while (file.available()) {
    scheduleBuffer += file.readString();
  }

  return scheduleBuffer;
}
void SPIFFS_setScheduleData(String source) {
  File write = SPIFFS.open("/schedule.json", FILE_WRITE);

  if (!write) {
    Serial.println("Error Open File lamp");
    return;
  }

  if (write.print(source)) {
    Serial.println("SPIFFS: Write Success");
  } else {
    Serial.println("SPIFFS: Write Failed");
  }

  write.close();
}

String SPIFFS_getSensorData() {
  String sensorBuffer;

  File file = SPIFFS.open("/sensor.json");

  if (!file) {
    Serial.println("SPIFFS: Error Open File 2");
    return "null";
  }

  while (file.available()) {
    sensorBuffer += file.readString();
  }

  return sensorBuffer;
}
void SPIFFS_setSensorData(String source) {}
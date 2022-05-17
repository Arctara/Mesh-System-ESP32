#include "firebase_cmp.h"

//* Firebase Data Configuration
FirebaseData stream;
FirebaseData fbdo;

//* Firebase User Configuration
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
volatile boolean firebaseDataChanged = false;

//* Initialize Struct Data
streamData receivedDataFirebase;

void FIREBASE_init() {
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void FIREBASE_streamCallback(FirebaseStream data) {
  receivedDataFirebase.streamPath = data.streamPath();
  receivedDataFirebase.dataPath = data.dataPath();
  receivedDataFirebase.dataType = data.dataType();
  receivedDataFirebase.eventType = data.eventType();
  receivedDataFirebase.data = data.stringData();

  firebaseDataChanged = true;
}

void FIREBASE_streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Firebase: Stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("Firebase: Error code => %d, Error reason => %s\n\n",
                  stream.httpCode(), stream.errorReason().c_str());
}
void FIREBASE_initStream() {
  if (!Firebase.RTDB.beginStream(&stream, "/"))
    Serial.printf("Firebase: Stream begin error, %s\n\n",
                  stream.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&stream, FIREBASE_streamCallback,
                                  FIREBASE_streamTimeoutCallback);
}

void FIREBASE_initDeviceData() {
  if (Firebase.RTDB.getString(&fbdo,
                              "homes/" + WiFi.macAddress() + "/macAddress")) {
    if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
      Serial.println(fbdo.to<String>());
      Serial.println();
    }
  } else {
    Serial.println(
        "GLOBAL: Device isn't registered in Firebase, registering...");
    Serial.println();
    Firebase.RTDB.setString(&fbdo, "homes/" + WiFi.macAddress() + "/macAddress",
                            WiFi.macAddress());
  }
}

void FIREBASE_getSchedule() {
  if (Firebase.RTDB.getString(&fbdo,
                              "homes/" + WiFi.macAddress() + "/schedules")) {
    SCHEDULE_build(fbdo.to<String>());
  } else {
    SCHEDULE_removeAll();
    Serial.println("GLOBAL: No Schedule.");
  }
}

bool FIREBASE_isTokenExpired() { return Firebase.isTokenExpired(); }

void FIREBASE_printTokenExpiredMessage() {
  Serial.println("Firebase Token Expired");
  Serial.println("Restarting system to get new token...");
}

void FIREBASE_printOfflineMessage() {
  Serial.println("GLOBAL: Can't send data to Firebase.");
  Serial.println("GLOBAL: Reason => Offline Mode.");
}

void FIREBASE_turnLamp(String lampId, bool condition) {
  Serial.println("Turning " + lampId + " on/off");
  if (Firebase.RTDB.setBool(&fbdo, lampLoc + "/" + lampId + "/condition",
                            condition)) {
    Serial.println("Success");
  } else {
    Serial.println("fail");
    Serial.println(fbdo.httpCode());
    Serial.println(fbdo.errorReason());
  }
}

void FIREBASE_turnPlug(String plugId, bool condition) {
  for (int i = 1; i <= SOCKET_COUNT; i++) {
    Firebase.RTDB.setBool(&fbdo,
                          plugLoc + "/" + plugId + "/sockets/" + "socket-" +
                              (String)i + "/condition",
                          condition);
    delay(500);
  }
}
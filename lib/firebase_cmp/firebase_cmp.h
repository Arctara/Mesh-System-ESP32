#ifndef FIREBASE_CMP
#define FIREBASE_CMP

#include <Firebase_ESP_Client.h>
#include <List.hpp>

#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include "global_cmp.h"
#include "schedule_cmp.h"

//* Firebase Data Configuration
extern FirebaseData stream;
extern FirebaseData fbdo;

//* Firebase User Configuration
extern FirebaseAuth auth;
extern FirebaseConfig config;

//* Firebase Variable
extern unsigned long sendDataPrevMillis;
extern int count;
extern volatile boolean firebaseDataChanged;

//* Struct Used for Firebase Data
struct streamData {
  String streamPath;
  String dataPath;
  String dataType;
  String eventType;
  String data;
};

//* Initialize Struct Data
extern streamData receivedDataFirebase;

void FIREBASE_init();
void FIREBASE_streamCallBack();
void FIREBASE_streamTimeoutCallback();
void FIREBASE_initStream();
void FIREBASE_initDeviceData();
void FIREBASE_getSchedule();

bool FIREBASE_isTokenExpired();

void FIREBASE_printTokenExpiredMessage();
void FIREBASE_printOfflineMessage();

void FIREBASE_turnLamp(String lampId, bool condition);
void FIREBASE_turnPlug(String plugId, bool condition);

#endif
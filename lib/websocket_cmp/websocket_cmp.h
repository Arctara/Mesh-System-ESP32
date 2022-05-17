#ifndef WEBSOCKET_CMP
#define WEBSOCKET_CMP

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#include <List.hpp>

#include "firebase_cmp.h"
#include "global_cmp.h"
#include "schedule_cmp.h"
#include "spiffs_cmp.h"
#include "time_cmp.h"
#include "wifi_cmp.h"

extern String target;
extern bool conditionToSendWebsocket;

void WS_init();
void WS_loop();
void WS_sendMessage();
void WS_onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *data, size_t len);
void WS_handleMessage(void *arg, uint8_t *data, size_t len);
void WS_turn(String device, bool condition);

#endif
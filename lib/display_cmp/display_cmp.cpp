#include "display_cmp.h"

SSD1306 display(0x3C, 21, 22);
QRcode qrcode(&display);

void DISPLAY_init() {
  display.init();
  display.write("Hello!");
  display.display();

  qrcode.init();
}

void DISPLAY_printQRCode() {
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
}
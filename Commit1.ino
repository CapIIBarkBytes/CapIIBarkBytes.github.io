#include "WiFi.h"



const char* ssid = "BarkBytes-Access-Point";
const char* password = "cap2barkbytes";


void setup() {
  WiFi.softAP(ssid,password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP32 IP: ");
  Serial.println(IP);
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

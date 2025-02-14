#include "SD.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

const char* ssid = "BarkBytes-Access-Point";
const char* password = "cap2barkbytes";

AsyncWebServer server(80);


void setup() {
  Serial.begin(9600);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  WiFi.softAP(ssid,password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP32 IP: ");
  Serial.println(IP);
  // handle http requests with server.on()

  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

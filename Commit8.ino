#include "SD.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <sqlite3.h>
#include <time.h>

const char* ssid = "NETGEAR30";
const char* wifi_password = "perfectskates004";

AsyncWebServer server(8080);

sqlite3 *feeder;
sqlite3_stmt *res;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

int openDb(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
}

void setup() {
  pinMode(21,OUTPUT);
  pinMode(22,OUTPUT);
  pinMode(17,OUTPUT);
  digitalWrite(17,HIGH);
  digitalWrite(21,LOW);
  digitalWrite(22,LOW);
  Serial.begin(9600);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  int rc;
  sqlite3_initialize();
  if (openDb("/sd/feeder.db", &feeder))
     return;
  sqlite3_close(feeder);
  WiFi.begin(ssid, wifi_password);
  delay(2000);
  Serial.println((WiFi.localIP()));
  // handle http requests with server.on()
  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request){
    digitalWrite(21,HIGH);
    digitalWrite(22,LOW);
    delay(1500);
    digitalWrite(21,LOW);
    digitalWrite(22,LOW);
    request->send(200,"text/plain","Hello World");
  });
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

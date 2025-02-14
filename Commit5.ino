#include "SD.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "sqlite3.h"

const char* ssid = "router_ssid";
const char* wifi_password = "wifi-password";

AsyncWebServer server(80);

sqlite3 *feeder;
sqlite3_stmt *res;

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

String get_wifi_status(int status){
    switch(status){
        case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
        case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
        case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
        case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
        case WL_CONNECTED:
        return "WL_CONNECTED";
        case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    }
}

void setup() {
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
  int status = WL_IDLE_STATUS;
  Serial.println("\nConnecting");
  Serial.println(get_wifi_status(status));
  WiFi.begin(ssid, wifi_password);
  while(status != WL_CONNECTED){
      delay(500);
      status = WiFi.status();
      Serial.println(get_wifi_status(status));
  }
  // handle http requests with server.on()

  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

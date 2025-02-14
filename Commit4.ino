#include "SD.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "sqlite3.h"

const char* ssid = "BarkBytes-Access-Point";
const char* password = "cap2barkbytes";

AsyncWebServer server(80);

sqlite3 *feeder;
sqlite3_stmt *res;
char *tail;

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
  Serial.begin(9600);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  int rc;
  sqlite3_initialize();
  if (openDb("/sd/feeder.db", &feeder))
     return;
  rc = db_exec(feeder,"CREATE TABLE IF NOT EXISTS schedules (S_ID INTEGER PRIMARY KEY ASC,A_ID INTEGER,H INTEGER,M INTEGER, D INTEGER,status,dog,food_amt)");
  if (rc != SQLITE_OK) {
     sqlite3_close(feeder);
     return;
   }
  rc = db_exec(feeder,"CREATE TABLE IF NOT EXISTS accounts (A_ID INTEGER PRIMARY KEY ASC,u_hash,p_hash)");
  if (rc != SQLITE_OK) {
     sqlite3_close(feeder);
     return;
   }
  rc = db_exec(feeder,"CREATE TABLE IF NOT EXISTS activity (Sess_ID INTEGER PRIMARY KEY ASC, local_time,food_amt)");
  if (rc != SQLITE_OK) {
     sqlite3_close(feeder);
     return;
   }
  sqlite3_close(feeder);
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

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
const char* tail;

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

char *zErrMsg = "";
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
}

bool get_credentials(int a_id,String u_hash,String p_hash){
  String sql = "SELECT A_ID,u_hash,p_hash FROM accounts WHERE A_ID = ? AND u_hash = ? AND p_hash = ?;";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
     sqlite3_close(feeder);
     return 0;
   }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_text(res,2,u_hash.c_str(),u_hash.length(),SQLITE_STATIC);
  sqlite3_bind_text(res,3,p_hash.c_str(),p_hash.length(),SQLITE_STATIC);
  bool row = 0;
  while(sqlite3_step(res) == SQLITE_ROW){
    row = 1;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);
  sqlite3_finalize(res);
  Serial.println(row);
  return row;
};

void insert_credentials(int a_id,String u_hash, String p_hash){
  String sql = "INSERT INTO accounts VALUES (?,?,?);";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
    Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_text(res,2,u_hash.c_str(),u_hash.length(),SQLITE_STATIC);
  sqlite3_bind_text(res,3,p_hash.c_str(),p_hash.length(),SQLITE_STATIC);
  if(sqlite3_step(res) != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);
  sqlite3_finalize(res);
  return;
};

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
  WiFi.mode(WIFI_AP);
  WiFi.softAP("barkbytes", NULL);
  WiFi.begin();
  delay(2000);
  Serial.println((WiFi.softAPIP()));
  // handle http requests with server.on()
  server.on("/login",HTTP_GET, [] (AsyncWebServerRequest *request){
    if (request -> hasParam("uh") && request -> hasParam("ph")){
      int a_id = atoi(request -> getParam("a") -> value().c_str());
      String u_hash = request -> getParam("uh") -> value();
      String p_hash = request -> getParam("ph") -> value();;
      if (get_credentials(a_id, u_hash, p_hash)){
        Serial.println(u_hash);
        Serial.println(p_hash);
        request->send(200,"application/json","{\"login_flag\":1}");
      }
      else{
        request->send(200,"application/json","{\"login_flag\":0}");
      }
    }
    else{
      request->send(200,"application/json","{\"message\":\"Please input username and password\"}");
    }
  });
  server.on("/login",HTTP_POST, [] (AsyncWebServerRequest *request){
    if (request -> hasParam("uh") && request -> hasParam("ph")){
      int a_id = atoi(request -> getParam("a") -> value().c_str());
      String u_hash = request -> getParam("uh") -> value();
      String p_hash = request -> getParam("ph") -> value();
      if(get_credentials(a_id, u_hash, p_hash)){
         request->send(200,"application/json","{\"message\":\"account already exists\"}");
      }
      else{
        insert_credentials(a_id,u_hash,p_hash);
        Serial.println(a_id);
        Serial.println(u_hash);
        Serial.println(p_hash);
        request->send(201,"application/json","{\"message\":\"account created\"}");
      }
    }
    else{
      request->send(200,"application/json","{\"message\":\"Please input username and password\"}");
    }
  });
  server.on("/feed", HTTP_GET, [] (AsyncWebServerRequest *request){
    String food_level;
    if (request -> hasParam("amount")){
      food_level = request -> getParam("amount") -> value();
      if (food_level == "small"){
        digitalWrite(21,HIGH);
        digitalWrite(22,LOW);
        delay(2000);
      }
      else if (food_level == "medium"){
        digitalWrite(21,HIGH);
        digitalWrite(22,LOW);
        delay(3000);
      }
      else if (food_level == "large"){
        digitalWrite(21,HIGH);
        digitalWrite(22,LOW);
        delay(4000);
      }
    }
    else{
      food_level = "N/A";
    }
    digitalWrite(21,LOW);
    digitalWrite(22,LOW);
    request->send(200,"text/plain",food_level);
  });
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

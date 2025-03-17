#include "SD.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <sqlite3.h>
#include <time.h>

struct tm *timeinfo;
String SCHEDULE_POLL_SQL = "SELECT status,food_amt FROM scheduels WHERE (H = ? AND M = ? AND D = ?) OR (H = ? AND M = ? AND D = 7)";

const char* ssid = "GSU";
const char* wifi_password = "";

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

bool check_credentials(int a_id,const char *u_hash,const char *p_hash){
  String sql = "SELECT A_ID,u_hash,p_hash FROM accounts WHERE A_ID = ? AND u_hash = ? AND p_hash = ?;";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
     sqlite3_close(feeder);
     return false;
   }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_text(res,2,u_hash,strlen(u_hash),SQLITE_STATIC);
  sqlite3_bind_text(res,3,p_hash,strlen(p_hash),SQLITE_STATIC);
  bool row = false;
  while(sqlite3_step(res) == SQLITE_ROW){
    row = true;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);
  sqlite3_finalize(res);
  Serial.println(row);
  return row;
};

void insert_account(int a_id,const char*  u_hash, const char*  p_hash){
  String sql = "INSERT INTO accounts (A_ID,u_hash,p_hash) VALUES (?,?,?)";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
    Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_text(res,2,u_hash,strlen(u_hash),SQLITE_STATIC);
  sqlite3_bind_text(res,3,p_hash,strlen(p_hash),SQLITE_STATIC);
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

void delete_account(int a_id){
  String sql = "DELETE FROM accounts WHERE A_ID = ?;";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
    Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_bind_int(res,1,a_id);
  if(sqlite3_step(res) != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);

  sql = "DELETE FROM schedules WHERE A_ID = ?;";
  rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
    Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_bind_int(res,1,a_id);
  if(sqlite3_step(res) != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(feeder));
    sqlite3_close(feeder);
    return;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);
  sqlite3_finalize(res);
  sqlite3_exec(feeder,"VACUUM",0,0,0);
  return;
};

bool check_time_slot(int h,int m,int d){
  bool time_slot = false;
  String sql = "SELECT * FROM schedules WHERE H = ? AND M = ? AND D = ?";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
     sqlite3_close(feeder);
     return false;
   }
  sqlite3_bind_int(res,1,h);
  sqlite3_bind_int(res,2,m);
  sqlite3_bind_int(res,3,d);
  while(sqlite3_step(res) == SQLITE_ROW){
    time_slot = true;
  }
  sqlite3_clear_bindings(res);
  sqlite3_reset(res);
  sqlite3_finalize(res);
  Serial.println(time_slot);
  return time_slot;
};

void insert_schedule(int a_id,int h,int m,int d,const char *status, const char *dog, const char *food_amt){
  String sql = "INSERT INTO schedules (A_ID,H,M,D,status,dog,food_amt) VALUES (?,?,?,?,?,?,?)";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
     sqlite3_close(feeder);
     return;
   }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_int(res,2,h);
  sqlite3_bind_int(res,3,m);
  sqlite3_bind_int(res,4,d);
  sqlite3_bind_text(res,5,status,strlen(status),SQLITE_STATIC);
  sqlite3_bind_text(res,6,dog,strlen(dog),SQLITE_STATIC);
  sqlite3_bind_text(res,7,food_amt,strlen(food_amt),SQLITE_STATIC);
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

void edit_schedule(int a_id,int h, int m, int d, const char *status, const char *dog, const char *food_amt){
  String sql = "INSERT INTO schedules (A_ID,H,M,D,status,dog,food_amt) VALUES (?,?,?,?,?,?,?)\ 
                  ON CONFLICT (A_ID) DO UPDATE SET A_ID = ?\
                  ON CONFLICT (H) DO UPDATE SET H = ?\
                  ON CONFLICT (M) DO UPDATE SET M = ?\
                  ON CONFLICT (D) DO UPDATE SET D = ?\
                  ON CONFLICT (status) DO UPDATE SET status = ?\
                  ON CONFLICT (dog) DO UPDATE SET dog = ?\
                  ON CONFLICT (food_amt) DO UPDATE SET food_amt = ?;";
  int rc = sqlite3_prepare_v2(feeder, sql.c_str(),sql.length()+1,&res,&tail);
  if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
     sqlite3_close(feeder);
     return;
  }
  sqlite3_bind_int(res,1,a_id);
  sqlite3_bind_int(res,2,h);
  sqlite3_bind_int(res,3,m);
  sqlite3_bind_int(res,4,d);
  sqlite3_bind_text(res,5,status, strlen(status),SQLITE_STATIC);
  sqlite3_bind_text(res,6,dog,strlen(dog),SQLITE_STATIC);
  sqlite3_bind_text(res,7,food_amt,strlen(food_amt),SQLITE_STATIC);
  sqlite3_bind_int(res,8,a_id);
  sqlite3_bind_int(res,9,h);
  sqlite3_bind_int(res,10,m);
  sqlite3_bind_int(res,11,d);
  sqlite3_bind_text(res,12,status,strlen(status),SQLITE_STATIC);
  sqlite3_bind_text(res,13,dog,strlen(dog),SQLITE_STATIC);
  sqlite3_bind_text(res,14,food_amt,strlen(food_amt),SQLITE_STATIC);
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
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
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
  sqlite3_exec(feeder,"DROP activity;",0,0,0);
  WiFi.begin(ssid, wifi_password);
  delay(2000);
  Serial.println((WiFi.localIP()));
  // handle http requests with server.on()
  server.on("/login",HTTP_GET, [] (AsyncWebServerRequest *request){
    if (request -> hasParam("uh") && request -> hasParam("ph")){
      int a_id = atoi(request -> getParam("a") -> value().c_str());
      String u_hash = request -> getParam("uh") -> value();
      String p_hash = request -> getParam("ph") -> value();;
      if (check_credentials(a_id, u_hash.c_str(), p_hash.c_str())){
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
      if(check_credentials(a_id, u_hash.c_str(), p_hash.c_str())){
         request->send(200,"application/json","{\"message\":\"account already exists\"}");
      }
      else{
        insert_account(a_id,u_hash.c_str(),p_hash.c_str());
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
  server.on("/login",HTTP_DELETE, [] (AsyncWebServerRequest *request){
    if (request -> hasParam("a")){
      int a_id = atoi(request -> getParam("a") -> value().c_str());
      String u_hash = request -> getParam("uh") -> value();
      String p_hash = request -> getParam("ph") -> value();
      if(check_credentials(a_id, u_hash.c_str(), p_hash.c_str())){
        delete_account(a_id);
        request->send(200,"application/json","{\"message\":\"account deleted\"}");
      }
      else{
        request->send(201,"application/json","{\"message\":\"account does not exist\"}");
      }
    }
    else{
      request->send(200,"application/json","{\"message\":\"Error: No account ID given\"}");
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

  server.on("/schedules", HTTP_GET,[] (AsyncWebServerRequest *request){
    if (request -> hasParam("a")){
      int a_id = atoi(request -> getParam("a") -> value().c_str());
      String resp = "{\"schdules\":[";
      String sql = "SELECT H,M,D,status,dog,food_amt FROM schedules WHERE A_ID = ?";
      sqlite3_bind_int(res,1,a_id);
      int rc = sqlite3_prepare_v2(feeder,sql.c_str(),sql.length()+1,&res,&tail);
      if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
        sqlite3_close(feeder);
        return;
      }
      while(sqlite3_step(res) != SQLITE_DONE){
        resp += "{\"hour\":";
        resp += sqlite3_column_int(res,0);
        resp +=",\"minute\":";
        resp += sqlite3_column_int(res,1);
        resp +=",\"day\":";
        resp  += sqlite3_column_int(res,2);
        resp += ",\"status\":";
        resp += (const char *)sqlite3_column_text(res,3);
        resp += ",\"dog\":";
        resp += (const char *)sqlite3_column_text(res,4);
        resp += ",\"food_amount\":";
        resp += (const char *)sqlite3_column_text(res,5);
        resp += "},";
      }
      resp[resp.length()-1] = ']';
      resp += "}";
      Serial.println(resp);
      request->send(200,"application/JSON",resp);
    }
    else{
      request->send(200,"application/json","{\"message\":\"Error: No account ID given\"}");
    }
  });

  server.on("/schedules", HTTP_POST,[] (AsyncWebServerRequest *request){
    if(request -> hasParam("a") && request -> hasParam("h") && request -> hasParam("m") && 
       request -> hasParam("da") && request -> hasParam("s") && request -> hasParam("do") && 
       request -> hasParam("f")){
          int a_id = atoi(request -> getParam("a") -> value().c_str());
          int h = atoi(request -> getParam("h") -> value().c_str()); 
          int m = atoi(request -> getParam("m") -> value().c_str()); 
          int d = atoi(request -> getParam("da") -> value().c_str()); 
          const char *status = request -> getParam("s") -> value().c_str();
          const char *dog = request -> getParam("do") -> value().c_str();
          const char *food_amt = request -> getParam("fo") -> value().c_str();
          if(check_time_slot(h,m,d)){
            request->send(200,"application/json","{\"message\":\"Schedule already exists\"}");
          }
          else{
            insert_schedule(a_id,h,m,d,status,dog,food_amt);
            request->send(201,"application/json","{\"message\":\"Schedule created\"}");
          }
    }
    else{
      request->send(200,"application/json","{\"message\":\"Please make sure all fields are filled out\"}");
    }
  });
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!getLocalTime(timeinfo)){
    Serial.println("Failed to obtain time");
    if(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, wifi_password);
    }
    delay(5000);
  }
  else{
    int hour = timeinfo -> tm_hour;
    int minute = timeinfo -> tm_min;
    int day = timeinfo -> tm_wday;
    const char *food_level;
    int rc = sqlite3_prepare_v2(feeder, SCHEDULE_POLL_SQL.c_str(),SCHEDULE_POLL_SQL.length()+1,&res,&tail);
    if (rc != SQLITE_OK) {
      Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(feeder));
      delay(5000);
    }
    else{
      sqlite3_bind_int(res,1,hour);
      sqlite3_bind_int(res,2,minute);
      sqlite3_bind_int(res,3,day);
      if(sqlite3_step(res) == SQLITE_ROW){
        if((const char*)sqlite3_column_text(res,0) == "active"){
          food_level = (const char*)sqlite3_column_text(res,1);
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
      }
      sqlite3_finalize(res);
    }
  delay(60000);
  }
}


#include <Arduino.h>
#include <list>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>
#include <Arduino_JSON.h>

const char* ssid = "OPPO Reno5";
const char* password = "v3qu6m3u";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Date & TIme
WiFiUDP ntpUDP;
NTPClient time_client(ntpUDP, "pool.ntp.org", 25200, 60000);
std::list<String*> times;
String date = "";
String currentTime = "";

// Sampling
#define SAMPLING_PERIODE 1000
unsigned long last_time_millis = 0;

void setup(){
  Serial.begin(115200);

  beginWiFi();
  beginSPIFFS();

  initWebSocket();

  // Start server on route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  // Start server
  server.begin();

  time_client.begin();  
  setenv("TZ", "Asia/Jakarta", 1);
  tzset();
}

void loop(){
  ws.cleanupClients();
  if(millis() - last_time_millis > SAMPLING_PERIODE){
      readAndSaveDateTime();
      // readAndSavePotentiometer();
      // readAndSaveHumidity();
      // readAndSaveTemperature();
      // printLog();
      notifyClients();
      last_time_millis = millis();
    }
}

void readAndSaveDateTime(){
  time_client.update();
  time_t raw_time = time_client.getEpochTime();
  struct tm *time_info;
  time_info = localtime(&raw_time);
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%A, %e %B %Y %H:%M:%S", time_info);
  
  String date_and_time = String(buffer);
  date = date_and_time.substring(0,date_and_time.length() - 9);
  String time = date_and_time.substring(date_and_time.length() - 8, date_and_time.length());
  currentTime = time;
  times.push_back(new String(time));
}

void beginSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void beginWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "restart") == 0) {
      // To Do
      notifyClients();
    }
  }
}

void notifyClients(){
  JSONVar json;
  json["date"] = date;
  json["currentTime"] = currentTime;

  JSONVar timesJsonArray;
  int i = 0;
  for (auto it = times.begin(); it != times.end(); ++it) {
    timesJsonArray[i] = *(*it);
    i++;
  }
  json["times"] = timesJsonArray;

  String jsonString = JSON.stringify(json);

  ws.textAll(jsonString);
}



#include <Arduino.h>
#include <list>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>
#include "DHT.h"
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
#define ARRAY_MAX_VALUES 10
String status = "stop";

//Button & Tilting
#define LEFT_BUTTON 21
#define RIGHT_BUTTON 19
// unsigned long lastTime = 0;
unsigned long debounce_delay = 300;
int steer = 0;
std::list<int> steers;

// Potentiometer
#define POTENTIOMETER_PIN 34
#define POTENTIOMETER_MIN 0
#define POTENTIOMETER_MAX 4095
#define SPEED_MIN 0
#define SPEED_MAX 10 // 10 m/s
std::list<float> speeds;

// DHT
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

std::list<float> temperatures;

std::list<float> humidities;

// Time, Distance, Acceleration
int runningTime = 0;
float distance = 0.0;
float acceleration = 0.0;

void setup(){
  Serial.begin(115200);

  beginWiFi();
  beginSPIFFS();

  initWebSocket();
  initButtons();

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

  dht.begin();
}

void loop(){
  ws.cleanupClients();
  if(millis() - last_time_millis > SAMPLING_PERIODE){
      readAndSaveDateTime();
      if(status == "start"){
        runningTime++;
        readAndSaveSpeedDistanceAcceleration();
        readAndSaveTemperature();
        readAndSaveHumidity();
        readAndSaveSteering();
      }
      // printLog();
      notifyClients();
      last_time_millis = millis();
    }
}

void initButtons() {
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
}

void readAndSaveSteering() {
  // if ((millis() - last_time_millis) > debounce_delay) {
    if (digitalRead(LEFT_BUTTON) == LOW) steer--;
    if (digitalRead(RIGHT_BUTTON) == LOW) steer++;
    
  //   last_time_millis = millis();
  // }
  steers.push_back(steer);
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

void readAndSaveSpeedDistanceAcceleration(){
  int value = analogRead(POTENTIOMETER_PIN);
  float newSpeed = mapPotentiometerValueToSpeed(value);
  float lastSpeed = speeds.back();
  acceleration = (newSpeed - lastSpeed) / 1;
  distance += (newSpeed * 1);
  speeds.push_back(newSpeed);
}

float mapPotentiometerValue(float value, float out_min, float out_max){
  return (value - POTENTIOMETER_MIN) * (out_max - out_min) / (POTENTIOMETER_MAX - POTENTIOMETER_MIN) + out_min;
}

float mapPotentiometerValueToSpeed(float value){
  return float(mapPotentiometerValue(value, SPEED_MIN, SPEED_MAX));
}

void readAndSaveTemperature(){
  float value = dht.readTemperature();
  temperatures.push_back(value);
}

void readAndSaveHumidity(){
  float value = dht.readHumidity();
  humidities.push_back(value);
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
    if (strcmp((char*)data, "start") == 0) {
      Serial.println("Start");
      status = "start";
      // notifyClients();
    }
    if (strcmp((char*)data, "pause") == 0) {
      Serial.println("Pause");
      status = "pause";
      // notifyClients();
    }
    if (strcmp((char*)data, "stop") == 0) {
      Serial.println("Stop");
      status = "stop";
      // notifyClients();
    }
  }
}

void notifyClients(){
  JSONVar json;
  json["status"] = status;
  json["date"] = date;
  json["currentTime"] = currentTime;
  json["time"] = runningTime;
  json["distance"] = distance;
  json["acceleration"] = acceleration;
  json["label"] = *times.back();
  json["speed"] = speeds.back();
  json["temperature"] = temperatures.back();
  json["humidity"] = humidities.back();
  json["tilt"] = steers.back();

  // JSONVar timesJsonArray;
  // int count = 0;
  // for (auto it = times.rbegin(); it != times.rend() && count < ARRAY_MAX_VALUES; ++it) {
  //     timesJsonArray[count] = *(*it);
  //     count++;
  // }
  // json["times"] = timesJsonArray;

  // JSONVar rawPotentiometerJsonArray;
  // count = 0;
  // for (auto it = potentiometerRawValues.rbegin(); it != potentiometerRawValues.rend() && count < ARRAY_MAX_VALUES; ++it) {
  //   rawPotentiometerJsonArray[count] = *it;
  //   count++;
  // }
  // json["potentiometerRawValues"] = rawPotentiometerJsonArray;

  // JSONVar anglePotentiometerJsonArray;
  // count = 0;
  // for (auto it = potentiometerAngleValues.rbegin(); it != potentiometerAngleValues.rend() && count < ARRAY_MAX_VALUES; ++it) {
  //   anglePotentiometerJsonArray[count] = *it;
  //   count++;
  // }
  // json["potentiometerAngleValues"] = anglePotentiometerJsonArray;

  String jsonString = JSON.stringify(json);

  ws.textAll(jsonString);
}

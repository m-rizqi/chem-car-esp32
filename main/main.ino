#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>
#include <cppQueue.h>
#include "DHT.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "OPPO Reno5";
const char* password = "v3qu6m3u";
WiFiUDP ntpUDP;
NTPClient time_client(ntpUDP, "pool.ntp.org", 25200, 60000);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define	QUEUE_IMPLEMENTATION	FIFO
#define ARRAY_MAX_VALUES 5
size_t int_size = sizeof(int);
size_t float_size = sizeof(float);

#define POTENTIOMETER_PIN 34
#define POTENTIOMETER_MIN 0
#define POTENTIOMETER_MAX 4095
cppQueue potentiometer_queue(int_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

cppQueue humidity_queue(float_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

cppQueue temperature_queue(float_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

#define ANGLE_MIN 0
#define ANGLE_MAX 360

#define SAMPLING_PERIODE 1000
unsigned long last_time_millis = 0;
struct Time {
  int hour;
  int minute;
  int second;
  String getFormattedTime(){
    String hour_string = (this->hour < 10 ? "0" : "") + String(this->hour, DEC);
    String minute_string = (this->minute < 10 ? "0" : "") + String(this->minute, DEC); 
    String second_string = (this->second < 10 ? "0" : "") + String(this->second, DEC); 
    String time_string = hour_string + ":" + minute_string + ":" + second_string; 
    return time_string;
  }
};
size_t time_size = sizeof(Time);
cppQueue time_queue(time_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Sensor Monitor</title>
    <link
      href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css"
      rel="stylesheet"
      integrity="sha384-9ndCyUaIbzAi2FUVXJi0CjmCapSmO7SnpJef0486qhLnuZ2cdeRhO02iuK6FUUVM"
      crossorigin="anonymous"
    />
    <style>
      body {
        background-color: #f5f7f9;
      }

      .header {
        overflow: hidden;
        background-color: #fff;
        padding: 15px 10px;
        text-align: center;
      }

      .header h4 {
        color: black;
        font-weight: 600;
      }
    </style>
  </head>
  <body>
    <div class="header">
      <h4 href="" class="heading">Sensor Monitor</h4>
    </div>

    <div class="p-3">
      <div class="bg-white rounded-2 p-3">
        <div class="row">
          <div class="col-md">
            <div class="fw-medium fs-6" style="color: #727272">
              Potentiometer
            </div>
            <div class="fw-bold fs-1 mt-2">75°</div>
            <div style="margin-top: -8px">Degree</div>
          </div>
          <div class="col-md">
            <canvas id="potentiometer_chart"></canvas>
          </div>
        </div>
      </div>

      <div class="bg-white rounded-2 p-3 mt-3">
        <div class="row">
          <div class="col-md">
            <div class="fw-medium fs-6" style="color: #727272">Temperature</div>
            <div class="fw-bold fs-1 mt-2">24°</div>
            <div style="margin-top: -8px">Celcius</div>
          </div>
          <div class="col-md">
            <canvas id="temperature_chart"></canvas>
          </div>
        </div>
      </div>

      <div class="bg-white rounded-2 p-3 mt-3">
        <div class="row">
          <div class="col-md">
            <div class="fw-medium fs-6" style="color: #727272">Humidity</div>
            <div class="fw-bold fs-1 mt-2">92%</div>
          </div>
          <div class="col-md">
            <canvas id="humidity_chart"></canvas>
          </div>
        </div>
      </div>
    </div>

    <script>
      const potentiometer_ctx = document.getElementById("potentiometer_chart");
      var potentiometer_chart = new Chart(potentiometer_ctx, {
        type: "line",
        data: {
          labels: [
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
          ],
          datasets: [
            {
              data: [86, 114, 106, 106, 107, 111, 133],
              label: "Degree of Angle",
              borderColor: "rgb(62,149,205)",
              backgroundColor: "rgb(62,149,205,0.1)",
            },
          ],
        },
      });

      const temperature_ctx = document.getElementById("temperature_chart");
      var temperature_chart = new Chart(temperature_ctx, {
        type: "line",
        data: {
          labels: [
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
          ],
          datasets: [
            {
              data: [70, 90, 44, 60, 83, 90, 100],
              label: "Temperature",
              borderColor: "rgb(60,186,159)",
              backgroundColor: "rgb(60,186,159,0.1)",
            },
          ],
        },
      });

      const humidity_ctx = document.getElementById("humidity_chart");
      var humidity_chart = new Chart(humidity_ctx, {
        type: "line",
        data: {
          labels: [
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
          ],
          datasets: [
            {
              data: [10, 21, 60, 44, 17, 21, 17],
              label: "Humidity",
              borderColor: "rgb(255,165,0)",
              backgroundColor: "rgb(255,165,0,0.1)",
            },
          ],
        },
      });

      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      window.addEventListener("load", onLoad);
      function initWebSocket() {
        console.log("Trying to open a WebSocket connection...");
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onmessage = onMessage;
      }
      function onOpen(event) {
        console.log("Connection opened");
      }
      function onClose(event) {
        console.log("Connection closed");
        setTimeout(initWebSocket, 2000);
      }
      function onMessage(event) {
        console.log(event.data);
      }
      function onLoad(event) {
        initWebSocket();
      }
    </script>
  </body>
</html>
)rawliteral";

String processor(const String& var){
  return String();
}

void setup() {
  Serial.begin(115200);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  time_client.begin();  
  setenv("TZ", "Asia/Jakarta", 1);
  tzset();

  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();

  dht.begin();
}

void loop() {
  ws.cleanupClients();
    if(millis() - last_time_millis > SAMPLING_PERIODE){
      readAndSaveTime();
      readAndSavePotentiometer();
      readAndSaveHumidity();
      readAndSaveTemperature();
      printLog();
      notifyClients();
      last_time_millis = millis();
    }
}

void readAndSaveTime(){
  time_client.update();
  time_t raw_time = time_client.getEpochTime();
  struct tm *time_info;
  time_info = localtime(&raw_time);
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%e %B %Y %H:%M:%S", time_info);
  String date_and_time = String(buffer);
  String date_string = date_and_time.substring(0,date_and_time.length() - 9);
  String time_string = date_and_time.substring(date_and_time.length() - 8, date_and_time.length());
  int hour = atoi(time_string.substring(0,2).c_str());
  int minute = atoi(time_string.substring(3,5).c_str());
  int second = atoi(time_string.substring(6,8).c_str());
  Time t = Time{hour, minute, second};
  if(time_queue.isFull()){
    Time temp;
    time_queue.pop(&temp);
  }
  time_queue.push(&t);
}

void readAndSavePotentiometer(){
  int value = analogRead(POTENTIOMETER_PIN);
  if(potentiometer_queue.isFull()){
    int temp;
    potentiometer_queue.pop(&temp);
  }
  potentiometer_queue.push(&value);
}

void readAndSaveHumidity(){
  float value = dht.readHumidity();
  if(humidity_queue.isFull()){
    float temp;
    humidity_queue.pop(&temp);
  }
  humidity_queue.push(&value);
}

void readAndSaveTemperature(){
  float value = dht.readTemperature();
  if(temperature_queue.isFull()){
    float temp;
    temperature_queue.pop(&temp);
  }
  temperature_queue.push(&value);
}

void printLog(){
  Time time_temp;
  Serial.print("Time: [");
  for(int i = 0; i < time_queue.getCount(); i++){
    time_queue.peekIdx(&time_temp, i);
    Serial.print(time_temp.getFormattedTime());
    Serial.print(",");
  }
  Serial.println("]");
  
  int int_temp;
  Serial.print("Potentiometer: [");
  for(int i = 0; i < potentiometer_queue.getCount(); i++){
    potentiometer_queue.peekIdx(&int_temp, i);
    Serial.print(int_temp);
    Serial.print(",");
  }
  Serial.println("]");

  float f_temp;
  Serial.print("Humidity: [");
  for(int i = 0; i < humidity_queue.getCount(); i++){
    humidity_queue.peekIdx(&f_temp, i);
    Serial.print(f_temp);
    Serial.print(",");
  }
  Serial.println("]");

  Serial.print("Temperature: [");
  for(int i = 0; i < temperature_queue.getCount(); i++){
    temperature_queue.peekIdx(&f_temp, i);
    Serial.print(f_temp);
    Serial.print(",");
  }
  Serial.println("]");
  Serial.println("");
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
    // if (strcmp((char*)data, "toggle") == 0) {
    //   ledState = !ledState;
    //   notifyClients();
    // }
  }
}

void notifyClients() {
  String json = "{";

  json += "\"times\":[";
  Time time_temp;
  for(int i = 0; i < time_queue.getCount(); i++){
    time_queue.peekIdx(&time_temp, i);
    json += "\"";
    String formatted_time = time_temp.getFormattedTime();
    json += formatted_time;
    json += "\"";
    if(i < time_queue.getCount() - 1){
      json += ",";
    }    
  }
  json += "]";

  json += ",\"potentiometer\":[";
  int int_temp;
  for(int i = 0; i < potentiometer_queue.getCount(); i++){
    potentiometer_queue.peekIdx(&int_temp, i);
    json += int_temp;
    if(i < potentiometer_queue.getCount() - 1){
      json += ",";
    }  
  }
  json += "]";

  json += ",\"humidity\":[";
  float float_temp;
  for(int i = 0; i < humidity_queue.getCount(); i++){
    humidity_queue.peekIdx(&float_temp, i);
    json += float_temp;
    if(i < humidity_queue.getCount() - 1){
      json += ",";
    }  
  }
  json += "]";

  json += ",\"temperature\":[";
  for(int i = 0; i < temperature_queue.getCount(); i++){
    temperature_queue.peekIdx(&float_temp, i);
    json += float_temp;
    if(i < temperature_queue.getCount() - 1){
      json += ",";
    }  
  }
  json += "]";

  json += "}";
  ws.textAll(json);
}

float mapPotentiometerValue(float value, float out_min, float out_max){
  return (value - POTENTIOMETER_MIN) * (out_max - out_min) / (POTENTIOMETER_MAX - POTENTIOMETER_MIN) + out_min;
}

float mapPotentiometerValueToAngle(float value){
  return mapPotentiometerValue(value, ANGLE_MIN, ANGLE_MAX);
}

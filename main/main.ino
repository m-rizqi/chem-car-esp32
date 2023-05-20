#include <cppQueue.h>
#define	QUEUE_IMPLEMENTATION	FIFO
#define ARRAY_MAX_VALUES 5
size_t int_size = sizeof(int);
size_t float_size = sizeof(float);

#define POTENTIOMETER_PIN 15
#define POTENTIOMETER_MIN 0
#define POTENTIOMETER_MAX 4095
cppQueue potentiometer_queue(int_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

#include "DHT.h"
#define DHT_PIN 4
// #define DHT_TYPE DHT11
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

cppQueue humidity_queue(float_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

cppQueue temperature_queue(float_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

#define ANGLE_MIN 0
#define ANGLE_MAX 360

#include <WiFi.h>
#include <WiFiUdp.h>

#include <NTPClient.h>
#include <time.h>
#define SAMPLING_PERIODE 1000
unsigned long last_time_millis = 0;
struct Time {
  int hour;
  int minute;
  int second;
  String getFormattedTime(){
    String hour_string = (this.hour < 10 ? "0" : "") + String(this.hour, DEC);
    String minute_string = (this.minute < 10 ? "0" : "") + String(this.minute, DEC); 
    String second_string = (this.second < 10 ? "0" : "") + String(this.second, DEC); 
    String time_string = hour_string + ":" + minute_string; 
  }
} t_time;
size_t time_size = sizeoft(t_time);

cppQueue time_queue(time_size, ARRAY_MAX_VALUES, QUEUE_IMPLEMENTATION);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
    if(millis() - last_time_millis > SAMPLING_PERIODE){
      readAndSaveTime();
      readAndSavePotentiometer();
      readAndSaveHumidity();
      readAndSaveTemperature();
      printLog();
      last_time_millis = millis();
    }
}

void readAndSaveTime(){

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
  int temp;

  Serial.print("Potentiometer: [");
  for(int i = 0; i < potentiometer_queue.getCount(); i++){
    potentiometer_queue.peekIdx(&temp, i);
    Serial.print(temp);
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

float mapPotentiometerValue(float value, float out_min, float out_max){
  return (value - POTENTIOMETER_MIN) * (out_max - out_min) / (POTENTIOMETER_MAX - POTENTIOMETER_MIN) + out_min;
}

float mapPotentiometerValueToAngle(float value){
  return mapPotentiometerValue(value, ANGLE_MIN, ANGLE_MAX);
}

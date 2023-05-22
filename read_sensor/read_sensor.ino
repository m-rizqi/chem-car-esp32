#include <OneWire.h>
#include <DallasTemperature.h>

#define POTENTIOMETER_PIN 15
#define POTENTIOMETER_MIN_ANGLE 0
#define POTENTIOMETER_MAX_ANGLE 4095

#define TERMOMETER_PIN 13
OneWire oneWire(TERMOMETER_PIN);
DallasTemperature DS18B20(&oneWire);

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(115200);
  DS18B20.begin();
}

void loop() {
  int analogValue = analogRead(POTENTIOMETER_PIN);
  float voltage = floatMap(analogValue, POTENTIOMETER_MIN_ANGLE, POTENTIOMETER_MAX_ANGLE, 0, 3.3);

  // print out the value you read:
  Serial.print("Analog: ");
  Serial.print(analogValue);
  Serial.print(", Voltage: ");
  Serial.println(voltage);

  DS18B20.requestTemperatures();       // send the command to get temperatures
  float tempC = DS18B20.getTempCByIndex(0);  // read temperature in °C
  float tempF = tempC * 9 / 5 + 32; // convert °C to °F

  Serial.print("Temperature: ");
  Serial.print(tempC);    // print the temperature in °C
  Serial.print("°C");
  Serial.print("  ~  ");  // separator between °C and °F
  Serial.print(tempF);    // print the temperature in °F
  Serial.println("°F");

  delay(1000);
}

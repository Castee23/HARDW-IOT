#include "hardware.h"

void setup() {
  Serial.begin(9600);
  pinMode(PIN_SENSOR_LUZ, INPUT);
}

void loop() {
  int valorLuz = analogRead(PIN_SENSOR_LUZ);
  Serial.println(valorLuz);
  delay(200);
}

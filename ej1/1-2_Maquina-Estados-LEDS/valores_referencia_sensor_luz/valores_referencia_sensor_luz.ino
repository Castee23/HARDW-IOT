#include "hardware.h"

// Sketch de calibracion: imprime lectura actual + minimo y maximo vistos.
// 1) Tapar el sensor con el dedo unos segundos  -> anotar MIN
// 2) Acercar la fuente de luz elegida (flash)   -> anotar MAX
// Pulsar reset para empezar una medida nueva.

int minVisto = 1023;
int maxVisto = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) { }          // 32U4: esperar a que se abra el monitor serie
  pinMode(PIN_SENSOR_LUZ, INPUT);
}

void loop() {
  int valorLuz = analogRead(PIN_SENSOR_LUZ);
  if (valorLuz < minVisto) minVisto = valorLuz;
  if (valorLuz > maxVisto) maxVisto = valorLuz;

  Serial.print("actual=");
  Serial.print(valorLuz);
  Serial.print("  min=");
  Serial.print(minVisto);
  Serial.print("  max=");
  Serial.println(maxVisto);
  delay(200);
}

#include "hardware.h"

// Sketch de calibracion del ADXL335 (ejercicio 1.3).
// Fase 1: girar la placa despacio por sus 6 caras -> min/max de cada eje (= -1 g / +1 g).
// Fase 2: con la pulsera puesta, anotar X/Y/Z en reposo y en el gesto de "mirar la hora".
// Enviar 'r' desde el monitor serie reinicia los min/max.

#define N_MUESTRAS          16     // media de 16 conversiones por eje: baja el ruido
#define UMBRAL_QUIETO       3      // cuentas: si ningun eje varia mas que esto, la placa esta quieta
#define PERIODO_MS          250UL
#define ESPERA_MONITOR_MS   3000UL

int minX = 1023, minY = 1023, minZ = 1023;
int maxX = 0,    maxY = 0,    maxZ = 0;
int prevX = 0,   prevY = 0,   prevZ = 0;

// La primera conversion tras cambiar de canal se descarta: el condensador de muestreo
// del ADC arrastra carga del canal anterior y la salida del ADXL335 (32 kOhm) tarda en
// reponerla (el ADC del 32U4 esta pensado para fuentes de <= 10 kOhm)
int leerEje(uint8_t pin) {
  analogRead(pin);
  long suma = 0;
  for (int i = 0; i < N_MUESTRAS; i++) {
    suma += analogRead(pin);
  }
  return (int)(suma / N_MUESTRAS);
}

void reiniciarExtremos() {
  minX = minY = minZ = 1023;
  maxX = maxY = maxZ = 0;
}

void imprimirEje(const char* nombre, int actual, int minimo, int maximo) {
  Serial.print(nombre);
  Serial.print("=");
  Serial.print(actual);
  Serial.print(" [");
  Serial.print(minimo);
  Serial.print("..");
  Serial.print(maximo);
  Serial.print("]   ");
}

void setup() {
  Serial.begin(9600);
  unsigned long tArranque = millis();
  while (!Serial && millis() - tArranque < ESPERA_MONITOR_MS) { }

  // INPUT sin pull-up: una pull-up de ~40 kOhm formaria divisor con los 32 kOhm del sensor
  pinMode(PIN_ACEL_X, INPUT);
  pinMode(PIN_ACEL_Y, INPUT);
  pinMode(PIN_ACEL_Z, INPUT);

  Serial.println("Calibracion ADXL335 - formato: eje=actual [min..max]");
  Serial.println("Min/max solo se actualizan con la placa quieta. 'r' = reiniciar extremos");
}

void loop() {
  if (Serial.available() && Serial.read() == 'r') {
    reiniciarExtremos();
    Serial.println("--- extremos reiniciados ---");
  }

  int x = leerEje(PIN_ACEL_X);
  int y = leerEje(PIN_ACEL_Y);
  int z = leerEje(PIN_ACEL_Z);

  // Con la placa en movimiento se suman aceleraciones dinamicas (> 1 g) a la gravedad:
  // esas lecturas no sirven para calibrar, solo cuentan las posiciones estaticas
  bool quieto = abs(x - prevX) <= UMBRAL_QUIETO &&
                abs(y - prevY) <= UMBRAL_QUIETO &&
                abs(z - prevZ) <= UMBRAL_QUIETO;
  prevX = x;
  prevY = y;
  prevZ = z;

  if (quieto) {
    minX = min(minX, x);  maxX = max(maxX, x);
    minY = min(minY, y);  maxY = max(maxY, y);
    minZ = min(minZ, z);  maxZ = max(maxZ, z);
  }

  imprimirEje("X", x, minX, maxX);
  imprimirEje("Y", y, minY, maxY);
  imprimirEje("Z", z, minZ, maxZ);
  Serial.println(quieto ? "(quieto)" : "(moviendo)");

  delay(PERIODO_MS);  // sketch de medida, sin maquina de estados: delay() aceptable aqui
}

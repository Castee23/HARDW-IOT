#ifndef HARDWARE_H
#define HARDWARE_H

// ---- Acelerometro ADXL335 (alimentado a 3.3 V desde "+" y "-" de la placa) ----
// Salidas analogicas con 32 kOhm internos: NUNCA pinMode INPUT_PULLUP en estos pines
#define PIN_ACEL_X          A7  // pin 7: desconectar el LED verde exterior si sigue unido
#define PIN_ACEL_Y          A8  // pin 8: desconectar el LED azul exterior si sigue unido
#define PIN_ACEL_Z          A9  // pin 9: interruptor de la placa en posicion ABIERTA (o puerto de expansion A9)

#endif // HARDWARE_H

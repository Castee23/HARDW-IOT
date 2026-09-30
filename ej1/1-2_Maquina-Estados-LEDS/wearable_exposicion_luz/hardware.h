#ifndef HARDWARE_H
#define HARDWARE_H

// ---- Perifericos exteriores (placa ProtoSnap) ----
#define PIN_SENSOR_LUZ      A2  // canal analogico A2 (explicito para analogRead)
#define PIN_ZUMBADOR        6   // solo digital: no gasta canal analogico (antes en A3)
#define PIN_BOTON           10  // pad "10" (serigrafia SCL), solo digital. Antes en A4
#define PIN_INTERRUPTOR     9   // tambien A9

// LEDs de colores exteriores no usados (el RGB interior hace su funcion).
// Pines libres con canal analogico, reservados para el ADXL335 del 1.3:
//   A3 (pin 3), A5 (pin 5), A7 (pin 7), A8 (pin 8)

// ---- Bus I2C ----
// PIN_SCL (10) ocupado por el boton: sin I2C en este ejercicio
#define PIN_SDA             11

// ---- LED RGB interior ----
#define PIN_RGB_ROJO        12  // PWM
#define PIN_RGB_VERDE       13  // PWM
#define PIN_RGB_AZUL        14  // PWM

// ---- Barra de 6 LEDs blancos interior ----
#define PIN_BARRA_LED_0     15
#define PIN_BARRA_LED_1     16
#define PIN_BARRA_LED_2     17
#define PIN_BARRA_LED_3     18
#define PIN_BARRA_LED_4     19
#define PIN_BARRA_LED_5     20

#endif // HARDWARE_H

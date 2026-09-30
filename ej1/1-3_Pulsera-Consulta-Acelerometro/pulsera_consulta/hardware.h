#ifndef HARDWARE_H
#define HARDWARE_H

// ---- Perifericos exteriores (placa ProtoSnap) ----
#define PIN_SENSOR_LUZ      A2  // canal analogico A2 (explicito para analogRead)
#define PIN_ZUMBADOR        6   // solo digital: no gasta canal analogico
#define PIN_BOTON           10  // pad "10" (serigrafia SCL), solo digital

// ---- Acelerometro ADXL335 (alimentado a 3.3 V desde "+" y "-" de la placa) ----
// Salidas analogicas con 32 kOhm internos: NUNCA pinMode INPUT_PULLUP en estos pines
#define PIN_ACEL_X          A7  // pin 7 (LED verde exterior: Vf > salida maxima, no carga la linea)
#define PIN_ACEL_Y          A8  // pin 8 (LED azul exterior). Linea con contacto intermitente: no se usa
#define PIN_ACEL_Z          A9  // pin 9: interruptor de la placa en posicion ABIERTA

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

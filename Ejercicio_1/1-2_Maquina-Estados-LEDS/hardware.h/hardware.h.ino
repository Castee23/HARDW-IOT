#ifndef HARDWARE_H
#define HARDWARE_H

// ---- Perifericos exteriores (placa ProtoSnap) ----
#define PIN_SENSOR_LUZ      2   // tambien A2
#define PIN_ZUMBADOR        3   // tambien A3
#define PIN_BOTON           4   // tambien A4
#define PIN_LED_AMARILLO    5   // tambien A5
#define PIN_LED_ROJO        6   // PWM, sin canal analogico en el esquematico
#define PIN_LED_VERDE       7   // PWM, tambien A7
#define PIN_LED_AZUL        8   // PWM, tambien A8
#define PIN_INTERRUPTOR     9   // tambien A9

// ---- Bus I2C ----
#define PIN_SCL             10  // PWM
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

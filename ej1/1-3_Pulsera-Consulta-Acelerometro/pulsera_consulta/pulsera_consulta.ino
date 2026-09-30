#include "hardware.h"

// Ejercicio 1.3: el wearable del 1.2 solo muestra la luminosidad en la barra cuando el
// usuario "consulta" la pulsera (gesto de mirar la hora), detectado con el ADXL335.
// La medida de exposicion, la alarma y el aviso verde siguen funcionando siempre.

// ---- Calibracion sensor de luz (medida con valores_referencia_sensor_luz, 2026-09-30) ----
#define LUZ_MIN              6     // sensor tapado con el dedo
#define LUZ_MAX              970   // flash del movil pegado al sensor
#define UMBRAL_LUMINOSIDAD   850   // ambiente del aula ~420: margen amplio por debajo del flash

// ---- Deteccion de consulta (medida con valores_referencia_acelerometro, 2026-09-30) ----
// Cuentas ADC brutas del eje Z (perpendicular a la placa), pulsera puesta y quieta:
//   brazo relajado   Z = 441..474 (calibracion), 497..501 (prueba del sketch final)
//   mirar la hora    Z = 578..612, con bajones puntuales a ~549 por temblor de la mano
//   brazo en la mesa Z = 575  -> indistinguible de mirar: con el brazo apoyado la barra tambien se ve
// X no se usa (solo separa ~26 cuentas, menos que la variacion del gesto) ni Y (contacto intermitente)
#define UMBRAL_Z_ENTRADA     545   // ~30 cuentas por debajo del minimo medido al mirar
#define UMBRAL_Z_SALIDA      520   // ~20 por encima del relajado y ~30 por debajo de los bajones al mirar
#define Z_MIN_VALIDO         300   // fuera de ~+-2 g la lectura es imposible en reposo:
#define Z_MAX_VALIDO         720   // cable suelto o en corto -> se trata como "no consulta"
#define TIEMPO_CONFIRMAR_MS  400UL // postura mantenida: descarta el balanceo del brazo al andar
#define PERIODO_ACEL_MS      50UL
#define N_MUESTRAS_ACEL      8

// ---- Tiempos del enunciado ----
#define TIEMPO_ALARMA_MS     20000UL
#define TIEMPO_COOLDOWN_MS   30000UL
#define PERIODO_BARRA_MS     400UL
#define PERIODO_PARPADEO_MS  500UL
#define PERIODO_DEBOUNCE_MS  50UL

// ---- Actuadores ----
#define FRECUENCIA_ALARMA_HZ 2000  // el zumbador necesita onda cuadrada (tone), no nivel fijo
#define NUM_LEDS_BARRA       6

// Si el LED RGB se enciende al reves (anodo comun), intercambiar estos dos valores
#define RGB_ENCENDIDO        HIGH
#define RGB_APAGADO          LOW

// ---- Depuracion por monitor serie (poner a 0 en la version final) ----
#define DEBUG_SERIE          1
#define PERIODO_DEBUG_MS     500UL
#define ESPERA_MONITOR_MS    3000UL  // tiempo maximo esperando a que se abra el monitor

enum EstadoExposicion { SIN_EXPOSICION, EN_EXPOSICION, ALARMA, COOLDOWN };
EstadoExposicion estado = SIN_EXPOSICION;

// REPOSO: barra apagada, esperando el gesto
// CONFIRMANDO: Z sobre el umbral, falta mantenerlo TIEMPO_CONFIRMAR_MS
// MOSTRANDO: barra encendida mientras se mantenga la postura de mirar la hora
enum EstadoConsulta { REPOSO, CONFIRMANDO, MOSTRANDO };
EstadoConsulta estadoConsulta = REPOSO;

const int pines_BARRA_LEDS[NUM_LEDS_BARRA] = {PIN_BARRA_LED_0, PIN_BARRA_LED_1, PIN_BARRA_LED_2,
                                              PIN_BARRA_LED_3, PIN_BARRA_LED_4, PIN_BARRA_LED_5};

unsigned long tInicioExposicion = 0;
unsigned long tInicioCooldown = 0;
unsigned long tUltimoToggleVerde = 0;
unsigned long tUltimaBarra = 0;
unsigned long tUltimaMuestraAcel = 0;
unsigned long tInicioConfirmar = 0;
bool verdeEncendido = false;
bool notificacionMedia = false;
bool botonUltimoLeido = HIGH;   // se inicializa con la lectura real en setup()
int zAcel = 0;                  // ultima media del eje Z

const char* nombreEstado(EstadoExposicion e) {
  switch (e) {
    case SIN_EXPOSICION: return "SIN_EXPOSICION";
    case EN_EXPOSICION:  return "EN_EXPOSICION";
    case ALARMA:         return "ALARMA";
    case COOLDOWN:       return "COOLDOWN";
  }
  return "?";
}

const char* nombreConsulta(EstadoConsulta e) {
  switch (e) {
    case REPOSO:      return "REPOSO";
    case CONFIRMANDO: return "CONFIRMANDO";
    case MOSTRANDO:   return "MOSTRANDO";
  }
  return "?";
}

void cambiarEstado(EstadoExposicion nuevo) {
#if DEBUG_SERIE
  Serial.print(">>> ");
  Serial.print(nombreEstado(estado));
  Serial.print(" -> ");
  Serial.println(nombreEstado(nuevo));
#endif
  estado = nuevo;
}

void cambiarEstadoConsulta(EstadoConsulta nuevo) {
#if DEBUG_SERIE
  Serial.print(">>> consulta ");
  Serial.print(nombreConsulta(estadoConsulta));
  Serial.print(" -> ");
  Serial.print(nombreConsulta(nuevo));
  Serial.print("  (z=");
  Serial.print(zAcel);
  Serial.println(")");
#endif
  estadoConsulta = nuevo;
}

// Con varios canales analogicos, el condensador de muestreo del ADC arrastra carga del
// canal anterior: la primera conversion tras cambiar de canal se descarta
int leerLuz() {
  analogRead(PIN_SENSOR_LUZ);
  return analogRead(PIN_SENSOR_LUZ);
}

// Media de N_MUESTRAS_ACEL conversiones, descartando la primera (salida del ADXL335 de 32 kOhm,
// mas alta que los <= 10 kOhm para los que esta pensado el ADC)
int leerEjeAcel(uint8_t pin) {
  analogRead(pin);
  long suma = 0;
  for (int i = 0; i < N_MUESTRAS_ACEL; i++) {
    suma += analogRead(pin);
  }
  return (int)(suma / N_MUESTRAS_ACEL);
}

void muestrearAcelerometro() {
  if (millis() - tUltimaMuestraAcel < PERIODO_ACEL_MS) {
    return;
  }
  tUltimaMuestraAcel = millis();
  zAcel = leerEjeAcel(PIN_ACEL_Z);
}

bool zValida(int z) {
  return z >= Z_MIN_VALIDO && z <= Z_MAX_VALIDO;
}

void setup() {
#if DEBUG_SERIE
  Serial.begin(9600);
  // Espera acotada: con PC da tiempo a abrir el monitor, con bateria arranca igual
  unsigned long tArranque = millis();
  while (!Serial && millis() - tArranque < ESPERA_MONITOR_MS) { }
  Serial.println("Pulsera con consulta por gesto - arrancado");
#endif

  pinMode(PIN_SENSOR_LUZ, INPUT);
  pinMode(PIN_ZUMBADOR, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_RGB_VERDE, OUTPUT);
  for (int i = 0; i < NUM_LEDS_BARRA; i++) { pinMode(pines_BARRA_LEDS[i], OUTPUT); }

  // INPUT sin pull-up: una pull-up de ~40 kOhm formaria divisor con los 32 kOhm del sensor
  pinMode(PIN_ACEL_X, INPUT);
  pinMode(PIN_ACEL_Y, INPUT);
  pinMode(PIN_ACEL_Z, INPUT);

  noTone(PIN_ZUMBADOR);
  digitalWrite(PIN_RGB_VERDE, RGB_APAGADO);

  // Partir del estado real: un boton ya pulsado (o en corto) no cuenta como flanco
  botonUltimoLeido = digitalRead(PIN_BOTON);
  zAcel = leerEjeAcel(PIN_ACEL_Z);
}

// Se llama en cada vuelta de loop() para no perder el historial del boton
bool flancoBotonPulsado() {
  static unsigned long tUltimoCambio = 0;
  bool actual = digitalRead(PIN_BOTON);

  if (actual != botonUltimoLeido && (millis() - tUltimoCambio) > PERIODO_DEBOUNCE_MS) {
    tUltimoCambio = millis();
    botonUltimoLeido = actual;
    if (actual == LOW) return true; // flanco de bajada = pulsacion valida
  }
  return false;
}

// Reparte [LUZ_MIN, LUZ_MAX] en 7 tramos iguales -> 0..6 LEDs encendidos
int nivelBarra(int luminosidad) {
  int lum = constrain(luminosidad, LUZ_MIN, LUZ_MAX);
  long nivel = (long)(lum - LUZ_MIN) * (NUM_LEDS_BARRA + 1) / (LUZ_MAX - LUZ_MIN + 1);
  return (int)nivel;
}

void escribirBarra(int nivel) {
  for (int i = 0; i < NUM_LEDS_BARRA; i++) {
    digitalWrite(pines_BARRA_LEDS[i], (i < nivel) ? HIGH : LOW);
  }
}

void apagarBarra() {
  escribirBarra(0);
}

// Solo se llama durante una consulta: refresco con la cadencia de 400 ms del enunciado
void actualizarBarraLeds(int luminosidad) {
  if (millis() - tUltimaBarra < PERIODO_BARRA_MS) {
    return;
  }
  tUltimaBarra = millis();
  escribirBarra(nivelBarra(luminosidad));
}

// Maquina de estados de la consulta, independiente de la de exposicion.
// Histeresis: se entra por encima de UMBRAL_Z_ENTRADA y se sale por debajo de UMBRAL_Z_SALIDA,
// asi el balanceo de +-20 cuentas cerca de un umbral no hace parpadear la barra.
// Una lectura fuera de rango (fallo de conexion) cuenta como brazo bajado: la barra se apaga.
void gestionarConsulta(int luminosidad) {
  bool valida = zValida(zAcel);
  bool sobreEntrada = valida && zAcel > UMBRAL_Z_ENTRADA;
  bool bajoSalida = !valida || zAcel < UMBRAL_Z_SALIDA;

  switch (estadoConsulta) {

    case REPOSO:
      if (sobreEntrada) {
        cambiarEstadoConsulta(CONFIRMANDO);
        tInicioConfirmar = millis();
      }
      break;

    case CONFIRMANDO:
      if (!sobreEntrada) {
        cambiarEstadoConsulta(REPOSO);
      } else if (millis() - tInicioConfirmar >= TIEMPO_CONFIRMAR_MS) {
        cambiarEstadoConsulta(MOSTRANDO);
        escribirBarra(nivelBarra(luminosidad));  // respuesta inmediata, sin esperar 400 ms
        tUltimaBarra = millis();
      }
      break;

    case MOSTRANDO:
      if (bajoSalida) {
        apagarBarra();
        cambiarEstadoConsulta(REPOSO);
      } else {
        actualizarBarraLeds(luminosidad);
      }
      break;
  }
}

void gestionarParpadeoVerde() {
  if (!notificacionMedia || estado != EN_EXPOSICION) {
    return;
  }
  if (millis() - tUltimoToggleVerde >= PERIODO_PARPADEO_MS) {
    tUltimoToggleVerde = millis();
    verdeEncendido = !verdeEncendido;
    digitalWrite(PIN_RGB_VERDE, verdeEncendido ? RGB_ENCENDIDO : RGB_APAGADO);
  }
}

void apagarNotificacionVerde() {
  notificacionMedia = false;
  verdeEncendido = false;
  digitalWrite(PIN_RGB_VERDE, RGB_APAGADO);
}

// Linea periodica: estados, luz, eje Z y entradas
void imprimirDebug(int luminosidad) {
#if DEBUG_SERIE
  static unsigned long tUltimoDebug = 0;
  if (millis() - tUltimoDebug < PERIODO_DEBUG_MS) {
    return;
  }
  tUltimoDebug = millis();

  Serial.print("estado=");
  Serial.print(nombreEstado(estado));
  Serial.print("  luz=");
  Serial.print(luminosidad);
  Serial.print(luminosidad >= UMBRAL_LUMINOSIDAD ? "(>=umbral)" : "(<umbral)");

  Serial.print("  z=");
  Serial.print(zAcel);
  if (!zValida(zAcel)) {
    Serial.print("(FUERA DE RANGO)");
  }
  Serial.print("  consulta=");
  Serial.print(nombreConsulta(estadoConsulta));
  if (estadoConsulta == MOSTRANDO) {
    Serial.print(" barra=");
    Serial.print(nivelBarra(luminosidad));
    Serial.print("/6");
  }

  if (estado == EN_EXPOSICION) {
    Serial.print("  t_exp=");
    Serial.print((millis() - tInicioExposicion) / 1000.0, 1);
    Serial.print("s  verde=");
    Serial.print(verdeEncendido ? "ON" : "off");
  } else if (estado == COOLDOWN) {
    Serial.print("  t_cooldown=");
    Serial.print((millis() - tInicioCooldown) / 1000.0, 1);
    Serial.print("s");
  }

  Serial.print("  boton=");
  Serial.println(digitalRead(PIN_BOTON) == LOW ? "PULSADO" : "suelto");
#endif
}

void loop() {
  int luminosidad = leerLuz();
  bool pulsacion = flancoBotonPulsado();
  muestrearAcelerometro();
  gestionarConsulta(luminosidad);

  switch (estado) {

    case SIN_EXPOSICION:
      if (luminosidad >= UMBRAL_LUMINOSIDAD) {
        cambiarEstado(EN_EXPOSICION);
        tInicioExposicion = millis();
        notificacionMedia = false;
      }
      break;

    case EN_EXPOSICION: {
      if (luminosidad < UMBRAL_LUMINOSIDAD) {
        cambiarEstado(SIN_EXPOSICION);
        apagarNotificacionVerde();
        break;
      }
      unsigned long transcurrido = millis() - tInicioExposicion;
      if (transcurrido >= TIEMPO_ALARMA_MS) {
        cambiarEstado(ALARMA);
        apagarNotificacionVerde();
        tone(PIN_ZUMBADOR, FRECUENCIA_ALARMA_HZ);
      } else if (transcurrido >= TIEMPO_ALARMA_MS / 2) {
        notificacionMedia = true;
      }
      break;
    }

    case ALARMA:
      if (pulsacion) {
        noTone(PIN_ZUMBADOR);
        cambiarEstado(COOLDOWN);
        tInicioCooldown = millis();
      }
      break;

    case COOLDOWN:
      if (millis() - tInicioCooldown >= TIEMPO_COOLDOWN_MS) {
        cambiarEstado(SIN_EXPOSICION);
      }
      break;
  }

  gestionarParpadeoVerde();
  imprimirDebug(luminosidad);
}

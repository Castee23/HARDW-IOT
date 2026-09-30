#include "hardware.h"

// ---- Calibracion (medida con valores_referencia_sensor_luz, 2026-09-30) ----
#define LUZ_MIN              6     // sensor tapado con el dedo
#define LUZ_MAX              970   // flash del movil pegado al sensor
#define UMBRAL_LUMINOSIDAD   850   // ambiente del aula ~420: margen amplio por debajo del flash

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

const int pines_BARRA_LEDS[NUM_LEDS_BARRA] = {PIN_BARRA_LED_0, PIN_BARRA_LED_1, PIN_BARRA_LED_2,
                                              PIN_BARRA_LED_3, PIN_BARRA_LED_4, PIN_BARRA_LED_5};

unsigned long tInicioExposicion = 0;
unsigned long tInicioCooldown = 0;
unsigned long tUltimoToggleVerde = 0;
unsigned long tUltimaBarra = 0;
bool verdeEncendido = false;
bool notificacionMedia = false;
bool botonUltimoLeido = HIGH;   // se inicializa con la lectura real en setup()

const char* nombreEstado(EstadoExposicion e) {
  switch (e) {
    case SIN_EXPOSICION: return "SIN_EXPOSICION";
    case EN_EXPOSICION:  return "EN_EXPOSICION";
    case ALARMA:         return "ALARMA";
    case COOLDOWN:       return "COOLDOWN";
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

void setup() {
#if DEBUG_SERIE
  Serial.begin(9600);
  // Espera acotada: con PC da tiempo a abrir el monitor, con bateria arranca igual
  unsigned long tArranque = millis();
  while (!Serial && millis() - tArranque < ESPERA_MONITOR_MS) { }
  Serial.println("Wearable exposicion luz - arrancado");
#endif

  pinMode(PIN_SENSOR_LUZ, INPUT);
  pinMode(PIN_ZUMBADOR, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_RGB_VERDE, OUTPUT);
  for (int i = 0; i < NUM_LEDS_BARRA; i++) { pinMode(pines_BARRA_LEDS[i], OUTPUT); }

  noTone(PIN_ZUMBADOR);
  digitalWrite(PIN_RGB_VERDE, RGB_APAGADO);

  // Partir del estado real: un boton ya pulsado (o en corto) no cuenta como flanco
  botonUltimoLeido = digitalRead(PIN_BOTON);
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

void actualizarBarraLeds(int luminosidad) {
  if (millis() - tUltimaBarra < PERIODO_BARRA_MS) {
    return;
  }
  tUltimaBarra = millis();

  int nivel = nivelBarra(luminosidad);
  for (int i = 0; i < NUM_LEDS_BARRA; i++) {
    digitalWrite(pines_BARRA_LEDS[i], (i < nivel) ? HIGH : LOW);
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

// Linea periodica: estado, luz, LEDs de la barra, tiempos y entradas
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
  Serial.print("  barra=");
  Serial.print(nivelBarra(luminosidad));
  Serial.print("/6");

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
  int luminosidad = analogRead(PIN_SENSOR_LUZ);
  bool pulsacion = flancoBotonPulsado();
  actualizarBarraLeds(luminosidad);

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

#include "hardware.h"

// ---- Calibracion (ajustar con valores_referencia_sensor_luz) ----
#define UMBRAL_LUMINOSIDAD   700UL
#define TIEMPO_ALARMA_MS     20000UL
#define TIEMPO_COOLDOWN_MS   30000UL
#define PERIODO_PARPADEO_MS  500UL
#define PERIODO_DEBOUNCE_MS  50UL

enum EstadoExposicion { SIN_EXPOSICION, EN_EXPOSICION, ALARMA, COOLDOWN };
EstadoExposicion estado = SIN_EXPOSICION;

const int pines_BARRA_LEDS[6] = {PIN_BARRA_LED_0, PIN_BARRA_LED_1, PIN_BARRA_LED_2,
                                  PIN_BARRA_LED_3, PIN_BARRA_LED_4, PIN_BARRA_LED_5};

unsigned long tInicioExposicion = 0;
unsigned long tInicioCooldown = 0;
unsigned long tUltimoToggleVerde = 0;
bool verdeEncendido = false;
bool notificacionMedia = false;

void setup() {
  pinMode(PIN_SENSOR_LUZ, INPUT);
  pinMode(PIN_ZUMBADOR, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_RGB_VERDE, OUTPUT);
  for (int i = 0; i < 6; i++) { pinMode(pines_BARRA_LEDS[i], OUTPUT); }

  digitalWrite(PIN_ZUMBADOR, LOW);
  digitalWrite(PIN_RGB_VERDE, LOW);
}

bool flancoBotonPulsado() {
  static bool ultimoEstadoLeido = HIGH;
  static unsigned long tUltimoCambio = 0;
  bool actual = digitalRead(PIN_BOTON);

  if (actual != ultimoEstadoLeido && (millis() - tUltimoCambio) > PERIODO_DEBOUNCE_MS) {
    tUltimoCambio = millis();
    ultimoEstadoLeido = actual;
    if (actual == LOW) return true; // flanco de bajada = pulsacion valida
  }
  return false;
}

void actualizarBarraLeds(int luminosidad) {
  int nivel = map(luminosidad, 0, 1023, 0, 6);
  for (int i = 0; i < 6; i++) {
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
    digitalWrite(PIN_RGB_VERDE, verdeEncendido ? HIGH : LOW);
  }
}

void apagarNotificacionVerde() {
  notificacionMedia = false;
  verdeEncendido = false;
  digitalWrite(PIN_RGB_VERDE, LOW);
}

void loop() {
  int luminosidad = analogRead(PIN_SENSOR_LUZ);
  actualizarBarraLeds(luminosidad);

  switch (estado) {

    case SIN_EXPOSICION:
      if (luminosidad >= UMBRAL_LUMINOSIDAD) {
        estado = EN_EXPOSICION;
        tInicioExposicion = millis();
        notificacionMedia = false;
      }
      break;

    case EN_EXPOSICION: {
      if (luminosidad < UMBRAL_LUMINOSIDAD) {
        estado = SIN_EXPOSICION;
        apagarNotificacionVerde();
        break;
      }
      unsigned long transcurrido = millis() - tInicioExposicion;
      if (transcurrido >= TIEMPO_ALARMA_MS) {
        estado = ALARMA;
        apagarNotificacionVerde();
        digitalWrite(PIN_ZUMBADOR, HIGH);
      } else if (transcurrido >= TIEMPO_ALARMA_MS / 2) {
        notificacionMedia = true;
      }
      break;
    }

    case ALARMA:
      if (flancoBotonPulsado()) {
        digitalWrite(PIN_ZUMBADOR, LOW);
        estado = COOLDOWN;
        tInicioCooldown = millis();
      }
      break;

    case COOLDOWN:
      if (millis() - tInicioCooldown >= TIEMPO_COOLDOWN_MS) {
        estado = SIN_EXPOSICION;
      }
      break;
  }

  gestionarParpadeoVerde();
}

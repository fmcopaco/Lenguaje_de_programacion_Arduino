// Libreria Semaforo.h -- Paco Cañada 2020 -- https://usuaris.tinet.cat/fmco/

//#include <Arduino.h>
#include "Semaforo.h"

// Constructor. Se llama al crear el objeto
Semaforo::Semaforo () {
  for (int i = 0; i < NUMLEDS; i++) {                     // Inicializa array de LED
    Leds[i].pinLED = -1;
    Leds[i].estado = OFF;
    Leds[i].currPWM = 0;
    Leds[i].interval = 100;
    Leds[i].timerPWM = millis();
  }
}

void Semaforo::init (int pinRojo, int pinVerde, int pinAmarillo = -1, int pinBlanco = -1) {
  Leds[ROJO].pinLED = pinRojo;
  Leds[VERDE].pinLED = pinVerde;
  Leds[AMARILLO].pinLED = pinAmarillo;
  Leds[BLANCO].pinLED = pinBlanco;
  if (pinRojo != -1) pinMode (pinRojo, OUTPUT);
  if (pinVerde != -1) pinMode (pinVerde, OUTPUT);
  if (pinAmarillo != -1) pinMode (pinAmarillo, OUTPUT);
  if (pinBlanco != -1) pinMode (pinBlanco, OUTPUT);
}

void Semaforo::initManiobra (int pinRojo, int pinBlanco) {
  init (pinRojo, -1, -1, pinBlanco);
};

void Semaforo::initPasoNivelVehiculos (int pinDerecha, int pinIzquierda) {
  init (pinDerecha, pinIzquierda, -1, -1);
}

void Semaforo::process () {                               // procesa estado de la señal
  _currTime = millis();
  if (_currTime - _flashTime > FLASH_INTERVAL) {          // actualiza temporizador parpadeos
    _flashTime = _currTime;
    _flashFase = ! _flashFase;
  }
  for (int i = 0; i < NUMLEDS; i++) {                     // controla los LED
    if (Leds[i].pinLED != -1) {                           // si tiene definido un pin
      switch (Leds[i].estado) {                           // segun su estado actualiza brillo final
        case OFF:
          Leds[i].finPWM = 0;
          break;
        case ON:
          Leds[i].finPWM = PWM;
          break;
        case FLASH_A:
          if (_flashFase)
            Leds[i].finPWM = 0;
          else
            Leds[i].finPWM = PWM;
          break;
        case FLASH_B:
          if (_flashFase)
            Leds[i].finPWM = PWM;
          else
            Leds[i].finPWM = 0;
          break;
      }
      fadePin (&Leds[i], _currTime);                      // Controla brillo
    }
  }
}


void Semaforo::fadePin (LED *led, unsigned long currTime) {  // Controla brillo de un LED
  if (currTime - led->timerPWM > led->interval) {         // espera a que pase el intervalo
    led->timerPWM = currTime;
    switch (led->currPWM) {
      case 0:                                             // Brillo 0: Apaga LED
        digitalWrite (led->pinLED, APAGADO);
        led->interval = 100;
        if (led->finPWM != 0)                             // Si se tiene que encender cambia brillo
          led->currPWM++;
        break;
      case PWM:                                           // Brillo maximo. Enciende LED
        digitalWrite (led->pinLED, ENCENDIDO);
        led->interval = 100;
        if (led->finPWM < PWM)                            // Si se tiene que apagar cambia brillo
          led->currPWM--;
        break;
      default:                                            // Brillo intermedio. Control PWM del LED
        if (led->fasePWM) {
          digitalWrite (led->pinLED, ENCENDIDO);
          led->interval = led->currPWM;
        }
        else {
          digitalWrite (led->pinLED, APAGADO);
          led->interval = PWM - led->currPWM;
          (led->currPWM < led->finPWM) ? led->currPWM++ : led->currPWM--;   // Cambia brillo segun brillo final
        }
        led->fasePWM = !led->fasePWM;
        break;
    }
  }
}

void Semaforo::aspecto (byte aspect) {                     // Selecciona aspecto de la señal
  switch (aspect) {                                       // LED rojo segun aspecto
    case PARADA:
    case REBASE_AUTORIZADO:
    case REBASE_AUTORIZADO_NO_PARAR:
      Leds[ROJO].estado = ON;
      break;
    case PASO_NIVEL_CERRADO:
      Leds[ROJO].estado = FLASH_A;
      break;
    default:
      Leds[ROJO].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED verde segun aspecto
    case VIA_LIBRE:
    case ANUNCIO_PRECAUCION:
      Leds[VERDE].estado = ON;
      break;
    case PASO_NIVEL_CERRADO:
    case VIA_LIBRE_CONDICIONAL:
      Leds[VERDE].estado = FLASH_B;
      break;
    default:
      Leds[VERDE].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED amarillo segun aspecto
    case ANUNCIO_PARADA:
    case ANUNCIO_PRECAUCION:
      Leds[AMARILLO].estado = ON;
      break;
    case ANUNCIO_PARADA_INMEDIATA:
      Leds[AMARILLO].estado = FLASH_A;
      break;
    default:
      Leds[AMARILLO].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED blanco segun aspecto
    case REBASE_AUTORIZADO:
    case MOVIMIENTO_AUTORIZADO:
      Leds[BLANCO].estado = ON;
      break;
    case REBASE_AUTORIZADO_NO_PARAR:
      Leds[BLANCO].estado = FLASH_A;
      break;
    default:
      Leds[BLANCO].estado = OFF;
      break;
  }
}

// -----------------------------------

void Semaforo::initHauptSignal (int pinRojo, int pinVerde, int pinAmarillo = -1, int pinBlanco = -1) {
  init (pinRojo, pinVerde, pinAmarillo, pinBlanco);
}

void Semaforo::initVorSignal (int pinVerde1, int pinVerde2, int pinAmarillo1, int pinAmarillo2) {
  init (pinVerde1, pinVerde2, pinAmarillo1, pinAmarillo2);
}

void Semaforo::aspectoDB (byte aspect) {                             // Selecciona aspecto de la señal
  switch (aspect) {                                       // LED rojo segun aspecto
    case HP0:
    case HP0_Sh1:
    case Vr1:
      Leds[ROJO].estado = ON;
      break;
    default:
      Leds[ROJO].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED verde segun aspecto
    case HP1:
    case HP2:
    case Vr1:
    case Vr2:
      Leds[VERDE].estado = ON;
      break;
    default:
      Leds[VERDE].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED amarillo segun aspecto
    case HP2:
    case Vr0:
    case Vr2:
      Leds[AMARILLO].estado = ON;
      break;
    default:
      Leds[AMARILLO].estado = OFF;
      break;
  }
  switch (aspect) {                                       // LED blanco segun aspecto
    case Sh1:
    case HP0_Sh1:
    case Vr0:
      Leds[BLANCO].estado = ON;
      break;
    default:
      Leds[BLANCO].estado = OFF;
      break;
  }
}

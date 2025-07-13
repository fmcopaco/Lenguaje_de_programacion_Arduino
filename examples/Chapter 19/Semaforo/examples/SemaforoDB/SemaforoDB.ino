#include <Semaforo.h>        // Libreria Semaforo.h -- Paco Cañada 2020 -- https://usuaris.tinet.cat/fmco/

Semaforo HauptSignal;
Semaforo VorSignal;

// Aspectos DB:   HP0, HP1, HP2, Sh1, HP0_Sh1, Vr0, Vr1, Vr2

const int pinInterruptor = 2;
int estadoAnterior, estadoPin;

void setup() {
  pinMode (pinInterruptor, INPUT_PULLUP); // seleccion aspecto a mostrar 5V: HP0-Vr0, GND: HP1-Vr1
  HauptSignal.initHauptSignal (3, 4);     // Puede tener 2, 3 o 4 luces (pins: Rojo, Verde, Amarillo, Blanco)
  VorSignal.initVorSignal (7, 8, 9, 10);  // (pins: Verde1, Verde2, Amarillo1, Amarillo2)
  estadoAnterior = HIGH;
  HauptSignal.aspectoDB (HP0);
  VorSignal.aspectoDB (Vr0);
}

void loop() {
  HauptSignal.process();
  VorSignal.process();
  estadoPin =  digitalRead (pinInterruptor);
  if (estadoPin != estadoAnterior) {
    estadoAnterior = estadoPin;
    if (estadoPin == LOW) {
      HauptSignal.aspectoDB (HP1);
      VorSignal.aspectoDB (Vr1);
    }
    else {
      HauptSignal.aspectoDB (HP0);
      VorSignal.aspectoDB (Vr0);
    }
  }
}


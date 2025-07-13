// Control de placa deslizante con motor paso a paso - Paco Cañada 2019

#include <AccelStepper.h>                 // Libreria motores paso a paso con aceleracion/deceleracion

// Placa Arduino Motor Shield rev3
// salidos digitales:
const int dirA = 12;                      // dir: Control del motor por la libreria
const int dirB = 13;
const int pwmA = 3;                       // pwm: Tiene que estar a HIGH
const int pwmB = 11;
const int brakeA = 9;                     // brake: Tiene que estar a LOW si no se ha cortado su jumper en la shield
const int brakeB = 8;
// entradas analogicas
const int sns0 = A0;                      // Lectura de la corriente del motor/bobina. (1.65V/A)
const int sns1 = A1;
// entradas pulsadores (conector en la shield)
const int pinA2 = A2;                     // Pulsar para ir a la via
const int pinA3 = A3;
const int pinD5 = 5;
  const int pinD6 = 6;  
const int pinSDA = A4;
// entrada final de carrera
const int pinSCL = A5;                    // Para buscar el punto 0

#define numVias 5                         // Numero de vias/pulsadores de nuestra plataforma deslizante

#define Via1 600                          // Pasos en que estan las vias. Deteminados por prueba y error
#define Via2 750
#define Via3 900
#define Via4 1050
#define Via5 1200

#define FinCarrera pinSCL                 // Para que sea mas facil de interpretar listado

struct PuntoParo {                            // struct de datos para el punto, contiene posicion y pulsador asociado
  int pulsador;
  long posicion;
};

PuntoParo Via[] = {                    // Array de puntos de parada
  { pinA2, Via1 },                        // pin pulsador y posicion
  { pinA3, Via2 },
  { pinD5, Via3 },
  { pinD6, Via4 },
  { pinSDA, Via5}, 
};

#define MaxVelocidad  200                 // Velocidad normal (pasos por segundo)
#define IniVelocidad  100                 // Velocidad al buscar final de carrera
#define ZeroVelocidad 50                  // Velocidad al buscar el paso 0
#define Aceleracion 20                    // Aceleracion (pasos por segundo al cuadrado)

// Define el objeto y el tipo de interface para AccelStepper:
// Con Arduino motor shield rev3 usar   2 -> FULL2WIRE < 2 wire stepper, 2 motor pins required (H-Bridge)
// Con Pololu A4988/DRV8825 usar        1 -> DRIVER    < Stepper Driver, 2 driver pins required (driver board)
// Con motor unipolar y ULN2803 usar    4 -> FULL4WIRE < 4 wire full stepper, 4 motor pins required (4 transistors)
//                                      AccelStepper stepper( 4, pinA1, pinA2, pinB1, pinB2);

AccelStepper stepper( 2, dirA, dirB);     // se usan pines dir_a y dir_b 


void setup()
{  
  pinMode(pwmA, OUTPUT);                  // Inicializa pines de salida del Arduino motor shield
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);
  
  digitalWrite(pwmA, HIGH);
  digitalWrite(pwmB, HIGH);
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);

  pinMode (FinCarrera, INPUT_PULLUP);     // Inicializa pines de entrada (pulsadores y final de carrera)
  for (int n=0; n<numVias; n++)
    pinMode (Via[n].pulsador, INPUT_PULLUP);

  BuscaZero();                            // Busca el paso 0
  
  stepper.setMaxSpeed(MaxVelocidad);
  stepper.setAcceleration(Aceleracion);
  stepper.moveTo(Via[0].posicion);        // Vamos a la primera via
}

void loop(){  
  if (stepper.distanceToGo() == 0) {
    stepper.run();                          // let the AccelStepper to disable motor current after stop
    CompruebaBotones();
  }
  stepper.run();
}

void BuscaZero () {
  long distancia;

  distancia  = Via[0].posicion / 2;         // Para asegurar encontrar el final de carrera, preveemos movernos
  distancia += Via[numVias -1].posicion;    // la distancia a la ultima via mas la mitad hasta la primera
  stepper.setMaxSpeed(IniVelocidad);        // A velocidad de busqueda
  stepper.setAcceleration(Aceleracion);
  stepper.move(-distancia);                 // Posicion sera relativa al punto actual (antihorario)
  
  while (digitalRead(FinCarrera) == HIGH) { // Mover mientras no se pulse el final de carrera
    stepper.run();
  }

  stepper.setCurrentPosition(0);             // Paso 0 provisional
  distancia = IniVelocidad;                  // alejarse un segundo de velocidad de busqueda
  stepper.setMaxSpeed(ZeroVelocidad);        // pero mas despacio para menos inercia
  stepper.moveTo(distancia);                 // Distancia absoluta (horario)
  while (stepper.distanceToGo() != 0) {        // Mover hasta alejarse para soltar el final de carrera
    stepper.run();
  }
  stepper.moveTo(-distancia);                 // Distancia absoluta desde el cero provisional para asegura que toca
  do {                                        // Movemos hasta que pulse el final de carrera
    stepper.run();
  } while (digitalRead(FinCarrera) == HIGH);
  
  stepper.setCurrentPosition(0);              // ya hemos encontrado el paso 0
  delay(1000);                                 // Esperamos para que nos vean
}

void CompruebaBotones() {
  for (int n=0; n< numVias; n++) {
    if (digitalRead(Via[n].pulsador) == LOW)  // Mira si esta pulsado
      stepper.moveTo(Via[n].posicion);        // pone el nuevo destino
  }
}


// Control de placa giratoria de segmento con sonido - Paco Cañada 2019
// Las librerias Servo.h y TMRpcm son incompatibles en Arduino Uno/Nano. 
// Las dos usan el Timer1, tenemos que cambiar la libreria TMRpcm para que use el Timer2
// y usamos PWMServo para mas estabilidad

//   Board                     SERVO_PIN_A   SERVO_PIN_B   SERVO_PIN_C
//   -----                     -----------   -----------   -----------
//   Arduino Uno, Duemilanove       9            10          (none)
//   Arduino Mega                  11            12            13

#include <PWMServo.h>                 // Libreria para el Servo
#include <SPI.h>                    // librerias para la SD
#include <SD.h>                     // 
#include <TMRpcm.h>                 // Libreria para el sonido desde SD

TMRpcm Audio;                       // Objeto para generar audio
PWMServo servoPlaca;                // Objeto para mover el servo

#define pinPulsador 5               // Pin del pulsador
#define pinServo SERVO_PIN_A        // Pin del servo 
#define pinSD 10                    // Pin de seleccion de la SD

#define Via1 72                     // Posiciones del servo para cada via
#define Via2 90
#define Via3 108

int posicion;                       // Posicion actal del servo
int direccion;                      // Direccion 1 o -1

void setup() {
  pinMode (pinPulsador, INPUT_PULLUP);     // Definimos el pin del pulsador
  servoPlaca.attach(pinServo);      // Conectamos el servo
  servoPlaca.write(Via2); // Movemos el servo a una salida, mejor la central
  posicion = Via2;                  // Ponemos la posicion actual
  direccion = 1;                    // Direccion hacia siguiente via
  delay(1000);                      // Esperamos a que el servo se coloque
  Audio.speakerPin = 3;             // Altavoz en pin 3 si se usa Timer2, sino seria el 9
  Audio.setVolume(4);               // Gestión del volumen (0 a 7). Mayor de 4 distorsiona
  Audio.quality(1);                 // Calidad del audio
  SD.begin(pinSD);                  // Inicializamos rutinas de la SD
}

void loop() {
  switch (posicion) {               // actuamos segun la posicion en que estamos
    case Via1:
      direccion = 1;                // Cambiamos direccion hacia siguiente via
      CompruebaPosicion ();         // Comprueba pulsador y sonido
      break;
    case Via2:
      CompruebaPosicion ();         // Comprueba pulsador y sonido
      break;
    case Via3:
      direccion = -1;               // Cambiamos direccion hacia anterior via
      CompruebaPosicion ();         // Comprueba pulsador y sonido
      break;
    default:
      MueveServo ();                // Mocvemos el servo
      break;
  }
}


void CompruebaPosicion () {
  if (Audio.isPlaying())                  // Si hemos llegado, paramos el sonido
    Audio.disable();
  if (servoPlaca.attached())              // y desconectamos el servo 
    servoPlaca.detach();
  if (digitalRead (pinPulsador) == LOW) { // Si pulsamos el pulsador
    Audio.play("placa.wav");              // Ponemos el sonido
    servoPlaca.attach(pinServo);          // Conectamos el servo
    MueveServo ();                        // Empezamos a mover el servo
  }
}


void MueveServo () {
  posicion += direccion;                   // Movemos el servo un poco segun direccion
  servoPlaca.write (posicion);
  delay(38);
}


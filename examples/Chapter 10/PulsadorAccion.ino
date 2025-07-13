// Control de un pulsador de accion con tres servos y 10 salidas digitales - Paco Cañada 2019

#include <VarSpeedServo.h>    // https://github.com/netlabtoolkit/VarSpeedServo?utm_source=rb-community&utm_medium=forum&utm_campaign=better-arduino-servo-library

/* PulsadorAccion lee y ejecuta un String que contiene los siguientes comandos:
 *  Xnn Ynn Znn   Servo a posicion nn
 *  Vnn           Velocidad servo nn
 *  W             Espere hasta que acabe el movimiento de los servos
 *  Hn            Salida digital n a HIGH
 *  Ln            Salida digital n a LOW
 *  I             Espera pulsador
 *  Pnn           Pausa de nn ms (maximo 65535ms)
 *  Rnn           Repetir n veces hasta F
 *  F             Fin de bucle, vuleve a R hasta ejecutar n veces
 *  <...>         Comentario
 */

        // Acciones que se ejecutan tras el reset  
String  Reset     = "H0<servo inicio>V20X25Y50Z180";
        // Acciones que se repiten
String  Acciones  = "<boton>WL0IH0<acciones>H4P1000L4x100y20z50WH3X25Y50Z180WL3<parpadeo espera final>R10H0P500L0P500F";

#define servoXpin 12                      // Definicion de los pines
#define servoYpin 11
#define servoZpin 10
#define pulsadorPin 13

int digitalPins[] = {A0,A1,2,3,4,5,6,7,8,9};

VarSpeedServo servoX;                     // Objetos de la libreria VarSpeedServo
VarSpeedServo servoY; 
VarSpeedServo servoZ; 

String  Operacion;                        // Acciones a ejecutar
int pos;                                  // Posicion en el String Operacion que se esta decodificando
unsigned int parametro;                   // Valor del parametro
int velocidad;                            // Velocidad del servo
int bucle=0;                              // contador para el bucle
int buclepos=0;                           // posicion a la que ha de volver el bucle

void setup() {
  int n;
  
  for (n=0; n < sizeof(digitalPins); n++) { // Inicializa salidas digitales
    pinMode (digitalPins[n], OUTPUT);
    digitalWrite (digitalPins[n], LOW);
  }
  pinMode (pulsadorPin,INPUT_PULLUP);     // Inicializa pin pulsador
  velocidad = 30;                         // velocidad servo por defecto
  pos = 0;                                // inicializa decodificacion al inicio del String
  Operacion = Reset;                      // Primero ejecutamos el String Reset
  Operacion.toUpperCase();                // Pone los comandos en mayusculas
}

void loop() {
  if (pos >= Operacion.length()) {         // Si hemos alcanzado el final del String, cargamos las acciones
    Operacion = Acciones;
    Operacion.toUpperCase();               // Pone los comandos en mayusculas
    pos = 0;                               // inicializa el puntero de decodificacion  
  }
  if (isAlpha (Operacion.charAt(pos)) || isPunct (Operacion.charAt(pos))) {
    switch (Operacion.charAt(pos++)) {
      case 'I':                             // Esperar a que se pulse el boton
        while (digitalRead(pulsadorPin) == HIGH);
        break;
      case 'H':                             // salida digital a HIGH
        parametro = ExtraeNumero(9);
        digitalWrite (digitalPins[parametro],HIGH);
        break;  
      case 'L':                             // salida digital a LOW
        parametro = ExtraeNumero(9);
        digitalWrite (digitalPins[parametro],LOW);
        break;  
      case 'P':                             // Pausa de ms
        parametro = ExtraeNumero(65535);
        delay (parametro);
        break;
      case 'V':                             // velocidad del servo
        velocidad = ExtraeNumero(255);      
        break;
      case 'X':                             // Mueve Servo X
        parametro = ExtraeNumero(180);
        servoX.attach(servoXpin);
        servoX.write(parametro,velocidad);
        break;
      case 'Y':                              // Mueve Servo Y
        parametro = ExtraeNumero(180);
        servoY.attach(servoYpin);
        servoY.write(parametro,velocidad);
        break;  
      case 'Z':                             // Mueve Servo Z
        parametro = ExtraeNumero(180);
        servoZ.attach(servoZpin);
        servoZ.write(parametro,velocidad);
        break;
      case 'W':                             // Espera a que acabe el movimiento de los servos
        servoX.wait();                      // Espera a Servo X
        servoX.detach();
        servoY.wait();                      // Espera a Servo Y
        servoY.detach();
        servoZ.wait();                      // Espera a Servo Z
        servoZ.detach();
        break;
      case 'R':
        bucle = ExtraeNumero(65535);        // Lee el numero de repeticiones
        buclepos = pos;                     // guarda la posicion a volver
        break;
      case 'F':
        bucle--;                            // decrementa contador de bucle
        if (bucle > 0)                      // si quedan bucles por hacer, cambia la posicion de decodificacion
          pos = buclepos;
        break;
      case '<':
        parametro = Operacion.indexOf ('>', pos);
        if (parametro == -1) 
          pos = Operacion.length();
        else
          pos = parametro + 1; 
        break;
    }
  }
  else  {
    ExtraeNumero(0);
  }
}

int ExtraeNumero (unsigned int maximo) {
  String numero;
  unsigned int valor;

  numero = "";                                                              // string para contener el numero
  while (isDigit (Operacion.charAt(pos)) && (pos < Operacion.length()) ) {  // lee caracter mientras sea un numero
    numero.concat (Operacion.charAt(pos++));                                // y lo añade al nuevo string
  }
  valor = min (numero.toInt(), maximo);                                     // convierte el string en numero sin que pase del maximo
  return (valor);                                                           // devuelve el valor obtenido
}


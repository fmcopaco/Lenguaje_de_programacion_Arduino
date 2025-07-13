// Paso a nivel DCC - Paco Cañada 2020
#include <Servo.h>                        // Librerías
#include <NmraDcc.h>

const int pinAltavoz = 3;                 // definición de la conexión de los pines
const int pinLuzDerecha = 4;
const int pinLuzIzquierda = 5;
const int pinLuzMaquinista = 6;
const int pinServo = 7;
const int pinDCC = 2;

const int posicionBarreraSubida = 120;    // limites de movimiento del servo 
const int posicionBarreraBajada = 10;

const int miDireccionDCC = 6;             // Dirección de accesorio DCC a la que respondemos

Servo servoBarrera;                       // Objeto para la librería Servo.h
NmraDcc  Dcc;                             // Objeto para la librería NmraDcc.h

unsigned long tiempoLuzCarretera;
unsigned long tiempoTono;
unsigned long tiempoServo;
unsigned long tiempoLuzMaquinista;

bool faseLuzCarretera;                    // variables para las fases
bool faseLuzMaquinista;
bool faseTono;

bool subirBarrera;                        // variables para las ordenes
bool bajarBarrera;

int posicionServo;                        // variable para el movimiento del servo


void setup() {
pinMode(pinAltavoz, OUTPUT);              // 1- Poner barrera arriba y todo apagado
pinMode(pinLuzDerecha, OUTPUT);
pinMode(pinLuzIzquierda, OUTPUT);
pinMode(pinLuzMaquinista, OUTPUT);

digitalWrite(pinAltavoz, LOW);
digitalWrite(pinLuzDerecha, LOW);
digitalWrite(pinLuzIzquierda, LOW);
digitalWrite(pinLuzMaquinista, LOW);

posicionServo = posicionBarreraSubida;    
servoBarrera.attach (pinServo);           
servoBarrera.write (posicionServo);       
delay (1000);                             
servoBarrera.detach ();                   

Dcc.pin (digitalPinToInterrupt (pinDCC), pinDCC, 1);
Dcc.initAccessoryDecoder (MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0);
}


void loop() {
  bajarBarrera = false;                       // 2- Esperar orden de bajada
  do {
    Dcc.process();
  } while (bajarBarrera == false);

  servoBarrera.attach (pinServo);             // 3- Encender luces carretera, sonido y bajar barrera
  do {
    Dcc.process();
    intermitenciaLuzCarretera();        
    generarTono();                                
    moverBarreraAbajo();                    
  } while (posicionServo > posicionBarreraBajada); 
  servoBarrera.detach ();  

  noTone(pinAltavoz);                         // 4 - Apagar sonido y encender luz maquinista
  digitalWrite(pinLuzMaquinista,HIGH);

  subirBarrera = false;                       // 5- Esperar orden de subida
  do {
    intermitenciaLuzCarretera();  
    intermitenciaLuzMaquinista();      
    Dcc.process();
  } while (subirBarrera == false);

  digitalWrite (pinLuzMaquinista, LOW);       // 6- Apagar luz maquinista y subir barrera
  servoBarrera.attach (pinServo);              
  do {
    Dcc.process();
    intermitenciaLuzCarretera();                  
    moverBarreraArriba();                              
  } while (posicionServo < posicionBarreraSubida); 
  servoBarrera.detach ();            

  digitalWrite(pinLuzDerecha, LOW);           // 7- Apagar luces carretera 
  digitalWrite(pinLuzIzquierda, LOW);
}


void intermitenciaLuzCarretera() {
  if (millis() - tiempoLuzCarretera >= 500) { // 1- Si no han pasado 0,5s (500ms) no hacer nada.
    tiempoLuzCarretera = millis();            // 2- Actualizar tiempo
    if (faseLuzCarretera == false) {          // 3- Si Fase es false: Encender luz derecha y apagar izquierda
      digitalWrite (pinLuzDerecha, HIGH);
      digitalWrite (pinLuzIzquierda, LOW);
    }
    else {                                    // 4- Si Fase es true: Apagar luz derecha y encender izquierda
      digitalWrite (pinLuzDerecha, LOW);
      digitalWrite (pinLuzIzquierda, HIGH);
    }
  faseLuzCarretera = ! faseLuzCarretera;      // 5- Cambiar Fase
  }
}

void generarTono() {
  if (millis() - tiempoTono >= 350) {         // 1- Si no han pasado 350ms no hacer nada.
    tiempoTono = millis();                    // 2- Actualizar tiempo
    if (faseTono == false)                    // 3- Si Fase es false: generar tono de 277 Hz
      tone (pinAltavoz, 277);
    else                                      // 4- Si Fase es true: generar tono de 244Hz 
      tone (pinAltavoz, 244);  
    faseTono = ! faseTono;                    // 5- Cambiar Fase
  }
}

void moverBarreraAbajo() {
  if (millis() - tiempoServo >= 20) {         // 1- Si no han pasado 20ms no hacer nada.
    tiempoServo = millis();                   // 2- Actualizar tiempo
    posicionServo = posicionServo - 1;        // 3- Actualizar posición hacia barrera bajada
    servoBarrera.write (posicionServo);       // 4- Mover el servo a nueva posición
  }
}


void intermitenciaLuzMaquinista() {
  if (millis() - tiempoLuzMaquinista >= 500) {  // 1- Si no han pasado 0,5s (500ms) no hacer nada.
    tiempoLuzMaquinista = millis();             // 2- Actualizar tiempo
    if (faseLuzMaquinista == false)             // 3- Si Fase es 0: Encender luz
      digitalWrite (pinLuzMaquinista, HIGH);
    else                                        // 4- Si Fase es 1: Apagar luz 
      digitalWrite (pinLuzMaquinista, LOW);
    faseLuzMaquinista = ! faseLuzMaquinista;    // 5- Cambiar Fase
  }
}

void moverBarreraArriba(){
  if (millis() - tiempoServo > 20) {          // 1- Si no han pasado 20ms no hacer nada.
    tiempoServo = millis();                   // 2- Actualizar tiempo
    posicionServo = posicionServo + 1;        // 3- Actualizar posición hacia barrera subida
    servoBarrera.write (posicionServo);       // 4- Mover el servo a nueva posición
  }
}

void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) {
  if (Addr == miDireccionDCC) {
    if (OutputPower == 1) {
      if (Direction == 1)
        subirBarrera = true;
     else
        bajarBarrera = true;
    }
  }
}



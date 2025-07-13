// Decodificador de accesorios Multiusos - Paco Cañada 2020
#include <Servo.h>        // Librerias
#include <NmraDcc.h>

#define MAX_PIN    14     // Numero maximo de pins a controlar
#define MAX_SERVO  12     // Numero maximo de servos a controlar (hasta 12)
#define CFG_BASE  112     // Direccion base de la EEPROM para las configuraciones (CV112)
#define ACT_BASE  230     // Direccion base para guardar el estado (CV230)
#define PARPADEO  500     // Parpadeo cada 0,5s
#define MULT_PULSO 20     // Multiplicador para el pulso

#define PIN_ON HIGH
#define PIN_OFF LOW

Servo servo[MAX_SERVO];                         // Objetos
NmraDcc Dcc;

int salida[MAX_PIN]={A0, A1, A2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
const int pinDCC = 2;

struct pinDef {                                 // configuracion de las salidas
  byte tipo;
  int  direccionDCC;
  byte valorA;
  byte valorB;
  byte valorC;
  unsigned long tiempo;                         // tiempo para la salida
  bool activado;                                // estado actual de la salida
  byte posServo;                                // posicion actual del servo
  int  indice;                                  // Indice al array de objetos servo
};

struct pinDef pinSalida[MAX_PIN];

int servosUsados;                               // contador de salidas de servo
unsigned long tiempoFlash;                      // variables para flash
byte Fase;
int selPin=0, selDireccion, selTipo, selValorA, selValorB, selValorC;  // variables para configurar salida

enum tipoSalida {FIJA, FLASH, SERVO, PULSO};    // valores para tipo
enum valorPar {ROJO, VERDE};                    // valores para Par
enum faseFlash  {FASE_A, FASE_B};               // valores para fase

void setup() {
  for (int n=0; n< MAX_PIN; n++) {              // inicializar salidas
    pinMode (salida[n],OUTPUT);
    digitalWrite (salida[n],PIN_OFF);
  }
  Serial.begin (115200);
  Serial.println (F("Decodificador de accesorios Multiusos by Paco Cañada\n"));
  resetConfiguracion ();                        // comprueba CV8 y resetea si es necesario
  Dcc.pin (digitalPinToInterrupt (pinDCC), pinDCC, 1);
  Dcc.initAccessoryDecoder (MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0);
  leeConfiguracion ();                          // lee configuracion de los pines
  tiempoFlash = millis();                       // tiempo para temporizador de parpadeo
  Serial.println (F("? para ver comandos"));
}

void loop() {
  if (millis() - tiempoFlash > PARPADEO) {    // temporizador para el parpadeo
    tiempoFlash = millis();
    Fase = (Fase == FASE_A) ? FASE_B : FASE_A;  // cambia la fase
    }
  for (int n=0; n < MAX_PIN; n++) {
    Dcc.process();
    switch (pinSalida[n].tipo) {
      case FIJA:
        salidaFija(n);
        break;
      case FLASH:
        salidaFlash(n);
        break;
      case PULSO:
        salidaPulso(n);
        break;
      case SERVO:
        salidaServo(n);
        break;
    }
  }
  if (Serial.available() >0) {
    int c = Serial.read();                      // lee caracter comando
    switch (c) {
      case 'I':                                 // I: Informacion
        printConfiguracionSalidas ();
        break;
      case 'C':                                 // C: Configurar salida
        configuraSalida();
        break;
      case '-':                                 // -: ultima salida configurada a ROJO
        activaSalida (selPin, ROJO);
        break;
      case '+':                                 // +: ultima salida configurada a VERDE
        activaSalida (selPin, VERDE);
        break;
      case '\n':                                // descartar
      case '\r':
        break;
      case '?':                                 // ?: Ayuda
      default:
        Serial.println (F("\nIntroduzca uno de estos comandos"));
        Serial.println (F("I: Informacion salidas"));
        Serial.println (F("C: Cambia salida"));
        Serial.print   (F("-: Posicion ROJO. Salida "));
        Serial.println (selPin);
        Serial.print   (F("+: Posicion VERDE. Salida "));
        Serial.println (selPin);
        Serial.println (F("?: Esta ayuda"));
        break;
    }
  }
}

void leeConfigSalida (int numSalida) {          // Lee la configuracion de una salida desde la EEPROM
  int n = (numSalida * 6) + CFG_BASE;
  pinSalida[numSalida].tipo = EEPROM.read(n++);
  pinSalida[numSalida]. direccionDCC = (EEPROM.read(n++) << 8) + EEPROM.read(n++);
  pinSalida[numSalida].valorA = EEPROM.read(n++);
  pinSalida[numSalida].valorB = EEPROM.read(n++);
  pinSalida[numSalida].valorC = EEPROM.read(n);
}

void leeConfiguracion () {   // Lee todas las configuraciones y desactiva salidas por defecto
  servosUsados = 0;                               // contador de servos
  for (int n=0 ; n < MAX_PIN; n++) {
    leeConfigSalida (n);                          // lee configuracion desde la EEPROM
    pinSalida[n].tiempo = 0;                      // resetea tiempo
    if (EEPROM.read (n + ACT_BASE) == 0) {          // recupera el ultimo estado guardado
      pinSalida[n].activado = false;
      pinSalida[n].posServo = pinSalida[n].valorA;  // posicion por defecto del servo
    }
    else {
      pinSalida[n].activado = true;
      pinSalida[n].posServo = pinSalida[n].valorB;  // posicion por defecto del servo
    }
    if ((pinSalida[n].tipo == SERVO) && (servosUsados < MAX_SERVO)) { // si es un servo
      pinSalida[n].indice = servosUsados;         // asignamos un indice 
      servo[servosUsados].write (pinSalida[n].posServo); // y una posicion
      servosUsados++;                             // incrementamos el contador de servos
    }
    else {
      pinSalida[n].indice = 0;                    // no es un servo o hay demasiados
    }
  }
}

void escribeConfigSalida (int numSalida) {
  int n = (numSalida * 6) + CFG_BASE;
  EEPROM.update (n++, pinSalida[numSalida].tipo);
  EEPROM.update (n++, highByte(pinSalida[numSalida]. direccionDCC));
  EEPROM.update (n++, lowByte(pinSalida[numSalida]. direccionDCC));
  EEPROM.update (n++, pinSalida[numSalida].valorA);
  EEPROM.update (n++, pinSalida[numSalida].valorB);
  EEPROM.update (n, pinSalida[numSalida].valorC);
}

void resetConfiguracion () {
  if (EEPROM.read (CV_MANUFACTURER_ID) != MAN_ID_DIY) {   // Si CV8 no es 13 reseteamos configuracion
    Serial.println (F("Reseteando configuracion\n"));
    for (int n=0; n < MAX_PIN; n++) {
      pinSalida[n].tipo = FIJA;
      pinSalida[n].direccionDCC = (n >> 1) + 1;           // la misma direccion para dos salidas
      pinSalida[n].valorA = (n & 0x01) ? VERDE : ROJO;    // Pares: ROJO, Impares: VERDE
      pinSalida[n].valorB = 0;
      pinSalida[n].valorC = 0;
      escribeConfigSalida (n);                            // Lo guardamos en la EEPROM
      EEPROM.update (n + ACT_BASE, 0);                    // guardamos ultimo estado como desactivado
    }
  EEPROM.update (CV_MANUFACTURER_ID, MAN_ID_DIY);
  }
}

void activaSalida (int numSalida, byte Direction) {
  switch (pinSalida[numSalida].tipo) {               // activar segun tipo
    case FIJA:
    case FLASH:
      pinSalida[numSalida].activado = (pinSalida[numSalida].valorA == Direction) ? true : false;
      break;
    case PULSO:
      pinSalida[numSalida].activado = (pinSalida[numSalida].valorA == Direction) ? true : false;
      pinSalida[numSalida].tiempo = millis();       // inicializa tiempo para temporizador
      break;
    case SERVO:
      pinSalida[numSalida].activado = (Direction == ROJO) ? false : true;
      servo[pinSalida[numSalida].indice].attach (salida[numSalida]);   // conecta pin al objeto servo correspondiente
      pinSalida[numSalida].tiempo = millis();       // inicializa tiempo para temporizador
      break;
  }
  byte estado = (pinSalida[numSalida].activado) ? 1 : 0; // guarda estado actual
  if (pinSalida[numSalida].tipo == PULSO) 
    estado=0;
  EEPROM.update(numSalida + ACT_BASE, estado);
}

void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) { 
  if (OutputPower == 1) {                           // solo comandos de activacion
    for (int n= 0; n < MAX_PIN; n++) {
      if (Addr == pinSalida[n].direccionDCC) {      // Si coincide la direccion
        activaSalida (n, Direction);                // activa salida segun tipo
      }
    }
//    Serial.print (F("Desvio: "));
//    Serial.println (Addr);
  }
}

void notifyCVChange( uint16_t CV, uint8_t Value) {
  resetConfiguracion();    // Si cambia una CV comprobamos si hay que resetear
}

void salidaFija (int numSalida) {
  if (pinSalida[numSalida].activado  == true)       // pone la salida segun activado
    digitalWrite (salida[numSalida],PIN_ON);
  else
    digitalWrite (salida[numSalida],PIN_OFF);
}

void salidaPulso (int numSalida) {
  if (pinSalida[numSalida].activado  == true)       // si esta activado comprueba tiempo
    if (millis() - pinSalida[numSalida].tiempo > (pinSalida[numSalida].valorB * MULT_PULSO)) // Entre 20ms y 5,1s
      pinSalida[numSalida].activado  = false;
  salidaFija (numSalida);
}

void salidaFlash (int numSalida) {
  if (pinSalida[numSalida].activado  == true) {     // si esta activado comprueba la fase
    if (Fase == pinSalida[numSalida].valorB)
      digitalWrite (salida[numSalida],PIN_ON);      // si coincide la fase se enciende
    else
       digitalWrite (salida[numSalida],PIN_OFF);    // si no coincide se apaga
  }
  else
    digitalWrite (salida[numSalida],PIN_OFF);       // si esta desactivado se apaga
}

void salidaServo (int numSalida) {
  byte posFinal = (pinSalida[numSalida].activado == true) ? pinSalida[numSalida].valorB : pinSalida[numSalida].valorA;
  if (millis() - pinSalida[numSalida].tiempo > (pinSalida[numSalida].valorC)) {  // espera segun velocidad
    pinSalida[numSalida].tiempo = millis();
    if (pinSalida[numSalida].posServo == posFinal) {
      if (servo[pinSalida[numSalida].indice].attached()) {  // si esta en posicion y conectado lo desconectamos
        while (digitalRead(salida[numSalida])== HIGH);      // Esperamos a que acabe el pulso  
        servo[pinSalida[numSalida].indice].detach();        // lo desconectamos
        digitalWrite(salida[numSalida],LOW);                // nos aseguramos de que esta desconectado
      }
    }
    else {
      if (posFinal > pinSalida[numSalida].posServo)       // actualiza posicion
        pinSalida[numSalida].posServo++;
      else
        pinSalida[numSalida].posServo--;
      servo[pinSalida[numSalida].indice].write(pinSalida[numSalida].posServo); // mueve hacia posicion final
    }
  }
}

void printEstado (int numSalida, bool verEstado) {
  if (verEstado) {
    Serial.print (F(" estado: "));
    if (pinSalida[numSalida].activado)
      Serial.println(F("ON"));
    else
      Serial.println(F("OFF"));
  }
  else
    Serial.println("");
}

void printSalida (int numSalida, bool verEstado) {
   int valor;
   char dato[80];                                     // Tamaño suficiente para una linea
  sprintf (dato,"Salida %2d - Direccion:%5d", numSalida, pinSalida[numSalida].direccionDCC); // "Salida xx - Direccion: yyyyy"
  Serial.print (dato);
  if (pinSalida[numSalida].tipo != SERVO) {           // si no es servo imprime ROJO o VERDE segun par
    if (pinSalida[numSalida].valorA == ROJO)
      Serial.print (F(" ROJO  - "));
    else
      Serial.print (F(" VERDE - "));
  }
  switch (pinSalida[numSalida].tipo) {                // imprime configuracion segun tipo
    case FIJA:
      Serial.print(F("FIJA"));
        printEstado(numSalida, verEstado);
        break;
    case FLASH:
      if (pinSalida[numSalida].valorB == FASE_A)
        Serial.print (F("FLASH FASE A"));
      else
        Serial.print (F("FLASH FASE B"));
        printEstado(numSalida, verEstado);
      break;
    case PULSO:
      Serial.print (F("PULSO de duracion "));
      valor = (int)pinSalida[numSalida].valorB * MULT_PULSO;
      Serial.print (valor);
      Serial.println(F("ms"));
      break;
    case SERVO:
      sprintf (dato," SERVO - ROJO: %d VERDE: %d velocidad %dms/grado angulo actual %d", pinSalida[numSalida].valorA, pinSalida[numSalida].valorB, pinSalida[numSalida].valorC, pinSalida[numSalida].posServo);
      Serial.println (dato);
      break;
  }
}

void printConfiguracionSalidas () {
  Serial.println (F("\nConfiguracion de las salidas\n"));
  for (int n=0; n < MAX_PIN; n++) {
    printSalida (n, true);
    Dcc.process();                                    // ir procesando comandos DCC
  }
  if (servosUsados > MAX_SERVO) {
    Serial.print (F("\nATENCION: Hay demasiados servos. Maximo "));
    Serial.println (MAX_SERVO);
  }
}

int entradaSerie (int maximo) {
  int n=0,c=0,valor=0;
  do {
    if (Serial.available()>0) {                       // espera a que llegue un caracter procesando DCC
      c = Serial.read();
      if (isDigit(c)) {
        valor = (valor * 10) + (c - '0');             // el digito leido se añade a valor
        n++;
      }
    }
    else {
      Dcc.process();
    }
  } while ((c != '\r') || (n == 0));                // hasta que se pulse enter
  valor = constrain (valor, 0, maximo);             // lo ajustamos
  return (valor);
}

void desactivaPulsos () {
  for (int n=0; n< MAX_PIN; n++) {                  // desconecta las salidas PULSO
    if (pinSalida[n].tipo == PULSO) {
      pinSalida[n].activado = false;
      digitalWrite (salida[n],PIN_OFF);
    }
  }
}
  
void configuraSalida() {
  desactivaPulsos();                                // desconecta las salidas pulso por seguridad
  Serial.print (F("\nIntroduzca salida a configurar entre 0 y "));
  Serial.println (MAX_PIN - 1);  
  selPin = entradaSerie (MAX_PIN - 1);
  Serial.println (F("\nConfiguracion actual:"));
  printSalida (selPin, true);                       // Imprimimos configuracion pin seleccionado
  Serial.println (F("\nIntroduzca Direccion DCC (1-2044, 0: Salir)"));
  selDireccion = entradaSerie(2044);
  if (selDireccion == 0) {
    Serial.println (F("\nCambios cancelados"));
    return;
  }
  Serial.print (F("Nueva direccion: "));
  Serial.println (selDireccion);
  Serial.println (F("\nIntroduzca tipo de salida (0:FIJA, 1:FLASH, 2:SERVO, 3:PULSO)"));
  selTipo = entradaSerie(3);
  switch (selTipo) {
    case FIJA:
      configuraFIJA();
      break;
    case FLASH:
      configuraFLASH();
      break;
    case PULSO:
      configuraPULSO();
      break;
    case SERVO:
      configuraSERVO();
      break;
  }
  pinSalida[selPin].tipo = selTipo;
  pinSalida[selPin].direccionDCC = selDireccion;
  pinSalida[selPin].valorA = selValorA;
  pinSalida[selPin].valorB = selValorB;
  pinSalida[selPin].valorC = selValorC;
  Serial.println (F("\nNueva configuracion:"));
  printSalida (selPin, false);                      // Imprimimos configuracion pin seleccionado
  Serial.println (F("\nCorrecto? (0:NO, 1:SI)"));
  if (entradaSerie(1)) {
    escribeConfigSalida (selPin);                   // correcto: guardar en EEPROM
    leeConfiguracion ();                            // lee configuracion de todos los pines
    Serial.println (F("\nCambios guardados"));
  }
  else {
    leeConfigSalida(selPin);                        // incorrecto: leer configuracion desde EEPROM
    Serial.println (F("\nCambios cancelados"));
  }
}


void entradaPar (int tipoSalida) {
  Serial.print (F("\nIntroduzca posicion de activacion de la salida "));
  switch (tipoSalida) {
    case FIJA:
      Serial.print (F("FIJA"));
      break;
    case FLASH:
      Serial.print (F("FLASH"));
      break;
    case PULSO:
      Serial.print (F("PULSO"));
      break;
  }
  Serial.println (F(" (0:ROJO, 1:VERDE)"));
  selValorA = entradaSerie(1);
  Serial.print (F("Nueva posicion: "));
  if (selValorA == ROJO)
    Serial.println (F("ROJO"));
  else
    Serial.println (F("VERDE"));
}

void configuraFIJA() {
  entradaPar (FIJA);
  selValorB = 0;
  selValorC = 0;
}

void configuraFLASH() {
  entradaPar (FLASH);
  Serial.println (F("\nIntroduzca Fase (0:FASE A, 1:FASE B)"));
  selValorB = entradaSerie(1);
  Serial.print (F("Nueva fase: "));
  if (selValorA == FASE_A)
    Serial.println (F("FASE A"));
  else
    Serial.println (F("FASE B"));
  selValorC = 0;
}

void configuraPULSO() {
  entradaPar (PULSO);
  Serial.print (F("\nIntroduzca duracion pulso en ms (1-"));
  int valor = 255 * MULT_PULSO;
  Serial.print (valor);
  Serial.println (F(")"));
  selValorB = entradaSerie(valor) / MULT_PULSO;
  selValorC = 0;
}

void configuraSERVO() {
  Serial.println (F("\nIntroduzca angulo posicion ROJO (0-180) "));
  selValorA = entradaSerie(180);
  Serial.print   (F("\nNuevo angulo (ROJO): "));
  Serial.println (selValorA);
  Serial.println (F("\nIntroduzca angulo posicion VERDE (0-180)"));
  selValorB = entradaSerie(180);
  Serial.print   (F("\nNuevo angulo (VERDE): "));
  Serial.println (selValorB);
  Serial.println (F("\nIntroduzca velocidad (0-255) Usual: 5..50ms por grado"));
  selValorC = entradaSerie(255);
}


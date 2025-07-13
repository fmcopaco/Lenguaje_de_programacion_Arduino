// Lanza Rutas XpressNet - Paco Cañada 2020
// Arduino Pro Micro (32U4)
#include <EEPROM.h>
#include <Keypad.h>
#include <XpressNet.h>                                      // ATENCION: Parchear libreria para el Arduino Pro Micro
                                                            // añadir:  || defined(__AVR_ATmega32U4__)
#define MAX_ACCESORIOS 8                                    // Maximo de accesorios a enviar por cada tecla
#define INICIO_EEPROM 4                                     // Direccion a partir de donde se guardan los datos
#define NUM_ACC_COLA 32                                     // Maximo numero de desvios en cola
#define ACC_TIEMPO_ON 100                                   // tiempo de activacion de accesorios en ms
#define ACC_TIEMPO_CDU 500                                  // tiempo de espera para recarga unidad descarga capacitiva (CDU) en ms
#define FILAS 4                                             // Filas y Columnas de la matriz
#define COLUMNAS 3

#if ((MAX_ACCESORIOS * FILAS * COLUMNAS) > 510)             // Comprobar que no se exceda capacidad EEPROM (1K)
  #error "Los datos exceden la capacidad de la EEPROM"
#endif

char keys[FILAS][COLUMNAS] = {                              // array dos dimensiones
  {1,2,3},
  {4,5,6},
  {7,8,9},
  {10,11,12}
};
byte filaPins[FILAS]   = {5, 4, 3, 2};                      // pins de las filas del teclado
byte colPins[COLUMNAS] = {10, 16, 14};                      // pins de las columnas del teclado
Keypad keypad = Keypad( makeKeymap(keys), filaPins, colPins, FILAS, COLUMNAS);

const int totalRutas = FILAS * COLUMNAS;                    // numero de rutas
unsigned int  MemoriaAccesorios[totalRutas][MAX_ACCESORIOS];// Memoria para accesorios
unsigned int accesoriosFIFO [NUM_ACC_COLA];                 // cola FIFO
int ultimoLlegar;                                           // indice cabeza cola
int primeroSalir;                                           // indice final cola
int enCola;                                                 // numero de accesorios en cola
unsigned long tiempoAccesorio;                              // envio de accesorios
bool recargando, esperaAccesorio;
unsigned int accEnviado;

XpressNetClass XpressNet;                                   // Objeto XpressNet

const int pinTXRX = 9;                                      // pin del MAX485
const int miDireccionXpressnet = 24;                        // direcion Xpressnet
const int RXLED = 17;                                       // LED RX indica estado de la central (Pro Micro: 17, Mega: 13)

byte csStatus;                                              // estado de la central

void setup() {
  Serial.begin (115200);                                    
  pinMode (RXLED,OUTPUT);                               // LED RX
  leeEEPROM();                                          // Lee rutas de la EEPROM
  borraFIFO();                                          // borra la cola de envio de accesorios
  XpressNet.start (miDireccionXpressnet, pinTXRX);      // inicializamos libreria
  csStatus = csTrackVoltageOff;                         // Suponemos que la central esta sin tension en via
  XpressNet.getPower();                                 // Pedimos el estado actual de la central
}

void loop() {
  char tecla;
  XpressNet.receive();                                // llama a la libreria para que vaya recibiendo paquetes
  enviaColaAccesorios();                              // va enviando los accesorios de la cola
  tecla = keypad.getKey();                            // lee teclado
  if ((tecla != NO_KEY) && (csStatus == csNormal))    // si se pulsa tecla en estado normal
    enviaRuta (tecla);                                // se envia ruta a la cola
  if (Serial.available() >0)                          // si el usuario escribe un comando
    entradaComandos();                                // se ejecuta el comando
}

void entradaComandos () {
  int c = Serial.read();                            // lee caracter comando
  switch (c) {
    case 'L':                                       // L: Listado de rutas
      Serial.println (F("\nLanza Rutas Xpressnet by Paco Cañada 2020"));
      listadoRutas();
      break;
    case 'E':                                       // E: Entrada de nuevas rutas
      entradaRutas();
      break;
    case 'G':                                       // G: Guardar rutas en EEPROM
      Serial.println (F("\nGuardar todas las Rutas en EEPROM. Esta seguro? - 0: No, 1: Si")); 
      c = entradaSerie (1);
      if (c == 1){
        guardaEEPROM();
      }
      else
        Serial.println (F("Guardar rutas cancelado"));
      break;
    case '\n':                                      // descartar
    case '\r':      
      break;
    case '?':                                       // ?: Ayuda
    default:
      Serial.println (F("\nIntroduzca uno de estos comandos"));
      Serial.println (F("L: Listado Rutas"));
      Serial.println (F("E: Entrada Rutas"));
      Serial.println (F("G: Guardar Rutas en EEPROM"));
      Serial.println (F("?: Esta ayuda"));
      break;  
    }
}

void leeEEPROM () {                                         // Leer memoria de rutas de la EEPROM
  int direccionEE, n, i;
  unsigned int dato;
  if ((EEPROM.read(0) != 'O') || (EEPROM.read(1) != 'K')) { // Si las dos primeras posiciones no es 'OK'
    for (n=0; n< totalRutas; n++)                           // Borra la memoria de rutas
      for (i=0; i< MAX_ACCESORIOS; i++)
        MemoriaAccesorios[n][i] = 0xFFFF;                   // 0xFFFF es accesorio no definido
  }
  else {
    direccionEE = INICIO_EEPROM;
    for (n=0; n< totalRutas; n++)                           // Lee la memoria de rutas de la EEPROM
      for (i=0; i< MAX_ACCESORIOS; i++) {
        dato = (EEPROM.read (direccionEE++) << 8) + EEPROM.read (direccionEE++);
        MemoriaAccesorios[n][i] = dato;
      }
  }
}  
  
void guardaEEPROM () {
  int direccionEE, n, i;
  direccionEE = INICIO_EEPROM;
  for (n=0; n< totalRutas; n++)                              // Guarda la memoria de rutas de la EEPROM
    for (i=0; i< MAX_ACCESORIOS; i++) {
      EEPROM.update (direccionEE++, highByte(MemoriaAccesorios[n][i]));
      EEPROM.update (direccionEE++,  lowByte(MemoriaAccesorios[n][i]));
    }
  EEPROM.update (0, 'O');                                    // Marca 'OK' de datos validos
  EEPROM.update (1, 'K');
  Serial.println (F("Rutas guardadas en EEPROM"));
}

void listadoRutas () {
  for (int n=0; n< totalRutas; n++) {                        // Lista cada ruta
    listaUnaRuta (n);
  }
}

void listaUnaRuta (int ruta) {
  int i;
  unsigned int dato;
  Serial.print (F("Ruta "));
  Serial.print (ruta + 1);
  Serial.print (F(": < "));
  for (i=0; i< MAX_ACCESORIOS; i++) {
    dato = MemoriaAccesorios[ruta][i];
    if (dato != 0xFFFF) {                               // 0xFFFF es accesorio no definido
      Serial.print ((dato & 0x03FF) + 1);               // accesorio 1 a 1024
      if (bitRead (dato,14))
          Serial.print (F("C "));                       // Cambia posicion si bit 14 activado
      else {
        if (bitRead (dato,15))
          Serial.print (F("V "));                       // posicion Rojo/Verde segun bit 15
        else
          Serial.print (F("R "));
      }  
    }
  }
  Serial.println (F(">"));                              // fin ruta
}

int entradaSerie (int maximo) {
  unsigned int n=0,c=0,valor=0;
  do {
    if (Serial.available()>0) {                           // espera a que llegue un caracter procesando Xpressnet
      c = Serial.read();
      if (isDigit(c)) {
        valor = (valor * 10) + (c - '0');                 // el digito leido se añade a valor
        n++;
      }
    }
    XpressNet.receive();                                  // llama a la libreria para que vaya recibiendo paquetes
  } while ((c != '\r') || (n == 0));                      // hasta que se pulse enter
  valor = constrain (valor, 0, maximo);                   // lo ajustamos
  return (valor);
}

void entradaRutas () {
  int ruta, accesorio, posicion, n;
  Serial.print (F("Introduzca Ruta a modificar: (0: Cancelar, 1-"));
  Serial.print (totalRutas);
  Serial.println (F(")"));
  ruta = entradaSerie (totalRutas);
  if (ruta==0)
    return;
  Serial.print (F("Defina los "));
  Serial.print (MAX_ACCESORIOS);
  Serial.print (F(" accesorios de la ruta "));
  Serial.println (ruta);
  for (n=0; n< MAX_ACCESORIOS; n++) {
    Serial.print (F("Introduzca accesorio "));
    Serial.print (n + 1);
    Serial.println(F(" (0: No definir, 1-1024)"));
    accesorio = entradaSerie (1024);
    if (accesorio > 0) {
      Serial.println (F("Posicion - 0: Rojo, 1: Verde, 2: Cambia"));
      posicion = entradaSerie (2);
      switch (posicion) {
        case 0:
          MemoriaAccesorios[ruta - 1][n] = accesorio - 1;             // Rojo: bit 15 a 0
          break;
        case 1:
          MemoriaAccesorios[ruta - 1][n] = (accesorio - 1) | bit(15); // Verde: bit 15 a 1
          break;
        case 2:
          MemoriaAccesorios[ruta - 1][n] = (accesorio - 1) | bit(14); // Cambio: bit 14 a 1
          break;
      }
    }
    else
      MemoriaAccesorios[ruta - 1][n] = 0xFFFF;                      // no definido
  }
  listaUnaRuta (ruta - 1);
}

void borraFIFO () {
  ultimoLlegar=0;                                       // indice cabeza cola
  primeroSalir=0;                                       // indice final cola
  enCola=0;                                             // numero de accesorios en cola
}

unsigned int  readFIFO () {
  primeroSalir = (primeroSalir + 1 ) % NUM_ACC_COLA;    //avanza puntero
  enCola--;
  return (accesoriosFIFO[primeroSalir]);
}

void writeFIFO (unsigned int instruccion) {
  ultimoLlegar = (ultimoLlegar + 1 ) % NUM_ACC_COLA;    // avanza puntero
  enCola++;
  accesoriosFIFO[ultimoLlegar] = instruccion;           // lo guarda en la cola
}

void enviaRuta (char tecla) {
  unsigned int accesorio;
  for (int n=0; n < MAX_ACCESORIOS ; n++) {            
    while (enCola >= NUM_ACC_COLA -2)                   // Si la cola esta llena espera a que haya hueco
      enviaColaAccesorios();
    accesorio = MemoriaAccesorios[tecla-1][n];
    if (accesorio != 0xFFFF) {                          // Si esta definido el accesorio, lo pone en la cola
      if (bitRead (accesorio,14)) {
        accesorio ^= bit(15);                           // Cambio de posicion si bit 14 activado
        MemoriaAccesorios[tecla-1][n] = accesorio;
      }
      writeFIFO (accesorio);
    }
  }
}

void enviaColaAccesorios () {
  if (recargando) {                                     // esperando a que se recargue la CDU.
    if (millis() - tiempoAccesorio > ACC_TIEMPO_CDU) {
      recargando = false;
    }
  }
  else {
    if (esperaAccesorio) {
      if (millis() - tiempoAccesorio > ACC_TIEMPO_ON) {
        enviaAccesorio (accEnviado, false);             // Envia desconectar accesorio
        tiempoAccesorio = millis();
        esperaAccesorio = false;
        recargando = true;
      }
    }
    else {
      if ((enCola > 0) && (csStatus == csNormal)) {     // Envia accesorio de la cola en estado normal
        accEnviado = readFIFO();
        enviaAccesorio (accEnviado, true);              // Envia conectar accesorio
        tiempoAccesorio = millis();
        esperaAccesorio = true;
      }
    }
  }
}

void enviaAccesorio (unsigned int instruccion, bool activa) { // Accessory Decoder operation request (AAAAAAAA 1000DBBD)
  byte  adrH, pos;
  adrH = highByte(instruccion) & 0x03;          
  pos = 0;                                                    // pos = 0000A00P   A: activo/inactivo, P: rojo/verde
  if (bitRead(instruccion,15))   
    pos |= 0x01;
  if (activa)                    
    pos |= 0x08;
  XpressNet.setTrntPos (adrH, lowByte(instruccion), pos); 
  Serial.println (instruccion,HEX);
}

void notifyXNetPower (uint8_t State) {                  // La libreria llama a esta funcion cuando cambia estado de la central
  csStatus = State;
}

void notifyXNetStatus (uint8_t State) {                 // Funcion de la libreria para mostrar estado en un LED
digitalWrite(RXLED, State);                             // usamos el LED del Arduino
}


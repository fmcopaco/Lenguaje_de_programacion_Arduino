// Mando XpressNet para panel - Paco Cañada 2020
#include <XpressNet.h>
#include <TimerOne.h>

const int pinOutA = 3;            // pines del encoder
const int pinOutB = 4;
const int pinSwitch = 5;
const int pinPulsador = 10;       // pin del pulsador

const int pinData = 6;            // DIO  pines pantalla 7 segmentos chip 74HC595
const int pinLatch = 7;           // RCLK
const int pinClock = 8;           // SCLK

const int pinTXRX = 9;            // pin de control del MAX485
const int miDireccionXpressnet = 25;  // Direccion del dispositvo en el bus Xpressnet

XpressNetClass XpressNet;

int miLocomotora;                 // Datos de la locomotora que estoy controlando
uint8_t miVelocidad;
uint8_t miSentido;
uint8_t miPasos;
union funciones {
  uint8_t Xpress[4];              // array para funciones, F0F4, F5F12, F13F20 y F21F28
  unsigned long Bits;             // long para acceder a los bits
}  miFuncion;

uint8_t estadoCentral;            // Estado de la central
bool pideInfoLoco;                // pedir informacion de la locomotora 
bool pideInfoFunc;                // pedir indormacion del estado de las funciones 
bool pedirFuncPendiente;          // falta pedir informacion del estado de las funciones

#define PASOS14 0                 // pasos segun los valores de la libreria
#define PASOS28 2
#define PASOS128 3

#define FUNC_OFF 0                // control de funciones según los valores de la librería
#define FUNC_ON 1
#define FUNC_CHANGE 2

int outA, outB, copiaOutA, copiaOutB; // valor de las entradas del encoder que usados por ISR
volatile byte encoderValor;       // estos valores del encoder son compartidos por ISR y programa
volatile byte encoderMax;
volatile bool encoderCambio;

unsigned long tiempoBotones;      // para lectura de los botones
const unsigned long antiRebote = 50;  // temporizador antirebote
int estadoPulsador;
int estadoSwitch;
bool pulsadorOn;
bool switchOn;

byte digitos[4];                  // patrones a mostrar en cada digito. 
uint8_t enciendeDP = -1;          // digito a mostrar con el punto decimal encendido
volatile bool actualizaPantalla;  // Compartido con interrupcion
volatile byte contadorISR = 0;    // contador de la interrupcion

uint8_t pantallaMenu = 0;         // menu seleccionado: 0:Locomotora, 1:Funciones
uint8_t pantallaDigito = 0;       // digito seleccionado en seleccion de locomotora
uint8_t pantallaFuncion = 0;      // valor de la funcion seleccionada

#define UNIDADES 0                // posicion en el array digitos[]
#define DECENAS 1
#define CENTENAS 2
#define MILES 3

#define CH_ESPACIO 10             // Posicion de los caracteres en la tabla de patrones
#define CH_F 11
#define CH_L 12
#define CH_t 13
#define CH_o 14
#define CH_P 15
#define CH_r 16
#define CH_c 17
#define CH_u 18
#define CH_n 19
#define CH_ADELANTE 20
#define CH_ATRAS 21
#define CH_BOLA 22

static const byte patron[] =      // 7 segmentos valores para 0..9, espacio, F,L,t,o,P,r,c,u,n,º
    {
//  patron caracter  |   0 = segmento on
 // dpGFEDCBA        |   1 = segmento off
    B11000000,  //0  |        A
    B11111001,  //1  |      -----
    B10100100,  //2  |   F |     | B
    B10110000,  //3  |     |  G  |
    B10011001,  //4  |      -----
    B10010010,  //5  |   E |     | C
    B10000010,  //6  |     |     |
    B11111000,  //7  |      -----   dp
    B10000000,  //8  |        D
    B10011000,  //9  |
    B11111111,  // espacio
    B10001110,  // F 
    B11000111,  // L - 
    B10000111,  // t
    B10100011,  // o -
    B10001100,  // P
    B10101111,  // r -
    B10100111,  // c -
    B11100011,  // u -
    B10101011,  // n -
    B11011111,  // ' -
    B11101111,  // , -
    B10011100   // º -
    };

enum modos {NORMAL, MENU_SEL, MENU_LOCO, MENU_FUNC, ERROR_XN};  // modo en el que estoy
  
modos miModo;

void setup() {
  pinMode (pinOutA,INPUT_PULLUP);         // entradas
  pinMode (pinOutB, INPUT_PULLUP);
  pinMode (pinSwitch, INPUT_PULLUP);
  pinMode (pinPulsador, INPUT_PULLUP);
  copiaOutA = digitalRead (pinOutA); 
  copiaOutB = digitalRead (pinOutB);
  attachInterrupt (digitalPinToInterrupt (pinOutA), encoderISR, CHANGE);
  pinMode (pinData,OUTPUT);  
  pinMode (pinLatch,OUTPUT);
  pinMode (pinClock,OUTPUT);
  Timer1.initialize(6250);                // Dispara cada 6.25 ms (40 veces/segundo por digito)
  Timer1.attachInterrupt(pantallaISR);    // Activa la interrupcion
  miLocomotora = 3;                       // Inicialmente controlamos la locomotora 3 a 28 pasos
  miPasos = PASOS28;
  encoderMax = 28;
  encoderValor = 0;
  pideInfoLoco = false;
  pideInfoFunc = false;
  pedirFuncPendiente = false;
  miModo = ERROR_XN;                      // Mostramos '0003' hasta que la central responda
  mostrarNumero (miLocomotora, 4);  
  XpressNet.start (miDireccionXpressnet, pinTXRX);  // inicializamos libreria
  XpressNet.getPower();                   // Pedimos el estado actual de la central
}

void loop() {
  XpressNet.receive();                    // llama a la libreria para que vaya recibiendo paquetes
  if (pideInfoLoco)
    pideInformacionLocomotora();
  if (pideInfoFunc)
    pideInformacionFunciones();
  leePulsadores();                        // comprueba el estado de los pulsadores
  if (actualizaPantalla)                  // cada 6,25ms. Interrupcion TimerOne
    enviaPantalla();
  if (pulsadorOn)                         // si se pulsa Pulsador
    controlPulsador();  
  if (switchOn)                           // si se pulsa boton encoder
    controlSwitch();
  if (encoderCambio)                      // si se mueve encoder. Interrupcion pin
    controlEncoder();
}

void encoderISR () {                      // interrupción encoder
  outA = digitalRead (pinOutA);
  outB = digitalRead (pinOutB);
  if (outA != copiaOutA) {                   // evitamos rebotes
    copiaOutA = outA;
    if ( outB != copiaOutB) {
    copiaOutB = outB;  
    if (outA  == outB)                    // comprueba sentido de giro
      encoderValor = (encoderValor >= encoderMax) ? encoderMax : ++encoderValor; // CW, hasta maximo
    else
      encoderValor = (encoderValor <=0) ? 0 : --encoderValor;  // CCW, hasta 0
    encoderCambio = true;
    }
  }
}

void pantallaISR() {                    // interrupcion pantalla
  contadorISR++;
  if (contadorISR == 4)                 // cuenta de 0 a 3
    contadorISR = 0;                    // empieza con los miles, luego centenas, decenas y unidades
  actualizaPantalla = true;
}

void enviaPantalla() {   
  int segmentos, caracter;
  byte contador;
  actualizaPantalla = false;
  contador = contadorISR;               // lo copiamos por si cambia ya que es compartido con ISR
  caracter = digitos[contador];         // caracter a mostrar en el digito
  segmentos = patron[caracter];         // patron de segmentos segun el caracter.
  if (enciendeDP == contador)           // si este digito necesita el punto decimal se enciende
    bitClear (segmentos, 7);
  digitalWrite (pinLatch,LOW);          // inicia transmision
  shiftOut (pinData, pinClock, MSBFIRST, segmentos);              // patron segmentos
  shiftOut (pinData, pinClock, MSBFIRST, 1 << (contador));    // activa digito
  digitalWrite (pinLatch, HIGH);
}

void mostrarNumero (int valor, int cifras) {
  digitos[UNIDADES] = valor % 10;       // guardamos el resto de la division en el array
  if (cifras >1)
    digitos[DECENAS] = (valor / 10) % 10;
  if (cifras >2)
    digitos[CENTENAS] = (valor / 100) % 10;
  if (cifras >3)
    digitos[MILES] = (valor / 1000) % 10;
}

uint8_t highByteCV17 (int direccion) {
  if (direccion > 99)                   // si es direccion larga coloca bits altos a uno
    direccion |= 0xC000;
  return (highByte(direccion));
}

void pideInformacionLocomotora () {     // informacion de locomotora.
  pideInfoLoco = false;
  pedirFuncPendiente = true;
  XpressNet.getLocoInfo (highByteCV17(miLocomotora), lowByte(miLocomotora));
}

void pideInformacionFunciones () {     // informacion de funciones.
  pideInfoFunc = false;
  XpressNet.getLocoFunc (highByteCV17(miLocomotora), lowByte(miLocomotora));
}

byte velocidadMaxima (uint8_t pasos) {
  switch (pasos) {                     // devuelve el numero de paso mas alto para el encoder
    case PASOS14:
      return (14);
      break;
    case PASOS28:
      return (28);
      break;
  case PASOS128:
    return (126);
    break;
  }
}

void nuevaVelocidad(uint8_t valor) {    // enviar nueva velocidad como requiere libreria
  int velocidad;
  miVelocidad = valor;
  switch (miPasos) {                    //  nos saltamos el paso de parada de emergencia
    case PASOS14:
    case PASOS128:
    if (valor>0)
      velocidad = valor + 1;
    else
      velocidad = 0;
    break;
  case PASOS28:                         // ponemos los 28 pasos en su formato
    if (valor > 0)                      // pasamos velocidad entre 0 y 31 (0  0  0  S4 S3 S2 S1 S0)
      velocidad = valor + 3;            // nos saltamos pasos parada de emergencia
    else
      velocidad = 0;
    bitWrite (velocidad, 5, bitRead (velocidad,0)); // copia bit 0        (0  0  S0 S4 S3 S2 S1 S0)
    velocidad >>= 1;                    // coloca en su sitio             (0  0  0  S0 S4 S3 S2 S1)
    break;
  }
  if (miSentido)                        // añade el sentido
    velocidad |= 0x80;
  XpressNet.setLocoDrive (highByteCV17(miLocomotora), lowByte(miLocomotora), miPasos, velocidad);
}

void cambiaFuncion (int numFuncion) {   // activamos o desactivamos la funcion segun su estado
  bool estado = bitRead (miFuncion.Bits, numFuncion);
  if (estado) {
    XpressNet.setLocoFunc (highByteCV17(miLocomotora), lowByte(miLocomotora), FUNC_OFF, numFuncion);
    bitClear (miFuncion.Bits, numFuncion);
  }
  else {
    XpressNet.setLocoFunc (highByteCV17(miLocomotora), lowByte(miLocomotora), FUNC_ON, numFuncion);
    bitSet (miFuncion.Bits, numFuncion);
  }
}

void notifyXNetPower (uint8_t State) {  // La libreria llama a esta funcion cuando cambia estado de la central 
  estadoCentral = State;
  enciendeDP = -1;  // Apagamos punto decimal
  switch (estadoCentral) {
    case csNormal:                      // Al entrar en modo normal mostramos la direccion de nuestra locomotora
      miModo = NORMAL;                  // pedimos informacion de la locomotora por si ha cambiado
      mostrarNumero (miLocomotora,4);
      encoderMax = velocidadMaxima (miPasos);
      encoderValor = miVelocidad;
      pideInfoLoco = true;
      break;
    case csShortCircuit:                // Corto circuito - OFF
    case csTrackVoltageOff:             // Sin tension en via - OFF
      digitos[MILES]= 0;
      digitos[CENTENAS]= CH_F;
      digitos[DECENAS]= CH_F;
      digitos[UNIDADES]= CH_ESPACIO;
      miModo = ERROR_XN;
      break;
    case csEmergencyStop:               // Parada emergencia - StoP
      digitos[MILES]= 5;
      digitos[CENTENAS]= CH_t;
      digitos[DECENAS]= CH_o;
      digitos[UNIDADES]= CH_P;
      miModo = ERROR_XN;
      break;
    case csServiceMode:                 // Programacion en modo servicio - Pro
      digitos[MILES]= CH_P;
      digitos[CENTENAS]= CH_r;
      digitos[DECENAS]= CH_o;
      digitos[UNIDADES]= CH_ESPACIO;
      miModo = ERROR_XN;
      break;
  }
}

void notifyLokAll(uint8_t Adr_High, uint8_t Adr_Low, boolean Busy, uint8_t Steps, uint8_t Speed, 
                  uint8_t Direction, uint8_t F0, uint8_t F1, uint8_t F2, uint8_t F3, boolean Req) {
  int Loco = ((Adr_High << 8) & 0x3F) + (Adr_Low); 
  if (Loco != miLocomotora)             // si no es nuestra locomotora no hace nada
    return;
  miSentido = Direction;                // sentido
  miPasos = Steps;                      // pasos
  switch (Steps) {                      // ajusta mi Velocidad segun los pasos
    case PASOS14:
    case PASOS128:
      miVelocidad = Speed - 1;          // eliminamos paso de stop de emergencia
      break;
    case PASOS28:                       // Mover bits a su sitio      (0  0  0  S0 S4 S3 S2 S1)
      Speed <<=1;                       // Hacemos sitio para el bit  (0  0  S0 S4 S3 S2 S1 0 )
      bitWrite (Speed, 0, bitRead(Speed,5));  // colocamos bit        (0  0  S0 S4 S3 S2 S1 S0)
      Speed &= 0x1F;                    // Limpiamos                  (0  0  0  S4 S3 S2 S1 S0);
      miVelocidad = Speed - 3;          // eliminamos pasos de stop de emergencia 
      break;
  }
  if (miVelocidad > 127)                // Corregimos ajuste (unsigned)
    miVelocidad = 0;
  if (pedirFuncPendiente) {             // Comprobamos si falta pedir informacion de las funciones
    pedirFuncPendiente = false;         
    pideInfoFunc = true;
  }
  else {
    miFuncion.Xpress[3] = F3;             // F3: F28F27F26F25F24F23F22F21
    miFuncion.Xpress[2] = F2;             // F2: F20F19F18F17F16F15F14F13
    miFuncion.Xpress[1] = F1;             // F1: F12F11F10 F9 F8 F7 F6 F5
  // ponemos F0: x  x  x  F0 F4 F3 F2 F1 asi F4 F3 F2 F1 F0 x x x para reordenarlos
    miFuncion.Xpress[0] = F0 <<4;         // F4 F3 F2 F1 0 0 0 0
    if (bitRead(F0, 4))  
      bitSet (miFuncion.Xpress[0], 3);    // F4 F3 F2 F1 F0 0 0 0
    miFuncion.Bits = miFuncion.Bits >> 3; // desplazamos los 29 bits a su sitio: 000F28F27...F0
  }
  nuevaPantalla();
}

void notifyXNetStatus (uint8_t State) {   // Funcion de la libreria para mostrar estado en un LED
  digitalWrite(LED_BUILTIN, State);       // usamos el LED del Arduino (Arduino Uno/Nano: pin 13)
}

void nuevaPantalla() {                    // mostrar datos en pantalla segun mi modo
  switch (miModo) {
    case NORMAL:                          // mostrar sentido y velocidad
      mostrarNumero (miVelocidad,3);  
      if (miSentido)
        digitos[MILES] = CH_ADELANTE;
      else
        digitos[MILES] = CH_ATRAS;
    encoderMax = velocidadMaxima(miPasos);
    encoderValor = miVelocidad;
    break;
  case MENU_SEL:                          // mostrar 'Loco' o 'Func'
    if (pantallaMenu == MENU_FUNC) {      // menu seleccionado: Locomotora o Funciones
      digitos[MILES]=CH_F;
      digitos[CENTENAS]=CH_u;
      digitos[DECENAS]=CH_n;
      digitos[UNIDADES]=CH_c;
    }
    else {
      digitos[MILES]=CH_L;
      digitos[CENTENAS]=CH_o;
      digitos[DECENAS]=CH_c;
      digitos[UNIDADES]=CH_o;
    }
    encoderMax = 1;
    encoderValor = (pantallaMenu == MENU_FUNC) ? 1 : 0;
    break;
  case MENU_LOCO:                         // mostrar numero de locomotora y punto decimal
    mostrarNumero (miLocomotora,4);
    enciendeDP = pantallaDigito;          // Enciende el punto decimal
    encoderMax = 9;
    encoderValor = digitos[pantallaDigito];  // en digitos[] ya esta el valor calculado
    break;
  case MENU_FUNC:                       // mostrar funcion y estado
    mostrarNumero (pantallaFuncion,2);
    digitos[CENTENAS] = CH_F;
    if (bitRead (miFuncion.Bits, pantallaFuncion)) // vemos si la funcion esta activa
      digitos[MILES] = CH_BOLA;         // mostrar funcion activa
    else 
      digitos[MILES] = CH_ESPACIO;      // mostrar funcion apagada
    encoderMax = 28;
    encoderValor = pantallaFuncion;
    break;
  case ERROR_XN:                        // dejar la pantalla igual
     break;
  }
}

void leePulsadores () {
  int entradaBoton;
  if (millis() - tiempoBotones > antiRebote) {  // lee cada cierto tiempo
    tiempoBotones = millis();
    entradaBoton = digitalRead (pinPulsador);   // comprueba cambio en boton pulsador
    if (estadoPulsador != entradaBoton) {
      estadoPulsador = entradaBoton;
    if (estadoPulsador == LOW)
      pulsadorOn = true;
    }
    entradaBoton = digitalRead (pinSwitch); // comprueba cambio en boton del encoder
    if (estadoSwitch != entradaBoton) {
      estadoSwitch = entradaBoton;
    if (estadoSwitch == LOW)
      switchOn = true;
    }
  }
}

void controlPulsador () {                   // acciones cuando se pulsa el boton segun el modo
  pulsadorOn = false;
  switch (miModo) {
    case NORMAL:                            // en modo normal pasa al menu de seleccion
      miModo = MENU_SEL;
      break;
    case MENU_LOCO:                         // en menu loco pasa a normal,pide informacion de la nueva locomotora
      miModo = NORMAL;
      enciendeDP= -1;                       // apaga punto decimal
      pideInfoLoco = true;
      return;                               // no muestra velocidad, espera a que llegue la informacion
      break;
    case MENU_SEL:                          // en los otros menu pasa a normal
    case MENU_FUNC:
      miModo = NORMAL;
      break;
    case ERROR_XN:                          // en error no hace nada
      break;
  }
  nuevaPantalla();                          // actualiza datos en pantalla
}

void controlSwitch () {                     // acciones cuando se pulsa el boton del encoder
  switchOn = false;
  switch (miModo) {
    case NORMAL:                            // en modo normal paramos o cambiamos de sentido parados
      if (miVelocidad == 0)
        miSentido = ! miSentido;            // cambiamos sentido si estamos parados
      nuevaVelocidad (0);                   // paramos, con el sentido adecuado.
      break;
    case MENU_SEL:                          // en selecion menu, entra en el menu correspondiente
      if (pantallaMenu == MENU_FUNC)        // menu seleccionado: Locomotora, Funciones
        miModo = MENU_FUNC;
      else {
        miModo = MENU_LOCO;
        pantallaDigito = UNIDADES;          // el primer digito a cambiar son las unidades
      }
      break;
    case MENU_LOCO:                         // en menu loco pasa al siguiente digito.
      switch (pantallaDigito) {
        case UNIDADES:
          pantallaDigito = DECENAS;
          break;
        case DECENAS:
          pantallaDigito = CENTENAS;
          break;
        case CENTENAS:
          pantallaDigito = MILES;
          break;
        case MILES:
          pantallaDigito = UNIDADES;
          break;
        }
      break;
    case MENU_FUNC:                         // en menu funcion activa/desactiva funcion
      cambiaFuncion (pantallaFuncion);
      break;
    case ERROR_XN:                          // en error reactiva la central si no estamos en modo de programacion
      if (estadoCentral != csServiceMode)
        XpressNet.setPower(csNormal);
      break;
  }
  nuevaPantalla();                          // actualiza datos en pantalla
}

void controlEncoder () {                    // acciones cuando se gira el encoder
  encoderCambio=false;
  switch (miModo) {
    case NORMAL:                            // en modo normal cambia la velocidad
      nuevaVelocidad (encoderValor);
      break;
    case MENU_LOCO:                         // en menu loco actualiza digito
      digitos[pantallaDigito] = encoderValor;
      miLocomotora = (digitos[MILES] * 1000) + (digitos[CENTENAS] * 100) + (digitos[DECENAS] * 10) + digitos[UNIDADES];
      break;
    case MENU_SEL:                          // en menu seleccion muestra la opcion
      if (encoderValor == 1)
        pantallaMenu = MENU_FUNC;
      else
        pantallaMenu = MENU_LOCO;
      break;
    case MENU_FUNC:                         // en menu funcion selecciona nueva funcion
      pantallaFuncion = encoderValor;
      break;
    case ERROR_XN:                          // en error no hace nada
      break;
  }
  nuevaPantalla();                          // actualiza datos en pantalla
}


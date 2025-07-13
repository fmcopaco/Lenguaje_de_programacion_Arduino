// TFT_TCO - Paco Cañada 2022 -- https://usuaris.tinet.cat/fmco/

// Descomentar la siguente linea sino funciona con la libreria Adafruit_TFTLCD.h o se desconoce el chip

#define TIPO_DESCONOCIDO

#ifdef TIPO_DESCONOCIDO
#define ROTACION_PANTALLA 1     // MCUFRIEND
#else
#define ROTACION_PANTALLA 3     // Adafruit
#endif

//====  Librerias y variables  ================================================================================

#include <Adafruit_GFX.h>                       // Libreria de graficos v1.5.0
#ifdef TIPO_DESCONOCIDO
#include <MCUFRIEND_kbv.h>                      // Libreria especifica para el Hardware v2.9.9
#else
#include <Adafruit_TFTLCD.h>                    // Libreria especifica para el Hardware v1.0.3
#endif
#include <TouchScreen.h>                        // Libreria para panel tactil v1.1.3
#include <XpressNet.h>                          // Libreria Xpressnet v2.1.0
#include "iconos.h"                             // iconos 32x32
#include "circuito.h"                           // Definicion del circuito


// Definicion de las conexiones de la pantalla

#define LCD_CS    A3                            // Chip Select
#define LCD_CD    A2                            // Command/Data
#define LCD_WR    A1                            // TFT Write
#define LCD_RD    A0                            // TFT Read
#define LCD_RESET A4                            // TFT Reset

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Colour definitions for 64K colour mode
// Bits 0..4 ->   Blue 0..4
// Bits 5..10 ->  Green 0..5
// Bits 11..15 -> Red 0..4
// Assign human-readable names to some common 16-bit color values:

#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define VIOLET      0x9199
#define BROWN       0x8200
#define PINK        0xF97F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
//#define ORANGE      0xFCA0
#define LIME        0x87E0
#define GREENYELLOW 0xAFE5
#define AQUA        0x5D1C
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define SILVER      0xA510
#define GOLD        0xA508
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF

#ifdef TIPO_DESCONOCIDO
MCUFRIEND_kbv tft;                                // usar MCUFRIEND para pantallas no soportadas por Adafruit
#else
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);   // Usar Adafruit que es mas ligera
#endif

unsigned int ID;                                  // ID de la pantalla
// Usar TouchScreen_Calibr_native para obtener los valores. Libreria MCUFRIEND
/*
  const int XP = 9, XM = A3, YP = A2, YM = 8;         // ILI9488 480x320
  const int TS_LEFT = 145, TS_RT = 913, TS_TOP = 72, TS_BOT = 956;
*/
/*
  const int XP = 7, XM = A1, YP = A2, YM = 6;         // ILI9341 320x240 2nd
  const int TS_LEFT = 995, TS_RT = 147, TS_TOP = 913, TS_BOT = 165;
*/

const int XP = 8, XM = A2, YP = A3, YM = 9;         //240x320 ILI9341 1st
const int TS_LEFT = 250, TS_RT = 905, TS_TOP = 175, TS_BOT = 907;


// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//Minimum and maximum pressure to sense the touch
#define MINPRESSURE 10
#define MAXPRESSURE 1000

int X;                                            // Variables que almacenaran la coordenada
int Y;                                            // X, Y donde presionemos y la variable Z
int Z;                                            // almacenara la presion realizada

bool pulsacionDetectada = false;                  // pulsacion detectada en panel tactil
unsigned long tiempoRebote;                       // tiempo de espera para evitar rebotes

byte pasoConfiguracion;                           // paso configuracion panel tactil

const int pinTXRX = A5;                           // pin del MAX485
const int miDireccionXpressnet = 23;              // direcion Xpressnet

byte csStatus;                                    // Estado de la central
byte RS[128];                                     // Datos de los RS

XpressNetClass XpressNet;

const unsigned char* iconos[] = {
  icon_viaV, icon_viaH, icon_curvaNE, icon_curvaSE, icon_curvaSO, icon_curvaNO, icon_curvaNE_SO, icon_curvaNO_SE,
  icon_topeN, icon_topeE, icon_topeS, icon_topeO, icon_vacio, icon_ruta, icon_conector,
  icon_semaforoN, icon_semaforoE, icon_semaforoS, icon_semaforoO, icon_PaNsub, icon_PaNbaj,
};

enum posDesvio {NO_MOVIDO, DESVIADO, RECTO, INVALIDO};  // DESVIADO: ROJO  -  > links     RECTO:    VERDE  +  < rechts

byte pantallaActual;
int posTexto;


unsigned int cambiosFIFO[32];                     // cola FIFO para cambios en pantalla
byte cambioIn;                                    // indice cabeza cola
byte cambioOut;                                   // indice final cola
byte enColaCambios;                               // numero de accesorios en cola

unsigned int accesoriosFIFO[32];                  // cola FIFO para mover accesorios
byte accesorioIn;                                 // indice cabeza cola
byte accesorioOut;                                // indice final cola
byte enColaAccesorios;                            // numero de accesorios en cola

unsigned long tiempoAccesorio;                    // envio de accesorios
unsigned int accEnviado;
bool recargando, esperaAccesorio;
unsigned long tiempoRuta = millis();              // envio de rutas

unsigned long tiempoInfo;                         // tiempo para pedir informacion
byte pantallaInfo, iconInfo;                      // contadores para pedir informacion
bool pideInformacion;                             // pidiendo informacion iconos

//====  Programa principal  ================================================================================

void setup() {
  ID = tft.readID();
  //  Controladores disponibles:  0x9325, 0x9328, 0x7575, 0x9341, 0x8357
  tft.begin(ID);                                  // Iniciamos el LCD especificando el controlador ILI9341 (0x9341).
  // Rotation: 0: portrait-USB top right, 2: portrait-USB bottom left, 1:landscape-USB bottom right, 3: landscape-USB top left.
  tft.setRotation(ROTACION_PANTALLA);             // Establecemos la pantalla Horizontal
  for (int i = 0; i < 128; i++)                   // borra estado contactos
    RS[i] = 0;
  csStatus = csTrackVoltageOff;                   // Suponemos que la central esta sin tension en via
  pantallaActual = 0;
  pintaPantalla();                                // pinta pantalla inicial
  borraAccesoriosFIFO();                          // borra colas
  borraCambiosFIFO();
  XpressNet.start(miDireccionXpressnet, pinTXRX); // Inicializamos bus Xpressnet
  XpressNet.getPower();                           // Pedimos el estado actual de la central
  refrescaInformacion();                          // Pedimos informacion de la posicion de los iconos
}


void loop() {
  byte iconoPulsado, tipo, posicion;
  unsigned int direccion;
  int col, fila;

  XpressNet.receive();                            // recibimos paquetes por Xpressnet
  actualizaCambiosIconos();                       // actualiza cambio en iconos de la pantalla actual
  XpressNet.receive();                            // recibimos paquetes por Xpressnet
  enviaColaAccesorios();                          // envia accesorios de la cola
  XpressNet.receive();                            // recibimos paquetes por Xpressnet
  if (pideInformacion)                            // comprueba si hay informacion pendiente
    buscaInformacion();                           // pide informacion posicion iconos
  XpressNet.receive();                            // recibimos paquetes por Xpressnet

  TSPoint p = ts.getPoint();                      // Realizamos lectura de las coordenadas panel tactil
  pinMode(XM, OUTPUT);                            // La librería utiliza estos pines como entrada y salida
  pinMode(YP, OUTPUT);                            // por lo que es necesario declararlos como salida justo despues de realizar una lectura de coordenadas.

  X = map(p.y, TS_TOP, TS_BOT, 0, tft.width());   // Mapeamos los valores analogicos leidos del panel tactil (0-1023)
  Y = map(p.x, TS_RT, TS_LEFT, 0, tft.height());
  Z = p.z;

  if (pulsacionDetectada) {
    if (millis() - tiempoRebote > 200)
      pulsacionDetectada = false;
  }
  else {
    if (Z > MINPRESSURE && Z < MAXPRESSURE) {     // si se pulsa la pantalla
      col = constrain (X, 0, tft.width() - 1) / 32;
      fila = constrain (Y, 0, tft.height() - 1) / 32;

      iconoPulsado = buscaIconoPulsado (col, fila);
      if (iconoPulsado != NO_EXISTE) {            // comprueba si hemos pulsado sobre un icono
        tipo = Pantalla[pantallaActual][iconoPulsado].tipo;
        if (tipo < ICON_SEMAFORO_N) {             // via, ruta o conector
          if (tipo == ICON_CONECTOR) {            // si es un conector cambiamos de pantalla
            pantallaActual = lowByte(Pantalla[pantallaActual][iconoPulsado].direccion);
            pintaPantalla();
          }
          if (tipo == ICON_RUTA) {                // si es una ruta enviamos la ruta
            if (millis() - tiempoRuta > ((tiempoAccesorioON + tiempoCDU) * 3)) {  // Esperar un tiempo prudencial entre rutas
              pintaIcono(Pantalla[pantallaActual][iconoPulsado], RECTO);
              direccion = lowByte(Pantalla[pantallaActual][iconoPulsado].direccion);
              enviaRuta(direccion);
              pintaIcono(Pantalla[pantallaActual][iconoPulsado], NO_MOVIDO);
            }
          }
        }
        else {                                    // semaforo o desvio
          direccion = Pantalla[pantallaActual][iconoPulsado].direccion;
          posicion = buscaPosicionActual (direccion);   // busca posicion actual
          if (posicion == RECTO)                  // cambiar a la otra posicion
            bitSet(direccion, 15);                // bit 15 activo indica posicion Desviado
          writeAccesoriosFIFO(direccion);
          pulsacionDetectada = true;
          tiempoRebote = millis();
        }
      }
    }
  }
}


//====  Iconos y pantallas  ================================================================================

void pintaIcono (Simbolo simbolo, byte posicion) {
  unsigned int color, colorLuz, x, y, offsetOn, offsetOff;
  byte icono, iconoR, iconoD;
  x = simbolo.posX * 32;
  y = simbolo.posY * 32;
  icono = simbolo.tipo;
  if (icono < ICON_DESVIO_V_NE) {                 // No son desvios
    color = COLOR_VIA;                            // color por defecto
    switch (posicion) {                           // calculamos luz en caso semaforos
      case DESVIADO:
        offsetOn = 8;
        offsetOff = 0;
        colorLuz = RED;
        break;
      case RECTO:
        offsetOn = 0;
        offsetOff = 8;
        colorLuz = GREEN;
        break;
      default:
        offsetOn = 0;
        offsetOff = 8;
        colorLuz = COLOR_FONDO;
        break;
    }
    switch (icono) {                              // casos especiales, RUTA, CONECTOR, SEMAFOROS
      case ICON_PAN_S:
        color = bitRead(posicion - 1, 1) ? COLOR_DESVIO_NO_POS : COLOR_PAN;
        tft.fillRect(x + 2, y + 4, 28, 21, COLOR_FONDO);   // borra posicion anterior
        if (posicion == DESVIADO)
          icono = ICON_PAN_B;
        break;
      case ICON_PAN_B:
        color = bitRead(posicion - 1, 1) ? COLOR_DESVIO_NO_POS : COLOR_PAN;
        tft.fillRect(x + 2, y + 4, 28, 21, COLOR_FONDO);   // borra posicion anterior
        if (posicion == DESVIADO)
          icono = ICON_PAN_S;
        break;
      case ICON_TEXTO:
        pintaTexto(lowByte(simbolo.direccion));   // pintamos texto y salimos
        return;
        break;
      case ICON_RUTA:
        color = (posicion == NO_MOVIDO) ? COLOR_RUTA : COLOR_VIA;
        break;
      case ICON_CONECTOR:
        color = COLOR_CONECTOR;
        break;
      case ICON_SEMAFORO_N:
        tft.fillRect(x + 22, y + 4 + offsetOff, 6, 7, COLOR_FONDO);   // borra luz
        XpressNet.receive();                                          // recibimos paquetes por Xpressnet
        tft.fillRect(x + 22, y + 4 + offsetOn, 6, 7, colorLuz);       // pinta Luz
        break;
      case ICON_SEMAFORO_E:
        tft.fillRect(x + 21 - offsetOff, y + 22, 7, 6, COLOR_FONDO);
        XpressNet.receive();                                          // recibimos paquetes por Xpressnet
        tft.fillRect(x + 21 - offsetOn, y + 22, 7, 6, colorLuz);
        break;
      case ICON_SEMAFORO_S:
        tft.fillRect(x + 4, y + 21 - offsetOff, 6, 7, COLOR_FONDO);
        XpressNet.receive();                                          // recibimos paquetes por Xpressnet
        tft.fillRect(x + 4, y + 21 - offsetOn, 6, 7, colorLuz);
        break;
      case ICON_SEMAFORO_O:
        tft.fillRect(x + 4 + offsetOff, y + 4, 7, 6, COLOR_FONDO);
        XpressNet.receive();                                          // recibimos paquetes por Xpressnet
        tft.fillRect(x + 4 + offsetOn, y + 4, 7, 6, colorLuz);
        break;
    }
    XpressNet.receive();                          // recibimos paquetes por Xpressnet
    tft.drawBitmap(x, y, iconos[icono], 32, 32, color);
  }
  else {                                          // Son desvios
    if (icono < ICON_DESVIO_H_NE) {
      iconoR = ICON_VIA_V;                        // Tramo recto vertical
      iconoD = icono - ICON_DESVIO_V_NE + ICON_CURVA_NE; // Tramo orientacion desviado (0..3) + base
    }
    else {
      iconoR = ICON_VIA_H;                        // Tramo recto horizontal
      iconoD = icono - ICON_DESVIO_H_NE + ICON_CURVA_NE; // Tramo orientacion desviado (0..3) + base
    }
    switch (posicion) {
      case RECTO:
        tft.drawBitmap(x, y, iconos[iconoD], 32, 32, COLOR_DESVIO_NO_POS);    // recto
        XpressNet.receive();                                                  // recibimos paquetes por Xpressnet
        tft.drawBitmap(x, y, iconos[iconoR], 32, 32, COLOR_DESVIO_POS);
        break;
      case DESVIADO:
        tft.drawBitmap(x, y, iconos[iconoR], 32, 32, COLOR_DESVIO_NO_POS);    // desviado
        XpressNet.receive();                                                  // recibimos paquetes por Xpressnet
        tft.drawBitmap(x, y, iconos[iconoD], 32, 32, COLOR_DESVIO_POS);
        break;
      default:
        tft.drawBitmap(x, y, iconos[iconoR], 32, 32, COLOR_DESVIO_NO_POS);    // no movido o invalido
        XpressNet.receive();                                                  // recibimos paquetes por Xpressnet
        tft.drawBitmap(x, y, iconos[iconoD], 32, 32, COLOR_DESVIO_NO_POS);
        break;
    }
  }
}


void pintaTitulo() {
  int n, x, y;
  y = tft.height() - 16;
  tft.fillRect(0, y, tft.width(), 16, (csStatus == csNormal) ? COLOR_FONDO_TITULO : COLOR_FONDO_TIT_STOP);
  n = strlen(nombrePantallas[pantallaActual]);
  x = (tft.width() - (n * 12)) / 2;
  tft.setCursor(x, y);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TITULO);
  tft.print(nombrePantallas[pantallaActual]);
}


void pintaPantalla() {
  unsigned int direccion, n;
  byte posicion;
  tft.fillScreen(COLOR_FONDO);
  pintaTitulo();
  XpressNet.receive();                            // recibimos paquetes por Xpressnet
  for (n = 0; n < numIconosPantalla[pantallaActual]; n++) {
    direccion = Pantalla[pantallaActual][n].direccion;
    if (Pantalla[pantallaActual][n].tipo < ICON_SEMAFORO_N)
      posicion = NO_MOVIDO;
    else
      posicion = buscaPosicionActual(direccion);
    if (bitRead(direccion, 14))                   // comprobamos si hay que invertirlo
      posicion ^= B11;
    pintaIcono (Pantalla[pantallaActual][n], posicion);
    XpressNet.receive();                          // recibimos paquetes por Xpressnet
  }
}


int extraeNumero (char *texto,  int maximo) {
  int valor;
  valor = 0;
  while (isDigit (texto[posTexto]))               // mientras sea un numero
    valor = (valor * 10) + (texto[posTexto++] - '0');   // lo añadimos al valor
  valor = min (valor, maximo);                    // no ha de ser superior al maximo
  return (valor);
}


void pintaTexto (int numTexto) {
  int x, y;
  char caracter;
  x = 0;
  y = 0;
  posTexto = 0;
  tft.setTextColor(COLOR_TEXTO);
  tft.setTextSize(1);
  while (caracter = textos[numTexto][posTexto++]) {
    switch (caracter) {                           // caracteres poco usados: ! # $ % & ; , @ ^ _ | ~
      case '@':                                   // coordenada X
        x = extraeNumero(textos[numTexto], tft.width() - 1);
        tft.setCursor(x, y);
        break;
      case ',':                                   // coordenada Y
        y = extraeNumero(textos[numTexto], tft.height() - 1);
        tft.setCursor(x, y);
      case ';':                                   // separador
        break;
      default:                                    // texto
        tft.print(caracter);
        break;
    }
  }
}

//====  Busqueda datos  ================================================================================

byte buscaIconoPulsado (int col, int fila) {      // busca icono pulsado en la pantalla actual
  byte x, y, n;
  for (n = 0; n < numIconosPantalla[pantallaActual]; n++) {
    x = Pantalla[pantallaActual][n].posX;
    y = Pantalla[pantallaActual][n].posY;
    if ((x == col) && (y == fila))
      return n;
  }
  return NO_EXISTE;                               // no encontrado
}


byte buscaPosicionActual(unsigned int direccion) {  // busca posicion actual de un accesorio
  byte modulo, desplazar, posicion;
  modulo = direccion >> 2;                        // calculamos modulo RS en el que esta la direccion
  desplazar = (direccion & B11) * 2;             // y la posicion de sus bits de estado
  posicion = (RS[modulo] >> desplazar) & B11;     // leemos posicion actual
  return posicion;
}


void  refrescaInformacion() {
  tiempoInfo = millis();                          // inicializamos variables para pedir datos
  pantallaInfo = 0;
  iconInfo = 0;
  pideInformacion = true;                         // hay informacion pendiente de recibir
}


void buscaInformacion() {
  unsigned int direccion;

  if (millis() - tiempoInfo > tiempoPosicionInfo) {   // espera un tiempo entre peticiones
    direccion =  Pantalla[pantallaInfo][iconInfo].direccion & 0x0FFF;
    if (direccion < 1024) {                       // solo accesorios
      tiempoInfo = millis();                      // resetea tiempo espera
      XpressNet.getTrntInfo (highByte(direccion), lowByte(direccion));  // pide informacion a la central
    }
    iconInfo++;                                   // siguiente icono de la pantalla
    if (iconInfo == numIconosPantalla[pantallaInfo]) {    // informacion pantalla completa
      iconInfo = 0;                               // iconos proxima pantalla
      pantallaInfo++;
      if (pantallaInfo == NUM_PANTALLAS) {        // todas las pantallas
        pantallaInfo = 0;
        pideInformacion = false;                  // ya hemos recibido toda la informacion
      }
    }
  }
}


//====  Callback Xpressnet  ================================================================================

void notifyTrnt(uint8_t Adr_High, uint8_t Adr_Low, uint8_t Pos) {   // Llamado por la libreria cuando hay cambios en accesorios
  byte modulo, mascara, desplazar, nuevaPos;
  unsigned int direccion;

  direccion = (Adr_High << 8) + Adr_Low;          // calculamos modulo RS en el que esta la direccion
  modulo = direccion >> 2;
  desplazar = (direccion & 0x03) * 2;             // la mascara ver sus bits de estado
  mascara = B11 << desplazar;
  nuevaPos = (Pos & B11) << desplazar;            // los datos de la nueva posicion
  if ((RS[modulo] ^ nuevaPos) & mascara) {        // si cambia la posicion respecto a la guardada
    RS[modulo] &= ~mascara;                       // borramos estado actual
    RS[modulo] |= nuevaPos;                       // ponemos nuevo estado
    writeCambiosFIFO (direccion);                 // guardamos direccion en la FIFO para actualizar pantalla
  }
}


void notifyXNetPower (uint8_t State) {            // La libreria llama a esta funcion cuando cambia estado de la central
  csStatus = State;                               // guardamos el estado
  pintaTitulo();                                  // Cambiamos el color de fondo del titulo segun el estado
}

//====  Cola accesorios  ================================================================================

void borraAccesoriosFIFO () {
  accesorioIn = 0;                                // indice cabeza cola
  accesorioOut = 0;                               // indice final cola
  enColaAccesorios = 0;                           // numero de accesorios en cola
  recargando = false;                             // CDU cargada
  esperaAccesorio = false;                        // ningun accesorio activado enviado
}


unsigned int  readAccesoriosFIFO () {
  accesorioOut = (accesorioOut + 1 ) & 0x1F;      //avanza puntero circular
  enColaAccesorios--;
  return (accesoriosFIFO[accesorioOut]);
}


void writeAccesoriosFIFO (unsigned int accesorio) {
  accesorioIn = (accesorioIn + 1 ) & 0x1F;        // avanza puntero circular
  enColaAccesorios++;
  accesoriosFIFO[accesorioIn] = accesorio;        // lo guarda en la cola
}


void enviaColaAccesorios() {
  byte adrH, adrL, pos;
  if (recargando) {                                // esperando a que se recargue la CDU.
    if (millis() - tiempoAccesorio > (tiempoCDU)) {
      recargando = false;
    }
  }
  else {
    if (esperaAccesorio) {
      if (millis() - tiempoAccesorio > (tiempoAccesorioON)) {
        adrH = highByte (accEnviado) & 0x03;
        adrL = lowByte  (accEnviado);
        pos = bitRead(accEnviado, 15) ? B0000 : B0001;  // bit 15 activo indica posicion Desviado
        XpressNet.setTrntPos (adrH, adrL, pos);     // A00P Envia desconectar accesorio
        tiempoAccesorio = millis();
        esperaAccesorio = false;
        recargando = true;
      }
    }
    else {
      if (enColaAccesorios > 0) {
        accEnviado = readAccesoriosFIFO();
        adrH = highByte (accEnviado) & 0x03;
        adrL = lowByte  (accEnviado);
        pos = bitRead(accEnviado, 15) ? B1000 : B1001;  // bit 15 activo indica posicion Desviado
        XpressNet.setTrntPos (adrH, adrL, pos);     // A00P
        tiempoAccesorio = millis();
        esperaAccesorio = true;
      }
    }
  }
}


void enviaRuta(unsigned int numRuta) {
  unsigned int pos, direccion;
  char caracter;
  pos = 0;
  direccion = 0;
  while (caracter = rutas[numRuta][pos]) {
    switch (caracter) {
      case 'D':
        bitSet(direccion, 15);                      // desviado bit 15 activo
      case 'R':
        direccion--;                                // Las direcciones empiezan en 0
        writeAccesoriosFIFO(direccion);             // guarda en cola
        direccion = 0;                              // resetea calculo direccion
        pos++;
        break;
      default:
        direccion = (direccion * 10) + (rutas[numRuta][pos++] - '0');  // añade valor a direccion
        break;
    }
  }
  tiempoRuta = millis();                            // timeout para rutas
}


//====  Cola cambios iconos  ================================================================================

void borraCambiosFIFO () {
  cambioIn = 0;                                     // indice cabeza cola
  cambioOut = 0;                                    // indice final cola
  enColaCambios = 0;                                // numero de accesorios en cola
}


unsigned int  readCambiosFIFO () {
  cambioOut = (cambioOut + 1 ) & 0x1F;              //avanza puntero circular
  enColaCambios--;
  return (cambiosFIFO[cambioOut]);
}


void writeCambiosFIFO (unsigned int direccion) {
  cambioIn = (cambioIn + 1 ) & 0x1F;                // avanza puntero circular
  enColaCambios++;
  cambiosFIFO[cambioIn] = direccion;                // lo guarda en la cola
}


void actualizaCambiosIconos() {
  unsigned int direccion;
  byte posicion, n;
  if (enColaCambios > 0) {
    direccion = readCambiosFIFO();
    posicion = buscaPosicionActual(direccion);
    for (n = 0; n < numIconosPantalla[pantallaActual]; n++) {   // Puede haber varios con la misma direccion
      if (direccion == (Pantalla[pantallaActual][n].direccion & 0x0FFF)) {    // 0..1023
        if (bitRead(Pantalla[pantallaActual][n].direccion, 14))
          pintaIcono (Pantalla[pantallaActual][n], posicion ^ B11);
        else
          pintaIcono (Pantalla[pantallaActual][n], posicion);
        XpressNet.receive();                        // recibimos paquetes por Xpressnet
      }
    }
  }
}


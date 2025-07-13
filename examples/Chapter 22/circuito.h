// TFT_TCO - Paco Cañada 2022 -- https://usuaris.tinet.cat/fmco/


//====  Macros y datos  ================================================================================

struct Simbolo {
  byte tipo;                                      // icono
  byte posX;                                      // columna 0..9 con 320x240
  byte posY;                                      // fila 0..6 con 320x240
  unsigned int direccion;                         // direcion DCC 1..1024 (0..1023 Xpressnet)(Bit 14 activo: mostrar posicion actual invertida)
};

enum posViaPaN        {VERTICAL, HORIZONTAL};
enum orientacion      {NORTE, ESTE, SUR, OESTE};
enum iconoOrientacion {NORDESTE, SUDESTE, SUDOESTE, NOROESTE, NE_SO, NO_SE};

enum tipoSimbolo  {
  ICON_VIA_V, ICON_VIA_H,                                                       // con icono, sin direccion
  ICON_CURVA_NE, ICON_CURVA_SE, ICON_CURVA_SO, ICON_CURVA_NO, ICON_CURVAS_NE_SO, ICON_CURVAS_NO_SE,
  ICON_TOPE_N, ICON_TOPE_E, ICON_TOPE_S, ICON_TOPE_O,
  ICON_TEXTO,                                                                   // sin icono, sin direccion
  ICON_RUTA,                                                                    // con icono,
  ICON_CONECTOR,
  ICON_SEMAFORO_N, ICON_SEMAFORO_E, ICON_SEMAFORO_S, ICON_SEMAFORO_O,           // con icono, con direccion
  ICON_PAN_S, ICON_PAN_B,
  ICON_DESVIO_V_NE, ICON_DESVIO_V_SE, ICON_DESVIO_V_SO, ICON_DESVIO_V_NO,       // sin icono, con direccion
  ICON_DESVIO_H_NE, ICON_DESVIO_H_SE, ICON_DESVIO_H_SO, ICON_DESVIO_H_NO,
};

#define NO_ASIGNADO   0x0F00
#define NO_EXISTE     0xFF
#define INVERTIDO(x)  (x | bit(14))

#define VIA(o,x,y)        {ICON_VIA_V+o,x,y,NO_ASIGNADO}
#define CURVA(o, x,y)     {ICON_CURVA_NE+o,x,y,NO_ASIGNADO}
#define TOPE(o,x,y)       {ICON_TOPE_N+o,x,y,NO_ASIGNADO}
#define PAN(o,x,y,d)      {ICON_PAN_S+o,x,y,d-1}
#define RUTA(x,y,d)       {ICON_RUTA,x,y,NO_ASIGNADO+d}
#define CONECTOR(x,y,d)   {ICON_CONECTOR,x,y,NO_ASIGNADO+d}
#define SEMAFORO(o,x,y,d) {ICON_SEMAFORO_N+o, x,y,d-1}
#define DESVIO_V(o,x,y,d) {ICON_DESVIO_V_NE+o, x,y,d-1}
#define DESVIO_H(o,x,y,d) {ICON_DESVIO_H_NE+o, x,y,d-1}
#define TEXTO(d)          {ICON_TEXTO,NO_EXISTE,NO_EXISTE,NO_ASIGNADO+d}


//====  Colores del circuito  ================================================================================

/*
#define COLOR_FONDO           tft.color565(235, 235, 235)
#define COLOR_FONDO_TITULO    AQUA
#define COLOR_FONDO_TIT_STOP  PINK
#define COLOR_TITULO          WHITE
#define COLOR_TEXTO           BLACK
#define COLOR_VIA             BLACK
#define COLOR_PAN             RED
#define COLOR_RUTA            ORANGE
#define COLOR_CONECTOR        DARKCYAN
#define COLOR_DESVIO_NO_POS   DARKGREY
#define COLOR_DESVIO_POS      YELLOW
*/

  #define COLOR_FONDO           BLACK             // Dark mode
  #define COLOR_FONDO_TITULO    AQUA
  #define COLOR_FONDO_TIT_STOP  PINK
  #define COLOR_TITULO          WHITE
  #define COLOR_TEXTO           WHITE
  #define COLOR_VIA             LIGHTGREY
  #define COLOR_PAN             RED
  #define COLOR_RUTA            ORANGE
  #define COLOR_CONECTOR        LIME
  #define COLOR_DESVIO_NO_POS   DARKGREY
  #define COLOR_DESVIO_POS      YELLOW


//====  Parametros del circuito  ================================================================================


const int tiempoAccesorioON   = 150;              // Tiempo de activacion accesorios
const int tiempoCDU           = 250;              // Tiempo espera recarga CDU
const int tiempoPosicionInfo  = 300;              // Tiempo entre peticion informacion posicion


//====  Definicion del circuito  ================================================================================

#define NUM_PANTALLAS 3

#define LONG_PANTALLA(x) sizeof(x)/sizeof(Simbolo)

const char texto0[] = {"@64,172[ ESTACION ]"};
const char texto1[] = {"@74,150;64@202;61@170,118;62,66;63"};  // Y = +2,+12,+22   X= +10
const char texto2[] = {"@138,86;69@170;70@230,140P.N. abierto@230,172P.N. cerrado"};
const char texto3[] = {"@138,130;73"};
/*
  const char ruta0[] = {"64R61R"};        // RECTO:    VERDE  +  <
  const char ruta1[] = {"64D62D61D"};     // DESVIADO: ROJO  -  >
  const char ruta2[] = {"61D62R63D"};
*/
const char ruta0[] = {"64R61R"};        // DESVIADO: ROJO  -  >
const char ruta1[] = {"64D62D61D"};      // RECTO:    VERDE  +  <
const char ruta2[] = {"61D62R63D"};
const char ruta3[] = {"70D69R"};
const char ruta4[] = {"70R69D"};

const char nombrePantalla0[]  = {"SALOU"};
const Simbolo pantalla0[]  = {
  TOPE(OESTE, 1, 2), VIA(HORIZONTAL, 2, 2), RUTA(3, 2, 2), VIA(HORIZONTAL, 4, 2), DESVIO_H(SUDOESTE, 5, 2, 63), VIA(HORIZONTAL, 6, 2), TOPE(ESTE, 7, 2),
  CURVA(SUDESTE, 2, 3), VIA(HORIZONTAL, 3, 3), RUTA(4, 3, 1),  DESVIO_H(NORDESTE, 5, 3, INVERTIDO(62)), CURVA(SUDOESTE, 6, 3),
  TOPE(OESTE, 1, 4), DESVIO_H(NOROESTE, 2, 4, 64), RUTA(3, 4, 0), VIA(HORIZONTAL, 4, 4), VIA(HORIZONTAL, 5, 4), DESVIO_H(NORDESTE, 6, 4, 61), VIA(HORIZONTAL, 7, 4), CONECTOR(8, 4, 1),
  TEXTO(0), TEXTO(1),
};

const char nombrePantalla1[] = {"PASO A NIVEL"};
const Simbolo pantalla1[] = {
  PAN(VERTICAL, 4, 1, 69), PAN(VERTICAL, 4, 3, 69),
  CONECTOR(2, 2, 0), VIA(HORIZONTAL, 3, 2), VIA(HORIZONTAL, 4, 2), SEMAFORO(OESTE, 5, 2, 70), VIA(HORIZONTAL, 6, 2), CONECTOR(7, 2, 2),
  RUTA(6, 4, 3), RUTA(6, 5, 4),
  TEXTO(2),
};

const char nombrePantalla2[] = {"BUCLE LIGERO"};
const Simbolo pantalla2[] = {
  CURVA(SUDESTE, 5, 2), VIA(HORIZONTAL, 6, 2), CURVA(SUDOESTE, 7, 2),
  VIA(VERTICAL, 5, 3), VIA(VERTICAL, 7, 3),
  CONECTOR(2, 4, 1), VIA(HORIZONTAL, 3, 4), DESVIO_H(SUDOESTE, 4, 4, 73), CURVA(NOROESTE, 5, 4), VIA(VERTICAL, 7, 4),
  CURVA(NORDESTE, 4, 5), VIA(HORIZONTAL, 5, 5), VIA(HORIZONTAL, 6, 5), CURVA(NOROESTE, 7, 5),
  TEXTO(3),
};

// Inicializar los siguientes arrays con los valores adecuados a nuestro circuito

const char *textos[]                        = {texto0, texto1, texto2, texto3};
const char *rutas[]                         = {ruta0, ruta1, ruta2, ruta3, ruta4};
const Simbolo     *Pantalla[NUM_PANTALLAS]  = {pantalla0, pantalla1, pantalla2};
const int numIconosPantalla[NUM_PANTALLAS]  = {LONG_PANTALLA(pantalla0), LONG_PANTALLA(pantalla1), LONG_PANTALLA(pantalla2)};
const char *nombrePantallas[NUM_PANTALLAS]  = {nombrePantalla0, nombrePantalla1, nombrePantalla2};

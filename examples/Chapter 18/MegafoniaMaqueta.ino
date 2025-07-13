#include <NmraDcc.h>
#include <DFRobotDFPlayerMini.h>

NmraDcc Dcc;                                              // Objetos libreria
DFRobotDFPlayerMini DFPlayer;

const byte DCC_PIN = 2;                                   // pin señal DCC
#define CV_VOLUMEN 33                                     // CV para volumen por defecto
#define DEF_VOLUMEN 20                                    // Valor del volumen por defecto (1-30)

int  volumen;
bool Playing;
byte ultimaCarpeta = 1;
bool repetir = false;

byte dccSonido = -1;
byte dccCarpeta = ultimaCarpeta + 23;
int  dccBase;

unsigned long dccTiempo = millis();
unsigned long tiempoDFP;

#define NUM_ACC_COLA  64                                  // Numero de accesorios en la cola de envio

byte sonidoFIFO [NUM_ACC_COLA];                            // cola FIFO
int ultimoLlegar;                                         // indice cabeza cola
int primeroSalir;                                         // indice final cola
int enCola;                                               // numero de accesorios en cola


void setup() {
  Serial.begin (9600);                                      // Comunicacion con DFPlayer
  if (!DFPlayer.begin (Serial))
    errorSD();                                              // en caso de error, comprobar conexion y SD
  DFPlayer.setTimeOut(500);                                 // tiempo para responder 500ms
  DFPlayer.outputDevice(DFPLAYER_DEVICE_SD);                // selecciona reproducir desde tarjeta SD
  Dcc.pin (digitalPinToInterrupt (DCC_PIN), DCC_PIN, 1);
  Dcc.initAccessoryDecoder (MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0);
  dccBase = leeDireccionDcc();                              // Lee direccion base DCC
  volumen = Dcc.getCV (CV_VOLUMEN);                         // Lee volumen por defecto
  volumen = constrain (volumen, 1, 30);
  DFPlayer.volume(volumen);
  resetPlayList();                                          // resetea lista reproduccion
 }

void loop() {
  Dcc.process();                                            // procesa paquetes DCC
  runDFPlayer();                                            // procesa estado reproduccion
  if (DFPlayer.available()) {                               // recibe mensaje de DFPlayer
    mensajeDFPlayer(DFPlayer.readType(), DFPlayer.read());
  }
}

void borraFIFO () {
  ultimoLlegar=0;                                       // indice cabeza cola
  primeroSalir=0;                                       // indice final cola
  enCola=0;                                             // numero de accesorios en cola
}

byte readFIFO () {
  primeroSalir = (primeroSalir + 1 ) % NUM_ACC_COLA;    //avanza puntero
  enCola--;
  return (sonidoFIFO[primeroSalir]);
}

void writeFIFO (byte sonido) {
  ultimoLlegar = (ultimoLlegar + 1 ) % NUM_ACC_COLA;    // avanza puntero
  enCola++;
  sonidoFIFO[ultimoLlegar] = sonido;                    // lo guarda en la cola
}




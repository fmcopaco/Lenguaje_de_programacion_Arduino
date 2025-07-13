/*  AutoSensoSignal version Arduino

   Controla dos semaforos de 3 luces RENFE. Al detectar tren pasa a ROJO, tras un tiempo a AMARILLO y luego VERDE

                   Norte  Sur
   Rojo             D3    D6
   Verde            D4    D7
   Amarillo         D5    D8
   Rele             D9    D10
   Sensor           D11   D12

   Programacion de CV:
   Direccion        CV1,CV9  (por defecto CV1=1, CV9=0: Direcciones 1 a 4)
   Reset            CV8 = 8

                    Norte   Sur
   Tiempo rojo      CV35    CV37
   Tiempo amarillo  CV36    CV38

*/

#include <Semaforo.h>       // Libreria Semaforo.h -- Paco Cañada 2020 -- https://usuaris.tinet.cat/fmco/arduino_sp.html
#include <NmraDcc.h>        // Libreria NmraDcc.h  -- https://github.com/mrrwa/NmraDcc

#define TIEMPO_ROJO     16                                  // Tiempos por defecto en 0,5s  (CV35, CV37)
#define TIEMPO_AMARILLO 8                                   //                              (CV36, CV39)

#define RELE_ON   LOW                                       // Activacion de los reles
#define RELE_OFF  HIGH

NmraDcc   Dcc;                                              // Objetos librerias
Semaforo  SemViaNorte;
Semaforo  SemViaSur;

const byte DCC_PIN        = 2;                              // pin señal DCC
const byte releViaNorte   = 9;                              // pin rele via norte
const byte releViaSur     = 10;                             // pin rele via sur
const byte sensorViaNorte = 11;                             // pin detector via norte
const byte sensorViaSur   = 12;                             // pin detector via sur

int  dccBase;                                               // direccion base DCC

unsigned long temporizadorViaNorte;                         // temporizadores espera
unsigned long temporizadorViaSur;

byte  estadoViaNorte;                                       // estado automatismo via
byte  estadoViaSur;

enum estadoVia {ESPERA_TREN, OCUPADO, LIBERADO};


void setup() {
  pinMode (sensorViaNorte, INPUT_PULLUP);                   // define las entradas y salidas
  pinMode (sensorViaSur, INPUT_PULLUP);
  pinMode (releViaNorte, OUTPUT);
  pinMode (releViaSur, OUTPUT);
  SemViaNorte.init (3, 4, 5);                               // define los semaforos y sus conexiones
  SemViaSur.init (6, 7, 8);

  cambiaSemaforoNorte (ESPERA_TREN, ANUNCIO_PARADA, RELE_OFF);    // aspecto por defecto
  cambiaSemaforoSur   (ESPERA_TREN, ANUNCIO_PARADA, RELE_OFF);

  Dcc.pin (digitalPinToInterrupt (DCC_PIN), DCC_PIN, 1);    // Inicializa DCC
  Dcc.initAccessoryDecoder (MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0);
  dccBase = leeDireccionDcc();                              // Lee direccion base DCC
}


void loop() {
  Dcc.process();                                            // Procesa paquetes DCC
  SemViaNorte.process();                                    // Procesa semaforos
  SemViaSur.process();
  compruebaSensorNorte();                                   // Comprueba sensores
  compruebaSensorSur();
}


int leeDireccionDcc() {
  int direccion, cv1, cv9;

  if (Dcc.getCV (CV_MANUFACTURER_ID) != MAN_ID_DIY) {       // Si CV8 no es 13, resetea CV
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_LSB, 1);
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_MSB, 0);
    Dcc.setCV (35, TIEMPO_ROJO);
    Dcc.setCV (36, TIEMPO_AMARILLO);
    Dcc.setCV (37, TIEMPO_ROJO);
    Dcc.setCV (38, TIEMPO_AMARILLO);
    Dcc.setCV (CV_MANUFACTURER_ID, MAN_ID_DIY);
  }
  cv1 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_LSB);
  cv9 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_MSB);
  direccion =  (( cv9 << 8) + (cv1 << 2)) - 3 ;
  return (direccion);
}


void notifyDccCVChange( uint16_t CV, uint8_t Value) {       // Si cambia una CV comprobamos de nuevo direccion
  dccBase = leeDireccionDcc();
}


void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) {   // orden DCC cambio desvio
  byte orden;

  if ((Addr >= dccBase) && (Addr <= dccBase + 3) && OutputPower) {    // comprueba direccion dentro del rango
    orden = (((Addr - dccBase) << 1) + Direction);                    // orden de cambio de aspecto entre 0 y 7
    switch (orden) {                                                  // actua segun orden
      case 0:                                               // Direccion base : Semaforo 3 luces
        SemViaNorte.aspecto (PARADA);
        break;
      case 1:
        SemViaNorte.aspecto (VIA_LIBRE);
        break;
      case 2:
        SemViaNorte.aspecto (ANUNCIO_PARADA);
        break;
      case 3:
        SemViaNorte.aspecto (ANUNCIO_PRECAUCION);
        break;

      case 4:                                               // Direccion base + 2: Semaforo 3 luces
        SemViaSur.aspecto (PARADA);
        break;
      case 5:
        SemViaSur.aspecto (VIA_LIBRE);
        break;
      case 6:
        SemViaSur.aspecto (ANUNCIO_PARADA);
        break;
      case 7:
        SemViaSur.aspecto (ANUNCIO_PRECAUCION);
        break;
    }
  }
}


void compruebaSensorNorte() {
  unsigned long tiempoEspera;
  switch (estadoViaNorte) {
    case ESPERA_TREN:                                       // si detecta tren pasa a rojo
      if (digitalRead(sensorViaNorte) == LOW) {
        cambiaSemaforoNorte (OCUPADO, PARADA, RELE_ON);
      }
      break;
    case OCUPADO:                                           
      if (digitalRead(sensorViaNorte) == LOW) {             // mientras detecta resetea temporizador
        temporizadorViaNorte = millis();
      }
      tiempoEspera = Dcc.getCV (35) * 500UL;                // si ha pasado CV35*0,5s sin detectar 
      if (millis() - temporizadorViaNorte > tiempoEspera) { // pasa a amarillo
        cambiaSemaforoNorte (LIBERADO, ANUNCIO_PARADA, RELE_OFF);
      }
      break;
    case LIBERADO:
      if (digitalRead(sensorViaNorte) == LOW) {             // si detecta tren pasa a rojo
        cambiaSemaforoNorte (OCUPADO, PARADA, RELE_ON);
      }
      else {
        tiempoEspera = Dcc.getCV (36) * 500UL;                  // si ha pasado CV36*0,5s sin detectar 
        if (millis() - temporizadorViaNorte > tiempoEspera) {   // pasa a verde
          cambiaSemaforoNorte (ESPERA_TREN, VIA_LIBRE, RELE_OFF);
        }
      }
      break;
  }
}


void compruebaSensorSur() {
  unsigned long tiempoEspera;
  switch (estadoViaSur) {
    case ESPERA_TREN:                                       // si detecta tren pasa a rojo
      if (digitalRead(sensorViaSur) == LOW) {
        cambiaSemaforoSur (OCUPADO, PARADA, RELE_ON);
      }
      break;
    case OCUPADO:
      if (digitalRead(sensorViaSur) == LOW) {               // mientras detecta resetea temporizador
        temporizadorViaSur = millis();
      }
      tiempoEspera = Dcc.getCV (37) * 500UL;                // si ha pasado CV37*0,5s sin detectar
      if (millis() - temporizadorViaSur > tiempoEspera) {   // pasa a amarillo
        cambiaSemaforoSur (LIBERADO, ANUNCIO_PARADA, RELE_OFF);
      }
      break;
    case LIBERADO:
      if (digitalRead(sensorViaSur) == LOW) {               // si detecta tren pasa a rojo
        cambiaSemaforoSur (OCUPADO, PARADA, RELE_ON);
      }
      else {
        tiempoEspera = Dcc.getCV (38) * 500UL;                // si ha pasado CV38*0,5s sin detectar 
        if (millis() - temporizadorViaSur > tiempoEspera) {   // pasa a verde
          cambiaSemaforoSur (ESPERA_TREN, VIA_LIBRE, RELE_OFF);
        }
      }
      break;
  }
}


void cambiaSemaforoNorte (estadoVia estado, aspectos aspectoSemaforo, int estadoRele) {
  estadoViaNorte = estado;
  temporizadorViaNorte = millis();
  SemViaNorte.aspecto (aspectoSemaforo);
  digitalWrite (releViaNorte, estadoRele);
}


void cambiaSemaforoSur (estadoVia estado, aspectos aspectoSemaforo, int estadoRele) {
  estadoViaSur = estado;
  temporizadorViaSur = millis();
  SemViaSur.aspecto (aspectoSemaforo);
  digitalWrite (releViaSur, estadoRele);
}

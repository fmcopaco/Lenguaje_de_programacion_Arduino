#include "Semaforo.h"        // Libreria Semaforo.h -- Paco Cañada 2020 -- https://usuaris.tinet.cat/fmco/
#include <NmraDcc.h>

NmraDcc Dcc;                                                // Objetos librerias

Semaforo Sem2Luces;
Semaforo Sem3Luces;
Semaforo Sem4Luces;
Semaforo Sem2LucesManiobras;
Semaforo Sem2LucesPasoNivel;

const byte DCC_PIN = 2;                                     // pin señal DCC
int  dccBase;                                               // direccion base DCC

void setup() {
  Sem2Luces.init (0, 1);                                    // define los semaforos y sus conexiones
  Sem3Luces.init (3, 4, 5);
  Sem4Luces.init (6, 7, 8, 9);
  Sem2LucesManiobras.initManiobra(10, 11);
  Sem2LucesPasoNivel.initPasoNivelVehiculos (13, 12);

  Dcc.pin (digitalPinToInterrupt (DCC_PIN), DCC_PIN, 1);    // Inicializa DCC
  Dcc.initAccessoryDecoder (MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0);
  dccBase = leeDireccionDcc();                              // Lee direccion base DCC
}

void loop() {
  Dcc.process();                                            // Procesa paquetes DCC
  Sem2Luces.process();                                      // Procesa semaforos
  Sem3Luces.process();
  Sem4Luces.process();
  Sem2LucesManiobras.process();
  Sem2LucesPasoNivel.process();
}

int leeDireccionDcc() {
  int direccion, cv1, cv9;

  if (Dcc.getCV (CV_MANUFACTURER_ID) != MAN_ID_DIY) {       // Si CV8 no es 13, resetea CV
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_LSB, 1);
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_MSB, 0);
    Dcc.setCV (CV_MANUFACTURER_ID, MAN_ID_DIY);
  }
  cv1 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_LSB);
  cv9 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_MSB);
  direccion =  (( cv9 << 8) + (cv1 << 2)) - 3 ;
  return (direccion);
}

void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) { 
  byte orden;
  
  if ((Addr >= dccBase) && (Addr <= dccBase + 11) && OutputPower) {   // comprueba direccion dentro del rango
    orden = (((Addr - dccBase) << 1) + Direction);                    // orden de cambio de aspecto entre 0 y 21
    switch (orden) {                                                  // actua segun orden
      case 0:                                                         // Direccion base: Semaforo dos luces
        Sem2Luces.aspecto (PARADA);
        break;
      case 1:
        Sem2Luces.aspecto (VIA_LIBRE);
        break;
      
      case 2:                                               // Direccion base + 1/2: Semaforo 3 luces
        Sem3Luces.aspecto (PARADA);
        break;
      case 3:
        Sem3Luces.aspecto (VIA_LIBRE);
        break;      
      case 4:
        Sem3Luces.aspecto (ANUNCIO_PRECAUCION);
        break; 
      case 5:
        Sem3Luces.aspecto (ANUNCIO_PARADA);
        break; 

      case 6:                                               // Direccion base + 3/4/5/6/7: Semaforo 3 luces
        Sem4Luces.aspecto (PARADA);
        break;
      case 7:
        Sem4Luces.aspecto (VIA_LIBRE);
        break;      
      case 8:
        Sem4Luces.aspecto (ANUNCIO_PRECAUCION);
        break; 
      case 9:
        Sem4Luces.aspecto (ANUNCIO_PARADA);
        break; 
      case 10:
        Sem4Luces.aspecto (VIA_LIBRE_CONDICIONAL);
        break;       
      case 11:
        Sem4Luces.aspecto (ANUNCIO_PARADA_INMEDIATA);
        break;       
      case 12:
        Sem4Luces.aspecto (-1);                             // apagado de todas las luces
        break;
      case 13:
        Sem4Luces.aspecto (MOVIMIENTO_AUTORIZADO);
        break; 
      case 14: 
        Sem4Luces.aspecto (REBASE_AUTORIZADO);
        break;
      case 15:
        Sem4Luces.aspecto (REBASE_AUTORIZADO_NO_PARAR);
        break;      


      case 16:                                                // Direccion base + 8/9: Semaforo 2 luces Maniobras
        Sem2LucesManiobras.aspecto (-1);                      // apagado de todas las luces  
        break;
      case 17:
        Sem2LucesManiobras.aspecto (MOVIMIENTO_AUTORIZADO);
        break;
      case 18:
        Sem2LucesManiobras.aspecto (REBASE_AUTORIZADO);
        break;
      case 19:
        Sem2LucesManiobras.aspecto (REBASE_AUTORIZADO_NO_PARAR);
        break;      
 
       case 20:                                                // Direccion base + 10/11: Semaforo 2 paso a nivel
        Sem2LucesPasoNivel.aspecto (PASO_NIVEL_CERRADO);
        break;
       case 21:
        Sem2LucesPasoNivel.aspecto (PASO_NIVEL_ABIERTO);
        break;         

    }
  }  
}


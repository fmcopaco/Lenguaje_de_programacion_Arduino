int leeDireccionDcc() {
  int direccion, cv1, cv9;

  if (Dcc.getCV (CV_MANUFACTURER_ID) != MAN_ID_DIY) {       // Si CV8 no es 13, resetea CV
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_LSB, 1);
    Dcc.setCV (CV_ACCESSORY_DECODER_ADDRESS_MSB, 0);
    Dcc.setCV (CV_VOLUMEN, DEF_VOLUMEN);
    Dcc.setCV (CV_MANUFACTURER_ID, MAN_ID_DIY);
  }
  cv1 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_LSB);
  cv9 = Dcc.getCV (CV_ACCESSORY_DECODER_ADDRESS_MSB);
  direccion =  (( cv9 << 8) + (cv1 << 2)) - 3 ;
  return (direccion);
}

void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) { 
  byte sonido;
  unsigned long tiempoActual;

  if ((Addr >= dccBase) && (Addr <= dccBase + 15) && OutputPower) {   // comprueba direccion dentro del rango
    sonido = (((Addr - dccBase) << 1) + Direction);                   // orden de sonido entre 0 y 31
    switch (sonido) {                                                 // actua segun orden
      case 24:                                                        // Seleccion de carpeta SD (01 a 06)
      case 25:
      case 26:
      case 27:
      case 28:
      case 29:
        if (dccCarpeta != sonido) {                                   // mete en la cola los cambios de carpeta
          dccCarpeta = sonido;
          writeFIFO (sonido);
        }
        break;
      case 30:                                                        // Detiene sonidos
        DFPlayer.stop();
        resetPlayList();
        writeFIFO (dccCarpeta);                                       // Reintroduce ultima carpeta recibida
        break;
      case 31:                                                        // Repite ultimo sonido
        tiempoActual = millis();
        if (tiempoActual - dccTiempo < 1000)                          // Ha de pasar mas de un segundo para poder repetir
          break;
        dccTiempo = tiempoActual;
        if (repetir)
          desactivaBucleSonido();
        else 
          activaBucleSonido();
        break;
      default:
        tiempoActual = millis();                                      // Seleccion sonido (001.mp3 a 024.mp3)
        if (sonido == dccSonido) {                                    // Evita paquetes repetidos
          if (tiempoActual - dccTiempo < 1000)                        // Ha de pasar mas de un segundo para reintroducir el mismo sonido
            break;
        }
        writeFIFO(sonido);
        dccSonido = sonido;
        dccTiempo = tiempoActual;
        break;
    }
  }
}





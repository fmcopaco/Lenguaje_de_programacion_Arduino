void resetPlayList () {             // borra la lista de reproduccion
  borraFIFO();
  notPlaying();
}

void notPlaying () {                // pone estado de no reproduciendo
  Playing=false;  
  tiempoDFP = millis();
}

void activaBucleSonido () {
  repetir = true;
  DFPlayer.enableLoop();
}

void desactivaBucleSonido () {
  repetir = false;
  DFPlayer.disableLoop();
}

void mensajeDFPlayer(uint8_t tipo, int valor){    // procesa mensaje del DFPlayer
  switch (tipo) {
    case DFPlayerPlayFinished:      //track is finished playing
    case TimeOut:                   //DFPlayer doesn't answer in 500ms
    case WrongStack:
    case DFPlayerCardRemoved:       //SD card is pulled out
    case DFPlayerCardInserted:      //SD card is plugged in
      notPlaying();
      break;
    case DFPlayerUSBInserted:       //USB flash is plugged in
    case DFPlayerUSBRemoved:        //USB flash is pulled out
    case DFPlayerCardOnline:        //SD card online
      break;
    case DFPlayerError:
      switch (valor) {
        case Busy:                  //initialization is not done
        case FileIndexOut:          //Specified track is out of current track scope
        case FileMismatch:          //Specified track is not found
        case CheckSumNotMatch:      //Checksum incorrect
        case SerialWrongStack:      //a frame has not been received completely yet
        case Sleeping:              //supports only specified device in sleep mode
        case Advertise:             //a inter-cut operation only can be done when a track is being played)
          notPlaying();
          break;        
      }
      break;
    default:                        // cualquier otro error
      break;
  }
}

void runDFPlayer() {
  byte sonido;

  if ((! Playing) && (enCola > 0)) {          // si no se reproduce sonido y hay sonidos en la cola
    if ((millis()- tiempoDFP) > 100) {        // DFPlayer necesita 100ms al acabar la reproduccion para inicializar la informacion de la proxima
      sonido = readFIFO();
      if (sonido > 23) {                      // cambio de carpeta
        ultimaCarpeta = sonido - 23;
      }
      else {                                  // sonido
        tiempoDFP = millis();      
        DFPlayer.playFolder(ultimaCarpeta, sonido + 1);
        repetir = false; 
        Playing = true;    
      }
    }
  }
}

void errorSD () {
  pinMode (LED_BUILTIN, OUTPUT);
  for (;;) {                                // Parpadea LED indefinidamente
    digitalWrite (LED_BUILTIN, HIGH);
    delay (500);
    digitalWrite (LED_BUILTIN, LOW);
    delay (500);
  }
}


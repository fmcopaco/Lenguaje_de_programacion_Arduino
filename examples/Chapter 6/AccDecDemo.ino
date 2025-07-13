// Demostració de la llibreria NmraDcc com descodificador d'accessoris -- Paco Cañada 2019

#include <NmraDcc.h>              // Llibreria DCC

NmraDcc  Dcc ;                    // Crea el objecte DCC
                                  // Definim unes constants que pot canviar l'usuari
const byte DCC_PIN = 2;           // DCC  pin
const byte LED_PIN = 13;          // LED pin
const int DCC_ADDRESS = 6;        // Adreça accessori

void setup() {                    // Inicialització
  pinMode (LED_PIN, OUTPUT );     // LED apagat per defecte
  digitalWrite(LED_PIN, LOW);
                                  // Configurem pin i tipus de descodificador
  Dcc.pin(digitalPinToInterrupt(DCC_PIN), DCC_PIN, 1);
  Dcc.initAccessoryDecoder( MAN_ID_DIY, 1, FLAGS_OUTPUT_ADDRESS_MODE, 0 );
}

void loop() {                     // Bucle principal
  Dcc.process();                  // Procesa la descodificacio de la senyal DCC
}

// Quan arriba un paquet DCC per accesoris s'executa aquesta rutina
void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower ) {
    if(( Addr == DCC_ADDRESS ) && OutputPower ) { // Si es la nostra adreça i esta activa
      if (Direction == 0)                         // Segons sigui recte/desviat
        digitalWrite(LED_PIN, LOW);               // apaguem LED
      else                                        // o
        digitalWrite(LED_PIN, HIGH);              // encenem LED
    }
}



// Diorama en un tarro de cristal - Scene in a jar -- Paco Cañada 2022 -- https://usuaris.tinet.cat/fmco/
// Usa un Arduino Pro Mini
// Vamos a trabajar a 8MHz
// Compilar siempre el sketch seleccionando Arduino Pro Mini - ATmega 328P (3.3V, 8MHz)
// Comentar o borrar la siguiente linea si usamos un Arduino Pro Mini a 8MHz
#define XTAL_16MHz

#include <Adafruit_NeoPixel.h>
#include <LowPower.h>
#include <avr/power.h>

#define NUMPIXELS     1
#define NEOPIXEL_PIN  6
#define FAROLA_PIN    7
#define REED_PIN      2               // pin de interrupcion D2 o D3

#define AREF_mV       1100            // VBG referencia interna 1100 mV
#define BATT_MIN      3500            // Tension minima bateria para aviso

const unsigned int tiempoDia   = 80;  // duracion del dia en segundos
const unsigned int tiempoNoche = 112; // duracion de la noche en segundos

const unsigned long BAND_GAP = 1024UL * AREF_mV;

enum leds     {ROJO, VERDE, AZUL};
enum escenas  {DIA, ATARDECER, NOCHE, AMANECER};

escenas escenaActual;

byte colorEscena[][3] = {             // colores iniciales de la transicion
  {160,  50,   0},                    // amanecer -> dia
  {255, 255, 255},                    // dia -> atardecer
  {200,  50, 110},                    // atardecer -> noche
  {  0,   0, 255}                     // noche -> amanecer
};

byte batteryFlash;

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
#ifdef XTAL_16MHz
  clock_prescale_set(clock_div_2);    // Permite dividir por 1, 2, 4, 8, 16, 32, 64, 128 y 256
#endif
  pinMode(LED_BUILTIN, OUTPUT);       // LED aviso bateria baja
  pinMode(FAROLA_PIN, OUTPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(FAROLA_PIN, LOW);
  pixels.begin();                     // inicializa pin Neopixels
}

void loop() {
  digitalWrite (FAROLA_PIN, LOW);     // Apagar farola al alba
  transicion(DIA);                    // El Sol va saliendo
  dormirSiesta(tiempoDia, ATARDECER); // Esperar hasta el atardecer
  transicion(ATARDECER);              // El Sol va bajando
  digitalWrite (FAROLA_PIN, HIGH);    // Encedender farola al atardecer
  transicion(NOCHE);                  // El Sol se esconde
  dormirSiesta(tiempoNoche, AMANECER);// Esperar hasta el amanecer
  transicion(AMANECER);               // Los primeros rayos del Sol
}


void transicion(escenas escena) {
  byte r, g, b, i;
  escenas proxEscena;
  escenaActual = escena;
  proxEscena = (escena == AMANECER) ? DIA : escena + 1;
  for (i = 0; i < 255; i++) {
    r = map(i, 0, 255, colorEscena[escenaActual][ROJO],  colorEscena[proxEscena][ROJO]);
    g = map(i, 0, 255, colorEscena[escenaActual][VERDE], colorEscena[proxEscena][VERDE]);
    b = map(i, 0, 255, colorEscena[escenaActual][AZUL],  colorEscena[proxEscena][AZUL]);
    pixels.setPixelColor(0, pixels.Color(pixels.gamma8 (r), pixels.gamma8 (g), pixels.gamma8 (b)));
    pixels.show();
    checkBattery();                   // comprueba bateria
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);     // pausa 60ms
    if (imanDetectado())              // reiniciar transicion al reactivar
      i = 0;
  }
  digitalWrite (LED_BUILTIN, LOW);    // apaga LED fuera de la transicion
}


void dormirSiesta (unsigned int segundos, escenas escena) {
  escenaActual = escena;
  while (segundos > 8) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);   // da cabezaditas de 8 segundos
    segundos = (imanDetectado()) ? 0 : segundos - 8;
  }
  if (segundos > 4) {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);   // resto de cabezaditas hasta tiempo de espera
    segundos = (imanDetectado()) ? 0 : segundos - 4;
  }
  if (segundos > 2) {
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    segundos = (imanDetectado()) ? 0 : segundos - 2;
  }
  if (segundos > 1)
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}

void dormirProfundo() {
  attachInterrupt(digitalPinToInterrupt(REED_PIN), despierta, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(digitalPinToInterrupt(REED_PIN));
}

void despierta () {                   // rutina de interrupcion externa
}                                     // no necesita hacer nada


bool imanDetectado() {
  byte r, g, b;
  if (digitalRead(REED_PIN) == LOW) { // comprueba reed
    pixels.clear();                   // apaga Neopixel y LED
    pixels.show();
    digitalWrite(FAROLA_PIN, LOW);
    digitalWrite (LED_BUILTIN, LOW);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);    // espera a retirar iman
    dormirProfundo();
    if ((escenaActual == NOCHE) || (escenaActual == AMANECER))
      digitalWrite(FAROLA_PIN, HIGH);
    r = colorEscena[escenaActual][ROJO];
    g = colorEscena[escenaActual][VERDE];
    b = colorEscena[escenaActual][AZUL];
    pixels.setPixelColor(0, pixels.Color(pixels.gamma8 (r), pixels.gamma8 (g), pixels.gamma8 (b)));
    pixels.show();
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);    // espera a retirar iman
    return true;
  }
  else
    return false;
}

unsigned int readVcc() {
  unsigned int result;
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG)
  ADMUX = bit (REFS0) | bit (MUX3) | bit (MUX2) | bit (MUX1);   // Voltage Reference: AVCC, Input Channel: 1.1V (VBG)
  delay(2);                                             // Wait for Vref to settle
  ADCSRA |= bit( ADSC );                                // start conversion
  while (ADCSRA & bit (ADSC));                          // wait until done
  result = ADC;                                         // read inaccurate result
  ADCSRA |= bit( ADSC );                                // Start conversion, second time is a charm
  while (ADCSRA & bit (ADSC));                          // wait until done
  result = BAND_GAP / ADC;                              // Back-calculate AVcc in mV    Vcc (in mV) = 1024 × 1100 / ADC
  return result;                                        // Vcc in millivolts
}

void checkBattery() {                                  // cada 60ms durante transiciones
  batteryFlash++;
  if ((readVcc() < BATT_MIN) && ((batteryFlash & 0x0F) == 0)) // flash cada segundo (60ms * 16 = 960ms)
    digitalWrite (LED_BUILTIN, HIGH);
  else
    digitalWrite (LED_BUILTIN, LOW);
}


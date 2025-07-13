/* Iluminacion aleatoria de varias luces en un edificio con diferentes 
   tiempos de apagado y encendido - Paco Cañada 2019                   */

#define numPins 6                         // numero de LEDs a controlar

int ledPin []  = { 2, 3, 4, 5, 6, 7};        // pines en los que estan conectados los LED
unsigned long finTiempo [numPins];        // fin de tiempo para encender o apagar LED
int estadoPin [numPins];                  // Estado actual del LED

long tiempoMinimoApagado = 10000;         // tiempo minimo que el LED esta apagado
long tiempoMaximoApagado = 25000;         // tiempo maximo que el LED esta apagado
long tiempoMinimoEncendido = 5000;        // tiempo minimo que el LED esta encendido
long tiempoMaximoEncendido = 20000;       // tiempo maximo que el LED esta encendido


void setup () {
  randomSeed (analogRead(A0));            // A0 es un pin analogico no usado, al aire. Lee el ruido.
  
  for (int n = 0; n < numPins ; n++) {    // Pone los pines como salidas y LED apagado por defecto
    pinMode  (ledPin[n], OUTPUT);         // pin como salida
    digitalWrite (ledPin[n], LOW);        // pin apagado
    estadoPin[n] = LOW;                   // guarda estado actual del pin en el array
    finTiempo[n] = nuevoTiempo(LOW);      // asigna un nuevo tiempo para cambiar de estado
  }
}


void loop () {
  for (int n = 0; n < numPins; n++) {     // Bucle para todos los LED
    if (millis() > finTiempo[n]) { // si el tiempo actual es mayor que el fin de tiempo del pin
      if (estadoPin[n] == LOW) {          // Mira el estado actual del pin si estaba apagado
        digitalWrite(ledPin[n], HIGH);    // si es asi, lo enciende
        estadoPin[n] = HIGH;              // y actualiza su estado
      } 
      else {                              // si por el contrario estaba encendido
        digitalWrite(ledPin[n], LOW);     // lo apaga
        estadoPin[n] = LOW;               // y actualiza su estado
      }
      finTiempo[n] = nuevoTiempo (estadoPin[n]);  // asigna un nuevo tiempo para el cambio de estado
    }
  }
}


unsigned long nuevoTiempo (int estado) {  // tiempo aleatorio a partir del tiempo actual y estado

  unsigned long tiempo;

  if (estado == HIGH)                     // tiempo aleatorio segun estado
    tiempo = random (tiempoMinimoEncendido, tiempoMaximoEncendido);
  else
    tiempo = random (tiempoMinimoApagado, tiempoMaximoApagado);
  tiempo += millis();                     // tiempo actual
  return (tiempo);                        // devuelve el tiempo calculado
}


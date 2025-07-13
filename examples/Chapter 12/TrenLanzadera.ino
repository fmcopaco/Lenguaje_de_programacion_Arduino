// Tren lanzadera analogico - Paco Cañada 2020

// Placa Arduino Motor Shield rev3
// salidas digitales:
const int dirA = 12;                      // dir: Polaridad de la salida
const int dirB = 13;
const int pwmA = 3;                       // pwm: activa/desactiva tension salida 
const int pwmB = 11;
const int brakeA = 9;                     // brake: Tiene que estar a LOW si no se ha cortado su jumper en la shield
const int brakeB = 8;
// entradas analogicas
const int snsA = A0;                      // Lectura de la corriente del motor. (1.65V/A)
const int snsB = A1;
const int poti = A2;                      // Potenciometro

#define deAaB LOW                         // sentido de marcha
#define deBaA HIGH
#define tiempoEspera 10000                // Tiempo de espera en estacion: 10s
#define numPulsos 12                      // numero de pulsos antes de cambiar de velocidad
#define minCorriente 6                    // Corriente minima para deteccion

int velActual;                            // velocidad actual del tren
int velObjetivo;                          // velocidad objetivo del tren (potenciometro)
int dirActual;                            // direccion actual del tren
int SensorA, SensorB;                     // lectura de la corriente
unsigned long AccDec;                     // temporizador para acelerar/frenar

enum Estado {ACELERA, CORRE, FRENA, ESPERA};

Estado Fase;

void setup()
{  
  pinMode(pwmA, OUTPUT);                  // Inicializa pines de salida del Arduino motor shield v3
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);
  pinMode(dirA, OUTPUT);
  pinMode(dirB, OUTPUT);

  bitSet(TCCR2B,2);                       // Timer 2 divisor: 256, frecuencia PWM en D3 y D11: 122.55 Hz (8ms)
  bitSet(TCCR2B,1);
  bitClear(TCCR2B,0);

  setVelocidad (0);                       // Detiene el tren
  digitalWrite(brakeA, LOW);              // Liberamos el freno
  digitalWrite(brakeB, LOW);
  buscaTren ();                           // localiza el tren y pone la direccion correcta
  AccDec = millis();                      // inicializamos tiempo temporizador
  leeVelObjetivo ();                      // velocidad objetivo segun potenciometro
  Fase = ACELERA;                         // arrancamos tren
}

void loop() {
  switch (Fase) {
    case ACELERA:
      if ((millis() - AccDec) > (numPulsos * 8)) {  // si tiempo mayor de numPulsos de 8ms (122hz) 
        AccDec = millis();
        velActual = (velActual > 251) ? 255 : velActual + 4; // Aumentamos velocidad
        setVelocidad (velActual);
        leeVelObjetivo();                 // Leemos velocidad objetivo segun potenciometro
      }
      if (velActual >= velObjetivo)       // Comprobamos si ha llegado a la velocidad objetivo
        Fase = CORRE;
      if (enEstacion())                   // Si ha llegado a la estacion frena el tren
        Fase = FRENA;
      break;
    case CORRE:
      leeVelObjetivo ();                  // Actualizamos velocidad segun potenciometro
      if (velActual != velObjetivo)
        setVelocidad (velObjetivo);
      if (enEstacion())                   // Si ha llegado a la estacion frena el tren
        Fase = FRENA;
      break;
    case FRENA:
      if ((millis() - AccDec) > (numPulsos * 8)) {  // si tiempo mayor de numPulsos de 8ms (122hz) 
        AccDec = millis();
        velActual = (velActual > 4) ? velActual - 4 : 0; // Disminuimos velocidad
        setVelocidad (velActual);
      }
      if (velActual == 0)                 // Si ha frenado, tenemos que esperar en la estacion
        Fase = ESPERA;
      break;
    case ESPERA:
      delay (tiempoEspera);               // Esperamos a que suban y bajen los pasajeros
      cambiaDireccion();                  // Vamos en direccion hacia la otra estacion
      Fase = ACELERA;
      break;
  }
}

void setVelocidad (int velocidad) {
  analogWrite (pwmA,velocidad);           // pins como PWM
  analogWrite (pwmB,velocidad);
  velActual = velocidad;
}

void setDireccion (int sentido) {
  digitalWrite (dirA, sentido);
  digitalWrite (dirB, sentido);
  dirActual = sentido;
}

void cambiaDireccion () {
  dirActual = (dirActual == deAaB) ? deBaA : deAaB ;
  setDireccion (dirActual);
}

void leeVelObjetivo() {
  int valor = analogRead (poti);          // lee potenciometro: valor de 0 a 1023
  valor = valor >> 4;                     // se divide entre 16. Son 64 escalones de velocidad PWM.
  velObjetivo = valor << 2;               // se multiplica por 4, para que este entre 0 y 255.
}

void leeAnalog () {
  SensorA = analogRead (snsA);            // Lee corriente del L298
  SensorB = analogRead (snsB);  
}

void leeSensores () {
  if (velActual > 0)                      // Solo si esta en marcha
    leeAnalog ();                         // lee corriente
}

void buscaTren () {
  setDireccion (deAaB);                   // direccion de A a B por defecto
  digitalWrite(pwmA,HIGH);                // activa tension salida. pins como salida digital
  digitalWrite(pwmB,HIGH);    
  delay (1);                              // esperamos un poco a que estabilize
  leeAnalog ();                           // Leemos la corriente
  setVelocidad (0);                       // pins como PWM y velocidad 0
  if (SensorB > minCorriente)             // Si esta en la estacion B, cambia la direccion
    setDireccion (deBaA);
}

bool enEstacion () {
  leeSensores();                          // Lee corriente del sensor segun direccion
  if ((dirActual == deAaB) && (SensorB > minCorriente))
    return (true);
  if ((dirActual == deBaA) && (SensorA > minCorriente))
    return (true);
  return (false);
}


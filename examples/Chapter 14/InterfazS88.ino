// Interfaz S88 con protocolo P50 (Maerklin 6050/6051) - Paco Cañada 2020


#define S88_MODULOS 2               // Numeros de modulos S88 de 16 entradas. Entre 1 y 31
#define S88_TIME 50                 // Temporizacion para S88. 50us como en NanoX-S88 de Paco

const int pinClock = 3;
const int pinLoad  = 4;
const int pinReset = 5;
const int pinData  = 6;

byte datosS88[62];                  // Buffer para los datos leidos (31 modulos * 2 bytes)

void setup() {
  pinMode (pinClock, OUTPUT);
  pinMode (pinLoad, OUTPUT);
  pinMode (pinReset, OUTPUT);
  pinMode (pinData, INPUT_PULLUP);
  
  digitalWrite (pinClock, LOW);     // para asegurarnos que las señales estas en el estado correcto
  digitalWrite (pinLoad, LOW);
  digitalWrite (pinReset,LOW);

  Serial.begin (2400, SERIAL_8N2);  // 2400 baudios, 2 stops bits, sin paridad
  for (int i=0; i<62; i++)          // Borramos buffer
    datosS88[i]=0;
}

void loop() {
  leeS88();
  if (Serial.available() > 0)       // Si hay caracteres en el puerto serie los interpretamos
    entradaSerie();
}

void iniS88 () {
  digitalWrite (pinClock, HIGH);  // clock a HIGH por defecto
  digitalWrite (pinLoad, HIGH);   // Activamos Load/PS
  delayMicroseconds (S88_TIME);
  digitalWrite (pinClock, LOW);
  delayMicroseconds (S88_TIME);  
  digitalWrite (pinClock, HIGH); 
  delayMicroseconds (S88_TIME);
  digitalWrite (pinLoad, LOW);    // Desactivamos Load
  digitalWrite (pinReset, HIGH);  // Pulso Reset
  delayMicroseconds (S88_TIME);
  digitalWrite (pinReset, LOW);
  delayMicroseconds (S88_TIME);
}

byte shiftInS88 () {
  byte value = 0;
  int i;
  for (i = 7; i >= 0; i--) {
    digitalWrite (pinClock, HIGH);        // Clock a HIGH y espera
    delayMicroseconds (S88_TIME);
    value |= digitalRead (pinData) << i;  // Leemos dato y guardamos bit
    digitalWrite (pinClock, LOW);         // Clock a LOW y espera
    delayMicroseconds (S88_TIME);
  }
  return (value);
}

void leeS88() {
  iniS88();                                   // Empezamos lectura
  for (int i=0; i< (S88_MODULOS * 2); i++)    // Leemos los modulos
    //datosS88[i] = shiftIn (pinData, pinClock, MSBFIRST);
    datosS88[i] = shiftInS88();
}

void leeDireccion() {
  while (Serial.available() == 0);    // Espera a que llegue el dato
    Serial.read();
}

void entradaSerie() {
  int c, n, i;
  
  c = Serial.read();                  // leemos comando
  if (c >= 1 && c <= 31)              // Control locomotoras
    leeDireccion();
  if (c == 33 || c == 34)             // Control accesorios
    leeDireccion();
  if (c >= 64 && c <= 85)             // Control funcion locomotoras F1..F4
    leeDireccion();
  if (c >= 129 && c <= 159) {         // Lectura de todos los S88, hasta el n
    n = (c - 128) * 2;
    for (i=0; i < n; i++)
      Serial.write (datosS88[i]);
  }
  if (c >= 193 && c <= 223) {         // Lectura de un solo S88, el n
    n = (c - 193) * 2;
    Serial.write (datosS88[n]);
    Serial.write (datosS88[n+1]);
  }
}




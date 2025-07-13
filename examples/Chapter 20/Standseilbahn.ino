  /* BRAWA Standseilbahn 6410 - Maquina de estados - F.Cañada 2020 */
  
  #define PIN_LUZ     3
  #define PIN_MOTOR1  4
  #define PIN_MOTOR2  5
  #define PIN_VELOCIDAD 7
  #define PIN_ALTAVOZ 13
  #define PIN_REED_ESTACION_ARRIBA 8
  #define PIN_REED_VIA_ARRIBA 9
  #define PIN_REED_VIA_ABAJO 10
  #define PIN_REED_ESTACION_ABAJO 11
  #define PIN_PULSADOR 12
  
  
  // FSM - Definicion Maquina de estados finitos
  
  int actualEstado;
  int proximoEstado;
  
  // Estados
  enum Estados {CABINA_ARRIBA, BAJANDO, CABINA_ABAJO, SUBIENDO};
  
  // Variables de contexto
  
  // Entradas
  #define NUM_VARIABLES 8
  
  enum Variables {PULSADOR, REED_ESTACION_ARRIBA, REED_ESTACION_ABAJO, REED_VIA_ARRIBA, REED_VIA_ABAJO, TIEMPO_A, TIEMPO_B, TIEMPO_C};
  
  struct {
    bool activo;                    // variable de contexto activa
    bool marcha;                    // Temporizador en marcha
    int pin;                        // Pin de entrada
    int ultimoNivel;                // Nivel del pin de entrada
    unsigned long tiempoInicial;    // Tiempo inicial
    unsigned long tiempoEspera;     // Tiempo de espera para activacion
  } Variable[NUM_VARIABLES];
  
  
  // Acciones
  void luzOn()  {digitalWrite(PIN_LUZ, LOW);}
  void luzOff() {digitalWrite(PIN_LUZ, HIGH);}
  void motorParar() {digitalWrite(PIN_MOTOR1, HIGH); digitalWrite(PIN_MOTOR2, HIGH);}
  void motorBajar() {digitalWrite(PIN_MOTOR1, LOW);  digitalWrite(PIN_MOTOR2, HIGH);}
  void motorSubir() {digitalWrite(PIN_MOTOR1, HIGH); digitalWrite(PIN_MOTOR2, LOW);}
  void motorLento() {digitalWrite(PIN_VELOCIDAD,HIGH);}
  void motorRapido() {digitalWrite(PIN_VELOCIDAD,LOW);}
  
  
  void setup() {
    iniEntrada(PULSADOR, PIN_PULSADOR);                           // definimos pin entrada
    iniEntrada(REED_ESTACION_ARRIBA, PIN_REED_ESTACION_ARRIBA);
    iniEntrada(REED_VIA_ARRIBA, PIN_REED_VIA_ARRIBA);
    iniEntrada(REED_VIA_ABAJO, PIN_REED_VIA_ABAJO);
    iniEntrada(REED_ESTACION_ABAJO, PIN_REED_ESTACION_ABAJO);
    pinMode(PIN_LUZ, OUTPUT);                                     // definimos pin salidas
    pinMode(PIN_MOTOR1, OUTPUT);
    pinMode(PIN_MOTOR2, OUTPUT);
    pinMode(PIN_VELOCIDAD, OUTPUT);
    motorParar();                                                 // inicializar salidas
    motorLento();
    luzOff();
    iniEstado(CABINA_ARRIBA);                                     // definimos estado inicial
  }
  
  void loop() {
    actualizaVariables();                                         // actualiza variables (entradas y temporizadores)
    ejecutaFSM();                                                 // ejecuta maquina de estados finitos
  }
  
  void iniEstado (int estado) {
    for (int i = 0; i < NUM_VARIABLES; i++) {                      // resetea temporizadores y estado
      Variable[i].marcha = false;
      Variable[i].activo = false;
    }
    actualEstado = estado;                                        // establece estado inicial
  }
  
  void iniEntrada (int entrada, int pin) {                        // definir pin entrada
    pinMode(pin, INPUT_PULLUP);
    Variable[entrada].pin = pin;
  }
  
  void detectaActivacion (int entrada) {                          // detecta pin entrada a LOW
    if (digitalRead(Variable[entrada].pin) == LOW)
      Variable[entrada].activo = true;
  }
  
  bool isFlanco (int entrada) {                                   // detecta flanco de subida o bajada en pin
    if (Variable[entrada].ultimoNivel != digitalRead(Variable[entrada].pin)) {
      if (Variable[entrada].ultimoNivel == HIGH)
        Variable[entrada].ultimoNivel = LOW;
      else
        Variable[entrada].ultimoNivel = HIGH;
      return (true);
    }
    return (false);
  }
  
  void detectaPulsacion (int entrada) {                           // detecta flanco de bajada
    if (isFlanco (entrada))
      if (Variable[entrada].ultimoNivel == LOW)
        Variable[entrada].activo = true;
  }
  
  void stopTemporizador (int num) {                                 // desactiva temporizador
    Variable[num].marcha = false;
  }
  
  void setTemporizador (int num, unsigned long espera) {            // establece temporizador
    Variable[num].tiempoEspera = espera;
    Variable[num].tiempoInicial = millis();
    Variable[num].marcha = true;
  }
  
  void resetVariables() {                                           // reseta variables y comprueba temporizadores
    for (int i = 0; i < NUM_VARIABLES; i++) {
      Variable[i].activo = false;
      if (Variable[i].marcha)
        if (millis() - Variable[i].tiempoInicial > Variable[i].tiempoEspera)
          Variable[i].activo = true;
    }
  }
  
  bool isTransicionVariable (int entrada, int estado) {              // comprueba la activacion de una variable hacia un estado
    if (Variable[entrada].activo == true) {
      proximoEstado = estado;
    }
    return (Variable[entrada].activo);
  }
  
  
  //------------------------------------------------------
  
  void actualizaVariables() {                                       // actualiza variables al inicio del ciclo
    resetVariables();
    detectaActivacion (REED_ESTACION_ARRIBA);
    detectaActivacion (REED_ESTACION_ABAJO);
    detectaActivacion (REED_VIA_ARRIBA);
    detectaActivacion (REED_VIA_ABAJO);
    detectaPulsacion (PULSADOR);
    delay(10);                                                      // espera para evitar rebotes
  }
  
  void ejecutaFSM () {                                              // maquina de estados finitos
    switch (actualEstado) {
      case CABINA_ARRIBA:
        if (isTransicionVariable(PULSADOR, CABINA_ARRIBA)) {         // comprueba transicion hacia un estado
          luzOn();                                                  // acciones
          setTemporizador (TIEMPO_A, 3000);
        }
        if (isTransicionVariable  (TIEMPO_A, BAJANDO)) {
          tone (PIN_ALTAVOZ, 277);
          delay(1000);
          noTone(PIN_ALTAVOZ);
          motorBajar();
          stopTemporizador(TIEMPO_A);
        }
        if (isTransicionVariable  (TIEMPO_C, CABINA_ARRIBA)) {
          luzOff();
          stopTemporizador(TIEMPO_C);
        }
        break;
      case BAJANDO:
        if (isTransicionVariable (REED_VIA_ARRIBA, BAJANDO)) {
          luzOff();
          motorRapido();
        }
        if (isTransicionVariable (REED_VIA_ABAJO, BAJANDO)) {
          motorLento();
          luzOn();
        }
        if (isTransicionVariable (REED_ESTACION_ABAJO, CABINA_ABAJO)) {
          motorParar();
          setTemporizador (TIEMPO_B, 6000);
        }
        break;
      case CABINA_ABAJO:
        if (isTransicionVariable  (TIEMPO_B, SUBIENDO)) {
          tone (PIN_ALTAVOZ, 277);
          delay(1000);
          noTone(PIN_ALTAVOZ);
          motorSubir();
        }
        break;
      case SUBIENDO:
        if (isTransicionVariable (REED_VIA_ABAJO, SUBIENDO)) {
          luzOff();
          motorRapido();
        }
        if (isTransicionVariable (REED_VIA_ARRIBA, SUBIENDO)) {
          motorLento();
          luzOn();
        }
        if (isTransicionVariable (REED_ESTACION_ARRIBA, CABINA_ARRIBA)) {
          motorParar();
          setTemporizador (TIEMPO_C, 3000);
        }
        break;
      default:                                                    // No deberiamos haber llegado hasta aqui
        iniEstado(CABINA_ARRIBA);
        motorParar();
        break;
    }
    actualEstado = proximoEstado;                                 // actualiza nuevo estado de la maquina
  }


/*
void detectaPulsacionCorta (int entrada, unsigned long tiempo) {  // detecta pulsacion corta inferior a tiempo
  if (isFlanco(entrada)) {
    if (Variable[entrada].ultimoNivel == LOW)
      Variable[entrada].tiempoInicial = millis();
    else {
      if (millis() - Variable[entrada].tiempoInicial < tiempo)
        Variable[entrada].activo = true;
    }
  }
}

void detectaPulsacionLarga (int entrada, unsigned long tiempo) {  // detecta pulsacion larga superior a tiempo
  if (isFlanco(entrada)) {
    if (Variable[entrada].ultimoNivel == LOW)
      Variable[entrada].tiempoInicial = millis();
    else {
      if (millis() - Variable[entrada].tiempoInicial > tiempo)
        Variable[entrada].activo = true;
    }
  }
}
*/


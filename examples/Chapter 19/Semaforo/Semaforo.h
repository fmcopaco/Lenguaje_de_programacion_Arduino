// Libreria Semaforo.h -- Paco Cañada 2020 -- https://usuaris.tinet.cat/fmco/

#ifndef SEMAFORO_H
#define SEMAFORO_H

#include <Arduino.h>

// Estructuras, variables, clases, prototipos de la libreria

#define FLASH_INTERVAL 1000                               // Periodo de tiempo para intermitencias
#define PWM 20                                            // Periodo PWM. Influye en el tiempo de encendido/apagado

#define ENCENDIDO HIGH                                    // Tension pin. Cambiar segun anodo/catodo comun
#define APAGADO LOW

enum aspectos  { PARADA, VIA_LIBRE, ANUNCIO_PRECAUCION,   // Aspectos RENFE
                 ANUNCIO_PARADA, REBASE_AUTORIZADO,
                 REBASE_AUTORIZADO_NO_PARAR, MOVIMIENTO_AUTORIZADO,
                 VIA_LIBRE_CONDICIONAL, ANUNCIO_PARADA_INMEDIATA,
                 PARADA_SELECTIVA, PASO_NIVEL_ABIERTO, PASO_NIVEL_CERRADO
               };

#define NUMLEDS 4                                         // Numero maximo de LED por señal

//--------------------------------------------------
enum aspectosDB { HP0, HP1, HP2, Sh1, HP0_Sh1, Vr0, Vr1, Vr2};     // Aspectos DB
//--------------------------------------------------

class Semaforo {
  public:
    // Constructor. Se llama al crear el objeto
    Semaforo ();

    // Metodos que puede llamar el usuario

    // Inicializacion de los pines usados por los semaforos
    void init (int pinRojo, int pinVerde, int pinAmarillo = -1, int pinBlanco = -1) ;
    void initManiobra (int pinRojo, int pinBlanco);
    void initPasoNivelVehiculos (int pinDerecha, int pinIzquierda);

    void process ();                                      // Procesa el semaforo.Llamar lo mas a menudo posible
    void aspecto (byte aspect);                           // Cambia el aspecto mostrado en el semaforo

    //--------------------------------------------------
    void initHauptSignal (int pinRojo, int pinVerde, int pinAmarillo = -1, int pinBlanco = -1);
    void initVorSignal (int pinVerde1, int pinVerde2, int pinAmarillo1, int pinAmarillo2);
    void aspectoDB (byte aspect);
    //--------------------------------------------------

  private:
    // Son invisibles para el usuario
    unsigned long _flashTime;                             // Temporizadores
    unsigned long _currTime;
    bool _flashFase;                                      // Fase actual del parpadeo
  
    struct LED {                                          // Estructura de control de un LED
      int pinLED;                                         // Pin al que esta conectado. -1 para sin conexion
      int estado;                                         // estado del LED
      int currPWM;                                        // Brillo actual
      int finPWM;                                         // Brillo final
      unsigned long interval;                             // Intervalo para el temporizador
      unsigned long timerPWM;                             // Temporizador
      bool fasePWM;                                       // Fase del control PWM
    };

    LED Leds[NUMLEDS];                                    // Array de control de LED

    enum color     {ROJO, VERDE, AMARILLO, BLANCO};       // Indices del array segun color
    enum estados   {OFF, ON, FLASH_A, FLASH_B};           // Estado del LED
    
    void fadePin (LED *led, unsigned long currTime);      // Controla el encendido/apagado progresivo del LED
   
};


#endif


// Control de una Pantalla OLED y reloj para panel de estacion - Paco Cañada 2020

#include "Wire.h"               // Libreria I2C
#include "uRTCLib.h"            // Libreria RTC DS3231    https://github.com/Naguissa/uRTCLib/blob/master/src/uRTCLib.h
#include "SSD1306Ascii.h"       // Librerias OLED SSD1306 https://github.com/greiman/SSD1306Ascii
#include "SSD1306AsciiWire.h"

uRTCLib rtc(0x68);                    // La direccion I2C del modulo de reloj DS3231

#define I2C_ADDRESS 0x3C              // la direccion I2C de la pantalla OLED SSD1306
#define RST_PIN -1
SSD1306AsciiWire oled;

int hora,minutos,segundos;            // variables para mostrar hora y mensaje
int ultSegundo=99;
bool muestraHora;
int mensaje;

TickerState state;                    // Mantiene puntero a la cola de mensajes en movimiento 
uint32_t tickTime = 0;

#define maxMensaje 5                  // Numero de mensajes en la pantalla. Se definen en el array text[]

  const char* text[] = {
  "RE  REGIONAL EXPRESS", "BARCELONA", "EFECTUA PARADA EN VILASECA, TARRAGONA, ALTAFULLA, TORREDEMBARRA, SANT VICENS Y VILANOVA",
  "AVE 03062", "MADRID ATOCHA", "EFECTUA PARADA EN TARRAGONA CAMP, LLEIDA PIRINEUS, ZARAGOZA DELICIAS Y GUADALAJARA",
  "RE  REGIONAL", "AEROPUERTO", "DIRECTO A AEROPUERTO",
  "T.HOTEL 10156","BARCELONA SANTS", "PARA ESTE TREN NO SON VALIDOS LOS BILLETES DE CERCANIAS",
  "HD DIURNO 17194", "BADAJOZ", "EFECTUA PARADA EN LEGANES Y LUEGO SIGUE RECTO" 
};

  
void setup() {
  Serial.begin(115200);               // Usamos 115200 baud para el monitor serie
  Serial.println ("Pantalla Reloj OLED by Paco");
  Serial.println ("? para ver comandos\n");

  Wire.begin();                       // Inicializacion puerto I2C. SDA = A4, SCL = A5
  Wire.setClock(400000L);
  
  rtc.refresh();                      // lee modulo RTC DS3231
  ultSegundo = rtc.second();
  muestraHora = true;

  oled.begin(&Adafruit128x32, I2C_ADDRESS);   // Inicializacion pantalla OLED 128x32 SSD1306
  oled.clear();
  oled.setFont(Verdana12_bold);
  oled.setCursor (22,1); 
  oled.print ("Design by Paco");
  delay (2000);
}

void loop() {
  if (Serial.available() > 0)         // Si hay caracteres en el puerto serie los interpretamos
    entradaSerie();
    
  rtc.refresh();                      // Actualizamos desde el reloj RTC la hora actual
  segundos = rtc.second();        
  if (muestraHora) {                  // Mostrar hora
    if ((ultSegundo != segundos)) {   // Cada nuevo segundo actualizamos la pantalla
      minutos = rtc.minute();
      hora = rtc.hour();
      ultSegundo = segundos;
      if ((segundos > 25) && (segundos < 35))
        oledTemp();                   // Temperatura
      else {
        oled.setFont(lcdnums14x24);   // Hora
        oled.clear();
        oled.setCursor (8,1);         // columna en pixels. fila en 8 filas de pixel.
        oledHora();
      }
    }
  }
  else {                              // Mostrar mensaje movil
    if (tickTime <= millis()) {
      tickTime = millis() + 30;
      int8_t rtn = oled.tickerTick(&state);
      if (rtn <= 0)                   // Si fin de mensaje, vuleve a cargarlo
        oled.tickerText(&state, text[(mensaje*3)-1]);
    }
  }
}


void entradaSerie () {
  int c = Serial.read ();             // Lee caracter del comando
  switch (c) {
    case '#':                         // Comando #HH:MM:SS poner en hora
      hora = Serial.parseInt();
      minutos = Serial.parseInt();
      segundos = Serial.parseInt();
      hora = constrain(hora, 0, 23);
      minutos = constrain(minutos, 0, 59);
      segundos = constrain(segundos, 0, 59);
      //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
      rtc.set(segundos, minutos, hora, 1, 23, 02, 20);
      rtc.refresh();
      Serial.print ("Nueva ");    
    case '@':                         // Comando @ leer hora
      Serial.print("Hora: ");
      Serial.print(rtc.hour());
      Serial.print(':');
      Serial.print(rtc.minute());
      Serial.print(':');
      Serial.println(rtc.second());  
      break;   
    case 'T':                         // Comando T lee temperatura
      Serial.print("Temp: ");
      Serial.print(rtc.temp());  
      Serial.println (" ºC");
      break;
    case '-':                         // Comando - muestra hora
      muestraHora = true;
      break;
    case '+':                         // Comando +N muestra mensaje N en la pantalla OLED
      mensaje = Serial.parseInt();
      mensaje = constrain (mensaje,1,maxMensaje);
      muestraHora = false;
      oled.clear ();
      oled.setFont(lcd5x7);
      oled.println (text[(mensaje*3)-3]);         // Primera linea mensaje. Tipo tren
      oled.setFont(Verdana12_bold);
      oled.println (text[(mensaje*3)-2]);         // Segunda linea mensaje. Destino
      oled.tickerInit(&state, Adafruit5x7, 3);    // mensaje movil en linea 3
      break;
    case '\n':                        // filtra caracter nueva linea
    case '\r':                        // filtra caracter retorno del carro
      break;
    case '?':                         // Comando ? ayuda
    default:                          // Cualquier otro comando no soportado muestra ayuda
      Serial.println (F("Introduzca uno de estos comandos:"));
      Serial.println (F("@         Lee hora actual"));
      Serial.println (F("T         Lee temperatura actual"));
      Serial.println (F("#HH:MM:SS Poner en hora"));
      Serial.println (F("+N        Muestra mensaje N en la pantalla"));
      Serial.println (F("-         Muestra hora en la pantalla"));
      Serial.println (F("?         Esta ayuda\n"));
      break;
  }
}

void oledHora() {
  if (hora<10)                          // muestra hora actual en la pantalla OLED
    oled.print ("0");
  oled.print(hora);
  (minutos < 10)  ? oled.print (":0") : oled.print (":");
  oled.print(minutos);
  (segundos < 10) ? oled.print (":0") : oled.print (":");
  oled.print(segundos);
  }

void oledTemp () {
  oled.setFont(Verdana12_bold);         // muestra temperatura en pantalla OLED
  oled.clear();
  oled.setCursor (40,0);                // columna en pixels. fila en 8 filas de pixel.
  oled.set2X ();
  oled.print(int (rtc.temp()));  
  oled.print(" 'C");
  oled.set1X ();
  oled.setCursor (44,3); 
  oled.setFont(lcd5x7);
  oledHora();
}


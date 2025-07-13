#include <Servo.h>

Servo myservo;                      // Creamos el objeto llamado myservo

int potpin = A0;                    // pin del potenciometro al pin A0
int val;                            // Se crea una variable para un buen desarrollo

void setup() {
  myservo.attach(9);                // usamos el pin PWM 9
}

void loop() {
  val = analogRead(potpin);         // leemos el pin A0
  val = map(val, 0, 1023, 0, 180);  // usamos la funcion map() para convertir los valores
  myservo.write(val);               // escribimos en el servo el valor obtenido
  delay(15);                        // retardo recomendado
}

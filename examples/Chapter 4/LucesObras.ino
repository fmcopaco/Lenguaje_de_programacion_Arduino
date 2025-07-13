int tiempo = 100; // Cuanto mas alto, mas lento el efecto
int pin;

void setup() {
  for (pin = 2; pin < 8; pin++) {
    pinMode (pin, OUTPUT);
  }
}

void loop() {
for (pin = 2; pin < 8; pin++) {
  digitalWrite (pin, HIGH);
  delay (tiempo);
  digitalWrite (pin,LOW);
  }
  delay (1000);
}

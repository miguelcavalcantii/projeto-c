// TESTE 02 - LEDs
// Ligacao: GPIO 32 -> resistor 220-330 ohm -> perna longa (+) do LED verde; perna curta (-) no GND.
//          GPIO 33 -> resistor 220-330 ohm -> perna longa (+) do LED vermelho; perna curta (-) no GND.
// Esperado: os LEDs piscam alternando a cada 0,5 s.

#define PIN_LED_GREEN 32
#define PIN_LED_RED   33

bool verdeAceso = true;
unsigned long ultimaTroca = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  Serial.println("=== TESTE 02 - LEDs ===");
}

void loop() {
  // millis() em vez de delay(): o programa nao fica travado esperando
  if (millis() - ultimaTroca >= 500) {
    ultimaTroca = millis();
    verdeAceso = !verdeAceso;
    digitalWrite(PIN_LED_GREEN, verdeAceso ? HIGH : LOW);
    digitalWrite(PIN_LED_RED,   verdeAceso ? LOW  : HIGH);
    Serial.println(verdeAceso ? "Verde ACESO" : "Vermelho ACESO");
  }
}

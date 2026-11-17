// TESTE 04 - Joystick analogico
// Ligacao: VRx -> GPIO 34, VRy -> GPIO 35, +5V/VCC do modulo -> 3V3 (NAO 5V!), GND -> GND.
// O ADC da ESP32 vai de 0 a 4095. Parado, o joystick deve ficar perto de ~1800-2000.
// Esperado: valores mudam ao mexer e a direcao aparece corretamente.
// Anote os valores parado / extremos no relatorio de testes.

#define PIN_JOY_X 34
#define PIN_JOY_Y 35

// Zona morta: valores perto do centro sao ignorados
#define LIMITE_BAIXO 1000
#define LIMITE_ALTO  3000

unsigned long ultimoPrint = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("=== TESTE 04 - Joystick ===");
}

void loop() {
  if (millis() - ultimoPrint >= 200) {
    ultimoPrint = millis();
    int x = analogRead(PIN_JOY_X);
    int y = analogRead(PIN_JOY_Y);

    String direcao = "CENTRO";
    if (x < LIMITE_BAIXO) direcao = "ESQUERDA";
    else if (x > LIMITE_ALTO) direcao = "DIREITA";
    else if (y < LIMITE_BAIXO) direcao = "CIMA";
    else if (y > LIMITE_ALTO) direcao = "BAIXO";
    // Obs.: dependendo de como o modulo esta virado, os nomes podem
    // sair trocados. Isso e normal; a gente ajusta depois.

    Serial.print("X=");
    Serial.print(x);
    Serial.print("  Y=");
    Serial.print(y);
    Serial.print("  -> ");
    Serial.println(direcao);
  }
}

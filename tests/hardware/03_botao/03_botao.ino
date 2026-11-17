// TESTE 03 - Botao com debounce
// Ligacao: uma perna do botao no GPIO 25, a outra no GND (sem resistor externo).
// Usa o pull-up interno: solto = HIGH, apertado = LOW.
// Esperado: cada aperto conta UMA vez (sem contagens duplicadas).

#define PIN_BUTTON 25
#define DEBOUNCE_MS 40

int estadoEstavel = HIGH;      // ultimo estado confirmado
int ultimaLeitura = HIGH;      // ultima leitura bruta
unsigned long inicioMudanca = 0;
int contador = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.println("=== TESTE 03 - Botao ===");
  Serial.println("Aperte o botao algumas vezes.");
}

void loop() {
  int leitura = digitalRead(PIN_BUTTON);

  // Se a leitura mudou, reinicia a contagem de tempo
  if (leitura != ultimaLeitura) {
    inicioMudanca = millis();
    ultimaLeitura = leitura;
  }

  // So aceita a mudanca se ela ficou estavel por DEBOUNCE_MS
  if (millis() - inicioMudanca >= DEBOUNCE_MS && leitura != estadoEstavel) {
    estadoEstavel = leitura;
    if (estadoEstavel == LOW) {   // acabou de ser apertado
      contador++;
      Serial.print("Botao apertado. Total: ");
      Serial.println(contador);
    }
  }
}

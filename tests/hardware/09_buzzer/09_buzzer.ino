// TESTE 09 - Buzzer
// Ligacao: perna + (a mais comprida) no GPIO 32, perna - no GND.
// Esperado: a cada 3 segundos toca um som diferente, na ordem
//   1) bi-bip agudo (liberado)  2) bip grave longo (negado)
//   3) tres bips (alerta)       4) sirene de 2 s (bloqueio)
// Se nao tocar nada: confira as pernas (+ no 32) e o GND.
// Se tocar so um "click" ou um chiado igual em todos: o buzzer e
// ATIVO; no SENTINEL, mude BUZZER_ATIVO para true no config.h.

#define PIN_BUZZER 32

void bip(int freq, int ms) {
  tone(PIN_BUZZER, freq);
  delay(ms);
  noTone(PIN_BUZZER);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.println("=== TESTE 09 - Buzzer (GPIO 32) ===");
}

void loop() {
  Serial.println("1) Liberado: bi-bip agudo");
  bip(1500, 100); delay(60); bip(2000, 150);
  delay(3000);

  Serial.println("2) Negado: bip grave longo");
  bip(400, 500);
  delay(3000);

  Serial.println("3) Alerta: 3 bips");
  for (int i = 0; i < 3; i++) { bip(1000, 150); delay(100); }
  delay(3000);

  Serial.println("4) Bloqueio: sirene");
  for (int i = 0; i < 4; i++) { bip(800, 250); bip(1300, 250); }
  delay(3000);
}

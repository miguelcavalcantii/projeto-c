// TESTE 01 - ESP32 basico
// Objetivo: confirmar que a placa grava, roda e se comunica pelo Serial.
// Nao precisa de nenhum componente ligado.
// Monitor Serial: 115200 baud.

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== TESTE 01 - ESP32 ===");
  Serial.print("Modelo do chip: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Revisao: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("Nucleos: ");
  Serial.println(ESP.getChipCores());
  Serial.print("Frequencia da CPU (MHz): ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Flash (bytes): ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("RAM livre (bytes): ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // Imprime o tempo ligado a cada 2 s, sem usar delay()
  static unsigned long ultimo = 0;
  if (millis() - ultimo >= 2000) {
    ultimo = millis();
    Serial.print("Ligado ha ");
    Serial.print(millis() / 1000);
    Serial.println(" s");
  }
}

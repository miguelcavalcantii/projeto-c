// TESTE 05 - Scanner I2C
// Objetivo: descobrir o endereco do LCD (normalmente 0x27 ou 0x3F)
// e verificar se ha outros dispositivos no barramento (ex.: display embutido da placa).
// Ligacao: SDA -> GPIO 21, SCL -> GPIO 22 (ver docs sobre nivel logico 5 V!).

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(21, 22); // SDA, SCL
  Serial.println("=== TESTE 05 - Scanner I2C ===");
}

void loop() {
  int encontrados = 0;
  Serial.println("Procurando...");
  for (int endereco = 1; endereco < 127; endereco++) {
    Wire.beginTransmission(endereco);
    if (Wire.endTransmission() == 0) {  // 0 = dispositivo respondeu
      Serial.print("Dispositivo encontrado em 0x");
      if (endereco < 16) Serial.print("0");
      Serial.println(endereco, HEX);
      encontrados++;
    }
  }
  if (encontrados == 0) {
    Serial.println("Nenhum dispositivo. Confira fios, alimentacao 5V do LCD e GND comum.");
  }
  Serial.println();
  delay(3000); // aqui pode usar delay: e so um scanner
}

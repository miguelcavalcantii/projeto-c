// TESTE 07 - RFID RC522
// Biblioteca: "MFRC522" (GithubCommunity / miguelbalboa) - Gerenciador de Bibliotecas.
// Ligacao: SDA/SS->27, SCK->18, MOSI->23, MISO->19, RST->26, 3.3V->3V3 (NUNCA 5V), GND->GND. IRQ fica solto.
// Esperado:
//   1) "Versao do chip" = 0x91 ou 0x92 (original) ou 0x88/0xB2 (clone, costuma funcionar).
//      0x00 ou 0xFF = problema de ligacao.
//   2) Ao aproximar um cartao, aparecem o UID e o tipo.
// ATENCAO: o UID NAO e segredo. Este teste so prova que a leitura funciona.

#include <SPI.h>
#include <MFRC522.h>

#define PIN_SS   27
#define PIN_RST  26
#define PIN_SCK  18
#define PIN_MISO 19
#define PIN_MOSI 23

MFRC522 rfid(PIN_SS, PIN_RST);

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);   // ganho maximo da antena
  Serial.println("=== TESTE 07 - RFID RC522 ===");

  byte versao = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("Versao do chip: 0x");
  Serial.println(versao, HEX);
  if (versao == 0x00 || versao == 0xFF) {
    Serial.println("ERRO: RC522 nao respondeu. Confira os fios e a alimentacao 3.3V.");
  } else {
    Serial.println("RC522 respondeu. Aproxime um cartao...");
    // Mostra a cada 2 s que o leitor continua vivo (ajuda a achar mau contato)
  }
}

unsigned long ultimoAviso = 0;

void loop() {
  if (millis() - ultimoAviso >= 2000) {
    ultimoAviso = millis();
    byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.print("... procurando cartao (versao 0x");
    Serial.print(v, HEX);
    Serial.println(v == 0x00 || v == 0xFF ? ") <- LEITOR PAROU DE RESPONDER: mau contato?" : ")");
  }
  // Sem cartao novo? Sai e tenta de novo na proxima volta do loop
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" (");
  Serial.print(rfid.uid.size);
  Serial.println(" bytes)");

  MFRC522::PICC_Type tipo = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("Tipo: ");
  Serial.println(rfid.PICC_GetTypeName(tipo));
  Serial.println();

  rfid.PICC_HaltA();       // manda o cartao "dormir" para nao repetir a leitura
  rfid.PCD_StopCrypto1();
}

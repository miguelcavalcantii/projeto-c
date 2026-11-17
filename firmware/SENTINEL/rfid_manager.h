// =============================================================
// SENTINEL - rfid_manager
// Responsabilidade: conversar com o leitor RFID RC522.
//
// - Funciona COM ou SEM o leitor: se ele nao responder, o sistema segue
//   normal (so a parte de cartao fica indisponivel).
// - A cada 10 s confere se o leitor ainda responde. Detecta se ele foi
//   desligado (falha de comunicacao) ou plugado com a placa ligada.
// - O UID do cartao NUNCA e guardado nem enviado. O sistema usa um
//   "id protegido" = HMAC-SHA256(chave da placa, UID), em hexadecimal.
//
// LIMITACAO IMPORTANTE (docs/04 e docs/09): o UID nao e segredo e pode
// ser copiado. Por isso o cartao e so o 1o fator: sempre pede o PIN.
// =============================================================
#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>

bool rfidIniciar();
void rfidAtualizar();               // chamar em TODA volta do loop() (autoteste)

bool rfidPresente();
byte rfidVersao();                  // 0x91/0x92 = original, 0x88/0xB2 = clone

// true quando um cartao NOVO foi lido. Depois use rfidUltimoId().
bool rfidCartaoDetectado();
String rfidUltimoId();              // id protegido (16 caracteres hex)
String rfidUltimoTipo();            // ex.: "MIFARE 1KB"
int rfidUltimoTamanhoUid();         // 4 ou 7 bytes (para a explicacao de clonagem)

bool rfidPerdeuConexao();           // true uma vez, quando o leitor para de responder
bool rfidReconectou();              // true uma vez, quando o leitor volta/e plugado

#endif

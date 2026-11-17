// =============================================================
// SENTINEL - crypto_manager
// Responsabilidade: as operacoes criptograficas do projeto.
// Usa a biblioteca mbedTLS, que ja vem junto com a placa ESP32.
//
// ETAPA 3: PBKDF2-HMAC-SHA256 para proteger a senha.
// ETAPA 5: o HMAC daqui sera reaproveitado na auditoria.
// =============================================================
#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>

#define HASH_BYTES 32   // SHA-256 sempre gera 32 bytes

// Preenche o vetor com bytes aleatorios do gerador de hardware da ESP32
void cryptoGerarAleatorio(byte saida[], int tamanho);

// PBKDF2-HMAC-SHA256: transforma senha + salt em um hash de 32 bytes.
// "iteracoes" = quantas vezes o HMAC e repetido (deixa a forca bruta lenta).
void cryptoPbkdf2(const byte senha[], int tamSenha,
                  const byte salt[], int tamSalt,
                  unsigned long iteracoes, byte saida[HASH_BYTES]);

// HMAC-SHA256 simples: "lacre" de 32 bytes que so quem tem a chave consegue gerar
void cryptoHmacSha256(const byte chave[], int tamChave,
                      const byte dados[], int tamDados, byte saida[HASH_BYTES]);

// Converte bytes em texto hexadecimal (ex.: {0xAB, 0x01} -> "ab01")
String cryptoParaHex(const byte dados[], int tamanho);

// Compara dois vetores sempre no MESMO tempo, mesmo se forem diferentes
// logo no primeiro byte (evita o "ataque de tempo").
bool cryptoCompararSeguro(const byte a[], const byte b[], int tamanho);

// Apaga um vetor da memoria (preenche com zeros)
void cryptoApagar(byte dados[], int tamanho);

#endif

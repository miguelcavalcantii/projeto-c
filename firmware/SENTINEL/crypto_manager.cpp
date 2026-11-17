#include "crypto_manager.h"
#include "mbedtls/md.h"
#include "esp_random.h"

void cryptoGerarAleatorio(byte saida[], int tamanho) {
  // Gerador de numeros aleatorios do HARDWARE da ESP32.
  // Observacao (documentada): ele e mais forte com o Wi-Fi/Bluetooth ligado.
  esp_fill_random(saida, tamanho);
}

// -------------------------------------------------------------
// PBKDF2 (RFC 8018) com HMAC-SHA256, gerando 32 bytes (1 bloco).
//
//   U1 = HMAC(senha, salt + 00 00 00 01)
//   U2 = HMAC(senha, U1)
//   U3 = HMAC(senha, U2)  ... ate U(iteracoes)
//   resultado = U1 XOR U2 XOR U3 XOR ... XOR U(iteracoes)
//
// Cada tentativa de adivinhar a senha custa "iteracoes" HMACs.
// Com 10.000 iteracoes, testar todas as 10.000 senhas de 4 digitos
// custa 100 milhoes de HMACs em vez de 10 mil.
// -------------------------------------------------------------
void cryptoPbkdf2(const byte senha[], int tamSenha,
                  const byte salt[], int tamSalt,
                  unsigned long iteracoes, byte saida[HASH_BYTES]) {
  byte u[HASH_BYTES];
  byte numeroBloco[4] = {0, 0, 0, 1};   // bloco numero 1

  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); // 1 = modo HMAC
  mbedtls_md_hmac_starts(&ctx, senha, tamSenha);   // a chave do HMAC e a senha

  // U1 = HMAC(senha, salt + numero do bloco)
  mbedtls_md_hmac_update(&ctx, salt, tamSalt);
  mbedtls_md_hmac_update(&ctx, numeroBloco, 4);
  mbedtls_md_hmac_finish(&ctx, u);
  for (int i = 0; i < HASH_BYTES; i++) saida[i] = u[i];

  // U2 ... Un: cada um e o HMAC do anterior, e todos sao somados com XOR
  for (unsigned long n = 1; n < iteracoes; n++) {
    mbedtls_md_hmac_reset(&ctx);             // reaproveita a mesma chave
    mbedtls_md_hmac_update(&ctx, u, HASH_BYTES);
    mbedtls_md_hmac_finish(&ctx, u);
    for (int i = 0; i < HASH_BYTES; i++) saida[i] ^= u[i];
  }

  mbedtls_md_free(&ctx);
  cryptoApagar(u, HASH_BYTES);
}

void cryptoHmacSha256(const byte chave[], int tamChave,
                      const byte dados[], int tamDados, byte saida[HASH_BYTES]) {
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                  chave, tamChave, dados, tamDados, saida);
}

String cryptoParaHex(const byte dados[], int tamanho) {
  const char digitosHex[] = "0123456789abcdef";
  String texto = "";
  for (int i = 0; i < tamanho; i++) {
    texto += digitosHex[dados[i] >> 4];     // 4 bits de cima
    texto += digitosHex[dados[i] & 0x0F];   // 4 bits de baixo
  }
  return texto;
}

bool cryptoCompararSeguro(const byte a[], const byte b[], int tamanho) {
  // Em vez de parar no primeiro byte diferente, olha TODOS os bytes.
  // Assim o tempo de resposta nao revela "quantos bytes acertou".
  byte diferenca = 0;
  for (int i = 0; i < tamanho; i++) diferenca |= (a[i] ^ b[i]);
  return diferenca == 0;
}

void cryptoApagar(byte dados[], int tamanho) {
  // "volatile" impede o compilador de "otimizar" e pular a limpeza
  volatile byte* p = dados;
  for (int i = 0; i < tamanho; i++) p[i] = 0;
}

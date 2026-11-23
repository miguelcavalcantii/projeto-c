#include "crypto_manager.h"
#include "mbedtls/md.h"
#include "esp_random.h"

void cryptoGerarAleatorio(byte saida[], int tamanho) {
  // Gerador de numeros aleatorios do HARDWARE da ESP32.
  // Observacao (documentada): ele e mais forte com o Wi-Fi/Bluetooth ligado.
  esp_fill_random(saida, tamanho);
}

// -------------------------------------------------------------
// SHA-256 de duas partes juntas: SHA256(parte1 + parte2).
// Usa so o "resumo" (hash) do mbedTLS, que existe em TODAS as
// versoes da placa ESP32. O HMAC e montado por nos logo abaixo.
// (As funcoes prontas de HMAC sumiram nas versoes novas da placa.)
// -------------------------------------------------------------
static void sha256DuasPartes(const byte parte1[], int tam1,
                             const byte parte2[], int tam2, byte saida[HASH_BYTES]) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0); // 0 = so hash
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, parte1, tam1);
  if (tam2 > 0) mbedtls_md_update(&ctx, parte2, tam2);
  mbedtls_md_finish(&ctx, saida);
  mbedtls_md_free(&ctx);
}

// -------------------------------------------------------------
// HMAC-SHA256 (RFC 2104), feito "na mao":
//
//   HMAC(chave, dados) = SHA256( (chave XOR opad) + SHA256( (chave XOR ipad) + dados ) )
//
//   ipad = 64 bytes 0x36   |   opad = 64 bytes 0x5c
//   Chave maior que 64 bytes vira o SHA256 dela antes.
// -------------------------------------------------------------
void cryptoHmacSha256(const byte chave[], int tamChave,
                      const byte dados[], int tamDados, byte saida[HASH_BYTES]) {
  byte k[64];
  for (int i = 0; i < 64; i++) k[i] = 0;
  if (tamChave > 64) {
    sha256DuasPartes(chave, tamChave, chave, 0, k);   // chave grande: usa o hash dela
  } else {
    for (int i = 0; i < tamChave; i++) k[i] = chave[i];
  }

  byte pad[64];
  byte interno[HASH_BYTES];

  // 1) parte de dentro: SHA256((k XOR 0x36) + dados)
  for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x36;
  sha256DuasPartes(pad, 64, dados, tamDados, interno);

  // 2) parte de fora: SHA256((k XOR 0x5c) + resultado de dentro)
  for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x5c;
  sha256DuasPartes(pad, 64, interno, HASH_BYTES, saida);

  // Limpa tudo que tinha a ver com a chave (a senha!)
  cryptoApagar(k, 64);
  cryptoApagar(pad, 64);
  cryptoApagar(interno, HASH_BYTES);
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

  // U1 = HMAC(senha, salt + numero do bloco 00 00 00 01)
  byte saltComBloco[64 + 4];
  if (tamSalt > 64) tamSalt = 64;          // nosso salt tem 16 bytes
  for (int i = 0; i < tamSalt; i++) saltComBloco[i] = salt[i];
  saltComBloco[tamSalt + 0] = 0;
  saltComBloco[tamSalt + 1] = 0;
  saltComBloco[tamSalt + 2] = 0;
  saltComBloco[tamSalt + 3] = 1;
  cryptoHmacSha256(senha, tamSenha, saltComBloco, tamSalt + 4, u);
  for (int i = 0; i < HASH_BYTES; i++) saida[i] = u[i];

  // U2 ... Un: cada um e o HMAC do anterior, e todos sao somados com XOR
  for (unsigned long n = 1; n < iteracoes; n++) {
    cryptoHmacSha256(senha, tamSenha, u, HASH_BYTES, u);
    for (int i = 0; i < HASH_BYTES; i++) saida[i] ^= u[i];
  }

  cryptoApagar(u, HASH_BYTES);
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

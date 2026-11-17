#include <Preferences.h>
#include "security_config.h"
#include "crypto_manager.h"
#include "authentication.h"

static Preferences nvs;                       // acesso a memoria permanente (NVS)
static bool senhaCadastrada[MAX_USUARIOS];    // copia na RAM: quem ja tem senha
static unsigned long ultimoTempoMs = 0;

// Nome das chaves na NVS. O usuario 0 usa os nomes antigos ("salt", "hash")
// para que a senha criada nas versoes anteriores continue valendo.
//   usuario 0 -> "salt",  usuario 2 -> "salt2"
static String chaveNvs(String base, int usuario) {
  if (usuario == 0) return base;
  return base + String(usuario);
}

// Converte os digitos {1,2,3,4} no texto "1234" (em bytes)
static void digitosParaBytes(const int digitos[], byte texto[]) {
  for (int i = 0; i < PIN_LENGTH; i++) texto[i] = '0' + digitos[i];
}

// Calcula o PBKDF2 e mede quanto tempo levou
static void calcularHash(const int digitos[], const byte salt[], unsigned long iteracoes, byte hash[]) {
  byte texto[PIN_LENGTH];
  digitosParaBytes(digitos, texto);
  unsigned long inicio = millis();
  cryptoPbkdf2(texto, PIN_LENGTH, salt, KDF_SALT_BYTES, iteracoes, hash);
  ultimoTempoMs = millis() - inicio;
  cryptoApagar(texto, PIN_LENGTH);   // tira a senha da memoria
}

void authIniciar() {
  nvs.begin("sentinel", true);       // true = so leitura
  for (int u = 0; u < MAX_USUARIOS; u++) {
    senhaCadastrada[u] = (nvs.getBytesLength(chaveNvs("hash", u).c_str()) == KDF_HASH_BYTES &&
                          nvs.getBytesLength(chaveNvs("salt", u).c_str()) == KDF_SALT_BYTES);
  }
  nvs.end();
}

bool authSenhaCadastrada(int usuario) {
  if (usuario < 0 || usuario >= MAX_USUARIOS) return false;
  return senhaCadastrada[usuario];
}

bool authCadastrarSenha(int usuario, const int digitos[]) {
  if (usuario < 0 || usuario >= MAX_USUARIOS) return false;
  byte salt[KDF_SALT_BYTES];
  byte hash[KDF_HASH_BYTES];

  cryptoGerarAleatorio(salt, KDF_SALT_BYTES);      // salt novo a cada cadastro
  calcularHash(digitos, salt, KDF_ITERATIONS, hash);

  nvs.begin("sentinel", false);      // false = leitura e escrita
  bool ok = nvs.putBytes(chaveNvs("salt", usuario).c_str(), salt, KDF_SALT_BYTES) == KDF_SALT_BYTES;
  ok = ok && nvs.putBytes(chaveNvs("hash", usuario).c_str(), hash, KDF_HASH_BYTES) == KDF_HASH_BYTES;
  ok = ok && nvs.putULong(chaveNvs("iter", usuario).c_str(), KDF_ITERATIONS) > 0;
  nvs.end();

  cryptoApagar(hash, KDF_HASH_BYTES);
  if (ok) senhaCadastrada[usuario] = true;
  return ok;
}

bool authVerificarSenha(int usuario, const int digitos[]) {
  if (!authSenhaCadastrada(usuario)) return false;

  byte salt[KDF_SALT_BYTES];
  byte hashSalvo[KDF_HASH_BYTES];
  byte hashDigitado[KDF_HASH_BYTES];

  nvs.begin("sentinel", true);
  nvs.getBytes(chaveNvs("salt", usuario).c_str(), salt, KDF_SALT_BYTES);
  nvs.getBytes(chaveNvs("hash", usuario).c_str(), hashSalvo, KDF_HASH_BYTES);
  unsigned long iteracoes = nvs.getULong(chaveNvs("iter", usuario).c_str(), KDF_ITERATIONS);
  nvs.end();

  // Refaz o calculo com o MESMO salt e compara os dois hashes
  calcularHash(digitos, salt, iteracoes, hashDigitado);
  bool igual = cryptoCompararSeguro(hashSalvo, hashDigitado, KDF_HASH_BYTES);

  cryptoApagar(hashSalvo, KDF_HASH_BYTES);
  cryptoApagar(hashDigitado, KDF_HASH_BYTES);
  return igual;
}

unsigned long authUltimoTempoMs() {
  return ultimoTempoMs;
}

unsigned long authIteracoes() {
  return KDF_ITERATIONS;
}

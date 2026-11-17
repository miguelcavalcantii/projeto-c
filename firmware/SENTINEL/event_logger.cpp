#include <LittleFS.h>
#include <Preferences.h>
#include "security_config.h"
#include "crypto_manager.h"
#include "event_logger.h"

#define ARQUIVO_LOG   "/audit.log"
#define ARQUIVO_TEMP  "/audit.tmp"
#define TOTAL_CAMPOS  10

static Preferences nvs;
static byte chave[32];
static bool pronto = false;

static unsigned long ultimoId = 0;
static String ultimoHash = "";
static int totalRegistros = 0;
static unsigned long ancoraId = 0;      // id do ultimo registro antes do arquivo atual
static String ancoraHash = "";          // hash desse registro (o "elo" com o passado)
static unsigned long numeroBoot = 0;
static unsigned long epoca = 0;

// 64 zeros: o "hash anterior" do primeiro registro de todos
static String hashZero() {
  String z = "";
  for (int i = 0; i < 64; i++) z += "0";
  return z;
}

// Tira ';' e quebras de linha (eles separam campos e registros)
static String limparCampo(String texto, int tamanhoMax) {
  texto.replace(";", ",");
  texto.replace("\n", " ");
  texto.replace("\r", " ");
  if ((int)texto.length() > tamanhoMax) texto = texto.substring(0, tamanhoMax);
  return texto;
}

// Pega o campo numero "indice" de uma linha separada por ';'
static String pegarCampo(String linha, int indice) {
  int atual = 0;
  int inicio = 0;
  for (unsigned int i = 0; i <= linha.length(); i++) {
    if (i == linha.length() || linha[i] == ';') {
      if (atual == indice) return linha.substring(inicio, i);
      atual++;
      inicio = i + 1;
    }
  }
  return "";
}

static int contarCampos(String linha) {
  int total = 1;
  for (unsigned int i = 0; i < linha.length(); i++) {
    if (linha[i] == ';') total++;
  }
  return total;
}

// HMAC do "corpo" do registro (tudo menos o hash atual), em hexadecimal
static String calcularHash(String corpo) {
  byte resultado[HASH_BYTES];
  cryptoHmacSha256(chave, 32, (const byte*)corpo.c_str(), corpo.length(), resultado);
  return cryptoParaHex(resultado, HASH_BYTES);
}

static void salvarAncora() {
  nvs.begin("sentinel_aud", false);
  nvs.putULong("anc_id", ancoraId);
  nvs.putString("anc_hash", ancoraHash);
  nvs.putULong("epoca", epoca);
  nvs.end();
}

bool auditIniciar() {
  // true = se o sistema de arquivos ainda nao existir, formata (so na 1a vez)
  if (!LittleFS.begin(true)) {
    pronto = false;
    return false;
  }

  nvs.begin("sentinel_aud", false);
  // Chave do HMAC: criada uma unica vez, com o gerador aleatorio do hardware
  if (nvs.getBytesLength("chave") != 32) {
    cryptoGerarAleatorio(chave, 32);
    nvs.putBytes("chave", chave, 32);
  } else {
    nvs.getBytes("chave", chave, 32);
  }
  ancoraId = nvs.getULong("anc_id", 0);
  ancoraHash = nvs.getString("anc_hash", hashZero());
  epoca = nvs.getULong("epoca", 0);
  numeroBoot = nvs.getULong("boot", 0) + 1;     // conta quantas vezes a placa ligou
  nvs.putULong("boot", numeroBoot);
  nvs.end();

  // Le o arquivo para descobrir o ultimo registro (de onde a corrente continua)
  ultimoId = ancoraId;
  ultimoHash = ancoraHash;
  totalRegistros = 0;
  File arquivo = LittleFS.open(ARQUIVO_LOG, "r");
  if (arquivo) {
    while (arquivo.available()) {
      String linha = arquivo.readStringUntil('\n');
      linha.trim();
      if (linha.length() == 0) continue;
      totalRegistros++;
      ultimoId = pegarCampo(linha, 0).toInt();
      ultimoHash = pegarCampo(linha, 9);
    }
    arquivo.close();
  }

  pronto = true;
  return true;
}

bool auditPronto() { return pronto; }

unsigned long auditRegistrar(String categoria, String tipo, String severidade,
                             String usuario, String detalhe) {
  if (!pronto) return 0;

  // Arquivo cheio: "vira a pagina". O ultimo hash vira a ancora do proximo
  // arquivo, entao a corrente continua ligada ao passado.
  // (Num produto real, aqui o arquivo antigo seria enviado para um servidor.)
  if (totalRegistros >= AUDIT_MAX_REGISTROS) {
    ancoraId = ultimoId;
    ancoraHash = ultimoHash;
    salvarAncora();
    LittleFS.remove(ARQUIVO_LOG);
    totalRegistros = 0;
  }

  unsigned long id = ultimoId + 1;
  String corpo = String(id) + ";" + String(numeroBoot) + ";" + String(millis() / 1000) + ";" +
                 limparCampo(categoria, 12) + ";" + limparCampo(tipo, 28) + ";" +
                 limparCampo(severidade, 8) + ";" + limparCampo(usuario, 12) + ";" +
                 limparCampo(detalhe, 48) + ";" + ultimoHash;
  String hash = calcularHash(corpo);

  File arquivo = LittleFS.open(ARQUIVO_LOG, "a");
  if (!arquivo) return 0;
  arquivo.print(corpo + ";" + hash + "\n");
  arquivo.close();

  ultimoId = id;
  ultimoHash = hash;
  totalRegistros++;
  return id;
}

ResultadoVerificacao auditVerificar() {
  ResultadoVerificacao r;
  r.integra = true;
  r.totalVerificados = 0;
  r.idComErro = 0;
  r.motivo = "ok";
  if (!pronto) {
    r.integra = false;
    r.motivo = "LittleFS indisponivel";
    return r;
  }

  String hashEsperado = ancoraHash;       // o 1o registro tem que apontar para a ancora
  unsigned long idEsperado = ancoraId + 1;

  File arquivo = LittleFS.open(ARQUIVO_LOG, "r");
  if (arquivo) {
    while (arquivo.available()) {
      String linha = arquivo.readStringUntil('\n');
      linha.trim();
      if (linha.length() == 0) continue;

      unsigned long id = pegarCampo(linha, 0).toInt();
      if (contarCampos(linha) != TOTAL_CAMPOS) {
        r.motivo = "registro mal formado";
      } else if (id != idEsperado) {
        r.motivo = "registro faltando ou fora de ordem";   // alguem apagou uma linha
      } else if (pegarCampo(linha, 8) != hashEsperado) {
        r.motivo = "corrente quebrada (hash anterior)";
      } else {
        String corpo = linha.substring(0, linha.lastIndexOf(';'));
        if (calcularHash(corpo) != pegarCampo(linha, 9)) {
          r.motivo = "conteudo alterado (HMAC nao confere)";
        }
      }

      if (r.motivo != "ok") {
        r.integra = false;
        r.idComErro = (id == 0) ? idEsperado : id;
        break;
      }
      hashEsperado = pegarCampo(linha, 9);
      idEsperado++;
      r.totalVerificados++;
    }
    arquivo.close();
  }

  // Confere se o fim do arquivo bate com o que esta na memoria
  // (detecta linhas apagadas no FINAL durante esta sessao)
  if (r.integra && idEsperado - 1 != ultimoId) {
    r.integra = false;
    r.idComErro = idEsperado;
    r.motivo = "registros finais apagados";
  }
  return r;
}

int auditLerUltimos(RegistroAuditoria registros[], int maximo) {
  if (!pronto || totalRegistros == 0) return 0;
  int pular = totalRegistros - maximo;
  if (pular < 0) pular = 0;

  int linhaAtual = 0;
  int copiados = 0;
  File arquivo = LittleFS.open(ARQUIVO_LOG, "r");
  if (!arquivo) return 0;
  while (arquivo.available() && copiados < maximo) {
    String linha = arquivo.readStringUntil('\n');
    linha.trim();
    if (linha.length() == 0) continue;
    if (linhaAtual >= pular) {
      registros[copiados].id = pegarCampo(linha, 0).toInt();
      registros[copiados].boot = pegarCampo(linha, 1).toInt();
      registros[copiados].uptime = pegarCampo(linha, 2).toInt();
      registros[copiados].categoria = pegarCampo(linha, 3);
      registros[copiados].tipo = pegarCampo(linha, 4);
      registros[copiados].severidade = pegarCampo(linha, 5);
      registros[copiados].usuario = pegarCampo(linha, 6);
      registros[copiados].detalhe = pegarCampo(linha, 7);
      registros[copiados].hashAnterior = pegarCampo(linha, 8);
      registros[copiados].hashAtual = pegarCampo(linha, 9);
      copiados++;
    }
    linhaAtual++;
  }
  arquivo.close();
  return copiados;
}

unsigned long auditSimularAdulteracao() {
  if (!pronto || totalRegistros < 3) return 0;
  int alvo = totalRegistros / 2;          // um registro do meio
  unsigned long idAlterado = 0;

  File origem = LittleFS.open(ARQUIVO_LOG, "r");
  File destino = LittleFS.open(ARQUIVO_TEMP, "w");
  if (!origem || !destino) return 0;

  int linhaAtual = 0;
  while (origem.available()) {
    String linha = origem.readStringUntil('\n');
    linha.trim();
    if (linha.length() == 0) continue;
    if (linhaAtual == alvo) {
      // O "invasor" troca o tipo do evento (ex.: um acesso negado vira
      // liberado) mas NAO consegue recalcular o HMAC, pois nao tem a chave.
      String tipoFalso = (pegarCampo(linha, 4) == "ACESSO_LIBERADO") ? "ACESSO_NEGADO" : "ACESSO_LIBERADO";
      String novaLinha = "";
      for (int c = 0; c < TOTAL_CAMPOS; c++) {
        if (c > 0) novaLinha += ";";
        novaLinha += (c == 4) ? tipoFalso : pegarCampo(linha, c);
      }
      linha = novaLinha;
      idAlterado = pegarCampo(linha, 0).toInt();
    }
    destino.print(linha + "\n");
    linhaAtual++;
  }
  origem.close();
  destino.close();
  LittleFS.remove(ARQUIVO_LOG);
  LittleFS.rename(ARQUIVO_TEMP, ARQUIVO_LOG);
  return idAlterado;
}

void auditNovoLog() {
  if (!pronto) return;
  LittleFS.remove(ARQUIVO_LOG);
  epoca++;                    // fica registrado quantas vezes o historico foi descartado
  ancoraId = ultimoId;        // os ids continuam crescendo, nunca repetem
  ancoraHash = hashZero();    // corrente nova
  salvarAncora();
  ultimoHash = ancoraHash;
  totalRegistros = 0;
}

unsigned long auditUltimoId() { return ultimoId; }
String auditUltimoHash() { return ultimoHash; }
int auditTotalRegistros() { return totalRegistros; }
unsigned long auditNumeroBoot() { return numeroBoot; }
unsigned long auditEpoca() { return epoca; }
unsigned long auditAncoraId() { return ancoraId; }

// =============================================================
// SENTINEL - event_logger (ETAPA 5: auditoria criptografica)
// Responsabilidade: guardar os eventos de seguranca de um jeito que
// qualquer ALTERACAO posterior possa ser detectada.
//
// Cada registro e uma linha do arquivo /audit.log (LittleFS):
//   id;boot;uptime;categoria;tipo;severidade;usuario;detalhe;hashAnterior;hashAtual
//
//   hashAtual = HMAC-SHA256(chave, "id;boot;...;detalhe;hashAnterior")
//
// Como cada hash depende do anterior, os registros formam uma CORRENTE.
// Mudar um registro antigo quebra o hash dele e a ligacao com o proximo.
// A chave (32 bytes aleatorios) fica na NVS: sem ela, nao da para
// recalcular a corrente depois de mexer no arquivo.
//
// LIMITACAO (documentada): a corrente detecta alteracao e remocao no
// meio, mas NAO impede apagar o arquivo inteiro ou restaurar uma copia
// antiga. Para isso servem os "checkpoints" enviados para fora (futuro).
// =============================================================
#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <Arduino.h>

struct RegistroAuditoria {
  unsigned long id;
  unsigned long boot;
  unsigned long uptime;
  String categoria;
  String tipo;
  String severidade;
  String usuario;
  String detalhe;
  String hashAnterior;
  String hashAtual;
};

struct ResultadoVerificacao {
  bool integra;               // true = nenhuma alteracao encontrada
  int totalVerificados;
  unsigned long idComErro;    // primeiro registro com problema (0 = nenhum)
  String motivo;
};

bool auditIniciar();
bool auditPronto();

// Grava um evento. Devolve o id do registro (0 se nao conseguiu gravar).
unsigned long auditRegistrar(String categoria, String tipo, String severidade,
                             String usuario, String detalhe);

ResultadoVerificacao auditVerificar();

// Copia os ultimos registros para o vetor. Devolve quantos copiou.
int auditLerUltimos(RegistroAuditoria registros[], int maximo);

// LABORATORIO: altera um registro do meio SEM recalcular o hash, como um
// invasor faria. Devolve o id alterado (0 = nao havia registros suficientes).
unsigned long auditSimularAdulteracao();

// Comeca um log novo (exige senha no menu). O historico anterior e descartado.
void auditNovoLog();

unsigned long auditUltimoId();
String auditUltimoHash();
int auditTotalRegistros();
unsigned long auditNumeroBoot();
unsigned long auditEpoca();      // quantas vezes o log foi reiniciado com "Novo log"
unsigned long auditAncoraId();

#endif

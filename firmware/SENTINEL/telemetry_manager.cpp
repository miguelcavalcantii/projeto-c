#include "config.h"
#include "security_config.h"
#include "telemetry_manager.h"
#include "display_manager.h"
#include "input_manager.h"
#include "authentication.h"
#include "security_engine.h"
#include "event_logger.h"
#include "user_manager.h"
#include "rfid_manager.h"

#define HEARTBEAT_MS         1000   // manda o "batimento" a cada 1 s
#define PAINEL_TIMEOUT_MS    5000   // sem PING por 5 s = painel desconectado
#define COMANDO_MAX          32     // comandos maiores que isso sao descartados
#define INTERVALO_PESADO_MS  2000   // AUDIT? e REC? no maximo 1 vez a cada 2 s

static unsigned long ultimoHeartbeat = 0;
static unsigned long ultimoPing = 0;
static unsigned long ultimoComandoPesado = 0;
static bool painelEstavaConectado = false;
static String estadoAtual = "INICIO";
static String comando = "";
static bool comandoGrandeDemais = false;

// Prepara um texto para ir dentro de aspas no JSON.
// Os icones do LCD (bytes 1 a 7) viram \u0001...\u0007, o bloco cheio
// vira um quadrado e a seta do LCD vira uma seta Unicode.
static String textoJson(String s) {
  String r = "\"";
  for (unsigned int i = 0; i < s.length(); i++) {
    byte c = (byte)s[i];
    if (c == '"') r += "\\\"";
    else if (c == '\\') r += "\\\\";
    else if (c == CHAR_BLOCO) r += "\\u2588";
    else if (c == CHAR_SETA) r += "\\u2192";
    else if (c < 32) {
      r += "\\u00";
      if (c < 16) r += "0";
      r += String(c, HEX);
    }
    else if (c > 126) r += "?";
    else r += (char)c;
  }
  r += "\"";
  return r;
}

void telemetriaBoot(bool lcdEncontrado) {
  String msg = "{\"t\":\"boot\",\"v\":" + textoJson(FIRMWARE_VERSION);
  msg += ",\"chip\":" + textoJson(ESP.getChipModel());
  msg += ",\"lcd\":" + String(lcdEncontrado ? "true" : "false");
  msg += ",\"boot\":" + String(auditNumeroBoot()) + "}";
  Serial.println(msg);
}

void telemetriaEstado(String nomeEstado) {
  estadoAtual = nomeEstado;
  Serial.println("{\"t\":\"state\",\"s\":" + textoJson(nomeEstado) + "}");
}

void telemetriaEvento(String categoria, String tipo, String severidade, String mensagem,
                      String usuario, unsigned long idAuditoria, String hashCurto) {
  String msg = "{\"t\":\"evt\",\"id\":" + String(idAuditoria);
  msg += ",\"up\":" + String(millis() / 1000);
  msg += ",\"cat\":" + textoJson(categoria);
  msg += ",\"type\":" + textoJson(tipo);
  msg += ",\"sev\":" + textoJson(severidade);
  msg += ",\"msg\":" + textoJson(mensagem);
  msg += ",\"usr\":" + textoJson(usuario);
  msg += ",\"h\":" + textoJson(hashCurto) + "}";
  Serial.println(msg);
}

static void enviarHeartbeat() {
  String msg = "{\"t\":\"hb\",\"up\":" + String(millis() / 1000);
  msg += ",\"heap\":" + String(ESP.getFreeHeap());
  msg += ",\"state\":" + textoJson(estadoAtual);
  msg += ",\"jx\":" + String(inputJoyX());
  msg += ",\"jy\":" + String(inputJoyY());
  msg += ",\"btn\":" + String(inputBotaoApertado() ? 1 : 0);
  msg += ",\"lcd\":" + String(displayOk() ? 1 : 0);
  msg += ",\"pc\":" + String(telemetriaPainelConectado() ? 1 : 0);
  msg += ",\"pin\":" + String(authSenhaCadastrada(0) ? 1 : 0);
  msg += ",\"kdf\":" + String(authUltimoTempoMs());
  msg += ",\"rfid\":" + String(rfidPresente() ? 1 : 0);
  msg += ",\"rfv\":" + String(rfidVersao());
  msg += ",\"fails\":" + String(secFalhas());
  msg += ",\"left\":" + String(secTentativasRestantes());
  msg += ",\"lock\":" + String(secSegundosRestantes());
  msg += ",\"lvl\":" + String(secNivel());
  msg += ",\"locks\":" + String(secTotalBloqueios());
  msg += ",\"recs\":" + String(auditTotalRegistros());
  msg += ",\"last\":" + String(auditUltimoId());
  msg += ",\"boot\":" + String(auditNumeroBoot());
  msg += ",\"epoch\":" + String(auditEpoca()) + "}";
  Serial.println(msg);
}

static void enviarCopiaLcd() {
  String msg = "{\"t\":\"lcd\",\"l1\":" + textoJson(displayCopiaLinha(0));
  msg += ",\"l2\":" + textoJson(displayCopiaLinha(1)) + "}";
  Serial.println(msg);
}

static void enviarVerificacao() {
  ResultadoVerificacao r = auditVerificar();
  String msg = "{\"t\":\"audit\",\"ok\":" + String(r.integra ? "true" : "false");
  msg += ",\"total\":" + String(r.totalVerificados);
  msg += ",\"bad\":" + String(r.idComErro);
  msg += ",\"why\":" + textoJson(r.motivo);
  msg += ",\"anchor\":" + String(auditAncoraId());
  msg += ",\"epoch\":" + String(auditEpoca()) + "}";
  Serial.println(msg);
}

static void enviarRegistros() {
  static RegistroAuditoria registros[AUDIT_REGISTROS_PAINEL];   // static: fora da pilha (stack) da tarefa
  int n = auditLerUltimos(registros, AUDIT_REGISTROS_PAINEL);
  Serial.println("{\"t\":\"recbegin\",\"n\":" + String(n) + "}");
  for (int i = 0; i < n; i++) {
    String msg = "{\"t\":\"rec\",\"id\":" + String(registros[i].id);
    msg += ",\"boot\":" + String(registros[i].boot);
    msg += ",\"up\":" + String(registros[i].uptime);
    msg += ",\"cat\":" + textoJson(registros[i].categoria);
    msg += ",\"type\":" + textoJson(registros[i].tipo);
    msg += ",\"sev\":" + textoJson(registros[i].severidade);
    msg += ",\"usr\":" + textoJson(registros[i].usuario);
    msg += ",\"prev\":" + textoJson(registros[i].hashAnterior);
    msg += ",\"hash\":" + textoJson(registros[i].hashAtual) + "}";
    Serial.println(msg);
  }
}

static void enviarUsuarios() {
  for (int i = 0; i < MAX_USUARIOS; i++) {
    String msg = "{\"t\":\"user\",\"i\":" + String(i);
    msg += ",\"name\":" + textoJson(usuarioNome(i));
    msg += ",\"admin\":" + String(i == 0 ? "true" : "false");
    msg += ",\"active\":" + String(usuarioAtivo(i) ? "true" : "false");
    msg += ",\"pin\":" + String(authSenhaCadastrada(i) ? "true" : "false");
    // So os 4 primeiros caracteres do id protegido: suficiente para diferenciar
    msg += ",\"card\":" + textoJson(usuarioTemCartao(i) ? usuarioCartao(i).substring(0, 4) : "");
    msg += ",\"n\":" + String(usuarioAcessos(i)) + "}";
    Serial.println(msg);
  }
}

// Comandos "pesados" (leem o arquivo inteiro) tem limite de frequencia:
// pedir demais vira um evento de "repeticao excessiva" (regra do IDS).
static bool podeExecutarPesado() {
  if (ultimoComandoPesado != 0 && millis() - ultimoComandoPesado < INTERVALO_PESADO_MS) {
    telemetriaEvento("serial", "CMD_EXCESSIVO", SEV_AVISO, "Comandos pesados repetidos rapido demais", "-", 0, "");
    return false;
  }
  ultimoComandoPesado = millis();
  return true;
}

// Executa um comando recebido do PC. So aceita comandos conhecidos:
// qualquer outra coisa e registrada como evento suspeito (validacao de entrada).
static void executarComando(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "PING") {
    ultimoPing = millis();
    if (ultimoPing == 0) ultimoPing = 1;
    Serial.println("{\"t\":\"pong\"}");
  } else if (cmd == "LCD?") {
    enviarCopiaLcd();
  } else if (cmd == "AUDIT?") {
    if (podeExecutarPesado()) enviarVerificacao();
  } else if (cmd == "REC?") {
    if (podeExecutarPesado()) enviarRegistros();
  } else if (cmd == "USERS?") {
    enviarUsuarios();
  } else {
    telemetriaEvento("serial", "CMD_DESCONHECIDO", SEV_AVISO, "Comando recusado pela validacao", "-", 0, "");
  }
}

static void lerComandos() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (comandoGrandeDemais) {
        telemetriaEvento("serial", "CMD_GRANDE_DEMAIS", SEV_AVISO, "Comando descartado (tamanho)", "-", 0, "");
      } else {
        executarComando(comando);
      }
      comando = "";
      comandoGrandeDemais = false;
    } else if (comando.length() < COMANDO_MAX) {
      comando += c;
    } else {
      comandoGrandeDemais = true;   // nao guarda mais nada: evita estourar a memoria
    }
  }
}

bool telemetriaPainelConectado() {
  return ultimoPing != 0 && millis() - ultimoPing < PAINEL_TIMEOUT_MS;
}

void telemetriaAtualizar() {
  lerComandos();

  // Detecta quando o painel conecta ou desconecta
  bool conectado = telemetriaPainelConectado();
  if (conectado && !painelEstavaConectado) {
    telemetriaEvento("painel", "PAINEL_CONECTADO", SEV_INFO, "Painel do PC conectado via USB", "-", 0, "");
    enviarCopiaLcd();
  } else if (!conectado && painelEstavaConectado) {
    telemetriaEvento("painel", "PAINEL_DESCONECTADO", SEV_AVISO, "Painel do PC parou de responder", "-", 0, "");
  }
  painelEstavaConectado = conectado;

  if (displayMudou()) enviarCopiaLcd();

  if (millis() - ultimoHeartbeat >= HEARTBEAT_MS) {
    ultimoHeartbeat = millis();
    enviarHeartbeat();
  }
}

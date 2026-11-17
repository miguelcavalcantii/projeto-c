// =============================================================
// SENTINEL - telemetry_manager
// Responsabilidade: conversar com o PAINEL DO COMPUTADOR pelo cabo USB.
//
// Protocolo (1 mensagem por linha, formato JSON):
//   ESP -> PC  {"t":"boot",...}   placa ligou
//              {"t":"state",...}  mudou de tela/estado
//              {"t":"hb",...}     "batimento" a cada 1 s (uptime, memoria,
//                                 joystick, senha, bloqueio, auditoria)
//              {"t":"lcd",...}    copia do que esta no LCD
//              {"t":"evt",...}    evento de seguranca (com id e hash da auditoria)
//              {"t":"audit",...}  resultado da verificacao da corrente
//              {"t":"rec",...}    um registro da auditoria (lista para o painel)
//              {"t":"user",...}   um usuario (sem senha e sem UID, so o resumo)
//              {"t":"pong"}       resposta ao PING
//   PC -> ESP  PING               "estou conectado" (a cada 2 s)
//              LCD?               pede a copia do LCD de novo
//              AUDIT?             verifica a corrente de auditoria
//              REC?               pede os ultimos registros da auditoria
//              USERS?             pede a lista de usuarios
//
// O canal e SO DE LEITURA: nenhum comando muda configuracao ou dados.
// Linhas que comecam com "#" sao so mensagens de depuracao.
// Documentacao: docs/05_painel_pc.md e docs/07_etapas4e5.md
// =============================================================
#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <Arduino.h>

// Severidades usadas nos eventos
#define SEV_INFO    "info"
#define SEV_AVISO   "aviso"
#define SEV_ALERTA  "alerta"
#define SEV_CRITICO "critico"

void telemetriaBoot(bool lcdEncontrado);
void telemetriaEstado(String nomeEstado);
// idAuditoria = 0 quando o evento nao foi para a auditoria
void telemetriaEvento(String categoria, String tipo, String severidade, String mensagem,
                      String usuario, unsigned long idAuditoria, String hashCurto);
void telemetriaAtualizar();        // chamar em TODA volta do loop()
bool telemetriaPainelConectado();

#endif

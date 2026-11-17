// =============================================================
// SENTINEL - security_engine (ETAPA 4: IDS e bloqueio)
// Responsabilidade: decidir quando uma sequencia de erros e suspeita
// e bloquear a autenticacao por um tempo.
//
// Regras (limites em security_config.h, podem ser mudados):
//   1a e 2a senha errada -> so registra
//   3a e 4a              -> ALERTA (possivel forca bruta)
//   5a                   -> BLOQUEIO temporario
//
// Tempo progressivo: 30 s, 60 s, 120 s, 240 s... ate 15 min (nunca
// bloqueia para sempre). Acertar a senha volta tudo ao normal.
//
// Todo o estado fica na NVS: reiniciar a placa NAO libera o bloqueio.
// =============================================================
#ifndef SECURITY_ENGINE_H
#define SECURITY_ENGINE_H

#include <Arduino.h>

enum ResultadoFalha {
  FALHA_REGISTRADA,   // erro comum
  FALHA_ALERTA,       // erros suficientes para desconfiar
  FALHA_BLOQUEIO      // bloqueio ativado agora
};

void secIniciar();
void secAtualizar();                      // chamar em TODA volta do loop() (conta o tempo)

ResultadoFalha secRegistrarFalha();
void secRegistrarSucesso();
unsigned long secTentativaDuranteBloqueio();  // devolve quantas tentativas ja houve

bool secBloqueado();
bool secAcabouDeDesbloquear();            // true uma unica vez, quando o tempo acaba
bool secBloqueioRetomadoNoBoot();         // a placa ligou ja bloqueada (reinicio suspeito)

unsigned long secSegundosRestantes();
unsigned long secUltimaDuracao();
int secFalhas();
int secTentativasRestantes();
int secNivel();
unsigned long secTotalBloqueios();

// Para o painel administrativo (Etapa 6). Valores fora da faixa sao recusados.
bool secDefinirLimites(int alerta, int bloqueio, unsigned long baseSeg, unsigned long maxSeg);

#endif

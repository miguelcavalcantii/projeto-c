#include <Preferences.h>
#include "security_config.h"
#include "security_engine.h"

#define GRAVAR_A_CADA_SEG 10   // durante o bloqueio, salva o tempo restante a cada 10 s

static Preferences nvs;

// Estado (copia na RAM do que esta salvo na NVS)
static int falhas = 0;                  // erros seguidos
static int nivel = 0;                   // quantos bloqueios seguidos (define o tempo)
static unsigned long restante = 0;      // segundos de bloqueio que faltam
static unsigned long totalBloqueios = 0;
static unsigned long ultimaDuracao = 0;

// Limites (comecam com os padroes de security_config.h)
static int limiteAlerta = IDS_ALERT_AFTER;
static int limiteBloqueio = IDS_LOCK_AFTER;
static unsigned long tempoBase = LOCK_BASE_SECONDS;
static unsigned long tempoMax = LOCK_MAX_SECONDS;

// Controle do relogio do bloqueio
static unsigned long ultimoSegundo = 0;
static int segundosSemGravar = 0;
static bool desbloqueouAgora = false;
static bool retomadoNoBoot = false;
static unsigned long tentativasNoBloqueio = 0;

static void salvarEstado() {
  nvs.begin("sentinel_ids", false);
  nvs.putInt("falhas", falhas);
  nvs.putInt("nivel", nivel);
  nvs.putULong("restante", restante);
  nvs.putULong("total", totalBloqueios);
  nvs.end();
}

void secIniciar() {
  nvs.begin("sentinel_ids", true);
  falhas = nvs.getInt("falhas", 0);
  nivel = nvs.getInt("nivel", 0);
  restante = nvs.getULong("restante", 0);
  totalBloqueios = nvs.getULong("total", 0);
  limiteAlerta = nvs.getInt("lim_alerta", IDS_ALERT_AFTER);
  limiteBloqueio = nvs.getInt("lim_bloq", IDS_LOCK_AFTER);
  tempoBase = nvs.getULong("t_base", LOCK_BASE_SECONDS);
  tempoMax = nvs.getULong("t_max", LOCK_MAX_SECONDS);
  nvs.end();

  // Se a placa ligou com tempo de bloqueio pendente, alguem pode ter
  // tentado "furar" o bloqueio reiniciando. O bloqueio continua.
  retomadoNoBoot = (restante > 0);
  ultimoSegundo = millis();
}

// 30 s no 1o bloqueio, 60 s no 2o, 120 s no 3o... limitado ao maximo
static unsigned long calcularDuracao() {
  unsigned long duracao = tempoBase;
  for (int i = 0; i < nivel; i++) {
    duracao = duracao * LOCK_MULTIPLIER;
    if (duracao >= tempoMax) return tempoMax;
  }
  return duracao;
}

ResultadoFalha secRegistrarFalha() {
  falhas++;
  ResultadoFalha resultado = FALHA_REGISTRADA;

  if (falhas >= limiteBloqueio) {
    ultimaDuracao = calcularDuracao();
    restante = ultimaDuracao;
    nivel++;                      // o proximo bloqueio sera mais longo
    totalBloqueios++;
    falhas = 0;                   // quando o bloqueio acabar, comeca a contar de novo
    tentativasNoBloqueio = 0;
    ultimoSegundo = millis();
    segundosSemGravar = 0;
    resultado = FALHA_BLOQUEIO;
  } else if (falhas >= limiteAlerta) {
    resultado = FALHA_ALERTA;
  }

  salvarEstado();                 // grava NA HORA: desligar logo depois nao apaga o erro
  return resultado;
}

void secRegistrarSucesso() {
  falhas = 0;
  nivel = 0;                      // recuperacao: o proximo bloqueio volta a ser de 30 s
  salvarEstado();
}

unsigned long secTentativaDuranteBloqueio() {
  tentativasNoBloqueio++;
  return tentativasNoBloqueio;
}

void secAtualizar() {
  if (restante == 0) return;
  if (millis() - ultimoSegundo >= 1000) {
    ultimoSegundo += 1000;
    restante--;
    segundosSemGravar++;
    if (restante == 0) {
      desbloqueouAgora = true;
      salvarEstado();
    } else if (segundosSemGravar >= GRAVAR_A_CADA_SEG) {
      // Grava de tempos em tempos (nao a cada segundo, para nao gastar a flash).
      // Se a placa reiniciar, perde no maximo 10 s de "progresso": o
      // bloqueio fica um pouco MAIS longo, nunca mais curto.
      salvarEstado();
      segundosSemGravar = 0;
    }
  }
}

bool secBloqueado() { return restante > 0; }

bool secAcabouDeDesbloquear() {
  bool d = desbloqueouAgora;
  desbloqueouAgora = false;
  return d;
}

bool secBloqueioRetomadoNoBoot() { return retomadoNoBoot; }
unsigned long secSegundosRestantes() { return restante; }
unsigned long secUltimaDuracao() { return ultimaDuracao; }
int secFalhas() { return falhas; }
int secTentativasRestantes() { return limiteBloqueio - falhas; }
int secNivel() { return nivel; }
unsigned long secTotalBloqueios() { return totalBloqueios; }

bool secDefinirLimites(int alerta, int bloqueio, unsigned long baseSeg, unsigned long maxSeg) {
  // Validacao: evita configuracoes perigosas (ex.: bloqueio eterno ou sem bloqueio)
  if (alerta < 1 || bloqueio < 2 || alerta >= bloqueio || bloqueio > 20) return false;
  if (baseSeg < 5 || maxSeg < baseSeg || maxSeg > 3600) return false;

  limiteAlerta = alerta;
  limiteBloqueio = bloqueio;
  tempoBase = baseSeg;
  tempoMax = maxSeg;
  nvs.begin("sentinel_ids", false);
  nvs.putInt("lim_alerta", alerta);
  nvs.putInt("lim_bloq", bloqueio);
  nvs.putULong("t_base", baseSeg);
  nvs.putULong("t_max", maxSeg);
  nvs.end();
  return true;
}

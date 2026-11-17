#include "security_config.h"
#include "historico.h"

static ItemHistorico fila[HISTORICO_TAMANHO];
static int proximo = 0;      // onde o proximo item sera escrito
static int quantidade = 0;   // quantos itens validos existem (ate HISTORICO_TAMANHO)

void historicoAdicionar(String tipo, String usuario) {
  fila[proximo].tipo = tipo;
  fila[proximo].usuario = usuario;
  fila[proximo].uptime = millis() / 1000;

  proximo = (proximo + 1) % HISTORICO_TAMANHO;   // da a volta: 9 -> 0
  if (quantidade < HISTORICO_TAMANHO) quantidade++;
}

int historicoQuantidade() {
  return quantidade;
}

ItemHistorico historicoPegar(int posicao) {
  // O mais recente esta UMA casa antes de "proximo".
  // Somar HISTORICO_TAMANHO evita numero negativo antes do %.
  int indice = (proximo - 1 - posicao + HISTORICO_TAMANHO * 2) % HISTORICO_TAMANHO;
  return fila[indice];
}

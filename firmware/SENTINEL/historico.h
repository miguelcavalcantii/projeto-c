// =============================================================
// SENTINEL - historico
// Responsabilidade: guardar os ULTIMOS eventos para a tela "Historico".
//
// ESTRUTURA DE DADOS: FILA CIRCULAR (buffer circular) de tamanho fixo.
//
//   posicoes:  [0] [1] [2] [3] [4] [5] [6] [7] [8] [9]
//                        ^ proximo a ser escrito
//
// - Inserir: escreve em "proximo" e anda 1 casa: proximo = (proximo + 1) % TAMANHO
// - Quando enche, o mais ANTIGO e sobrescrito (nao precisa mover nada).
// - Inserir e ler sao O(1): nao importa quantos eventos ja aconteceram.
// - Usa sempre a mesma memoria (nao cresce), ideal para microcontrolador.
// =============================================================
#ifndef HISTORICO_H
#define HISTORICO_H

#include <Arduino.h>

struct ItemHistorico {
  String tipo;
  String usuario;
  unsigned long uptime;    // segundos desde que a placa ligou
};

void historicoAdicionar(String tipo, String usuario);
int historicoQuantidade();
// posicao 0 = o mais recente, 1 = o anterior...
ItemHistorico historicoPegar(int posicao);

#endif

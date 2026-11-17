// =============================================================
// SENTINEL - user_manager
// Responsabilidade: a lista de usuarios do sistema.
//
// ESTRUTURA DE DADOS: um VETOR de structs (tamanho fixo MAX_USUARIOS).
// Cada posicao do vetor e um usuario:
//
//   indice | nome     | ativo | cartao (id protegido) | acessos
//   -------+----------+-------+-----------------------+--------
//     0    | Admin    | sim   | ""                    |   12
//     1    | Miguel   | sim   | "3fa1c09e..."         |    5
//
// Buscar o dono de um cartao = BUSCA LINEAR no vetor (O(n)).
// A senha de cada usuario fica no modulo authentication (mesmo indice).
// =============================================================
#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <Arduino.h>

struct Usuario {
  String nome;
  bool ativo;            // false = credencial revogada
  String cartao;         // id protegido do cracha ("" = sem cartao)
  unsigned long acessos; // quantos acessos liberados
};

void usuariosIniciar();

String usuarioNome(int indice);
bool usuarioAtivo(int indice);
bool usuarioTemCartao(int indice);
String usuarioCartao(int indice);
unsigned long usuarioAcessos(int indice);

// Busca linear: devolve o indice do dono do cartao, ou -1 se ninguem tem
int usuarioBuscarPorCartao(String idCartao);

bool usuarioAssociarCartao(int indice, String idCartao);  // false se o cartao ja e de outro
void usuarioRemoverCartao(int indice);
void usuarioDefinirAtivo(int indice, bool ativo);
void usuarioContarAcesso(int indice);

#endif

// =============================================================
// SENTINEL - authentication
// Responsabilidade: cadastrar e verificar a senha (PIN) de cada usuario.
//
// A senha NUNCA e guardada. Para cada usuario guardamos apenas:
//   - salt: 16 bytes aleatorios, diferentes para cada cadastro
//   - hash: PBKDF2-HMAC-SHA256(senha, salt) com N iteracoes
//   - iteracoes: o N usado (permite aumentar no futuro sem perder a senha)
// Tudo fica na NVS (memoria flash que sobrevive ao desligar).
//
// "usuario" e a posicao no vetor de usuarios (0 = Admin ... 3).
// =============================================================
#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <Arduino.h>

void authIniciar();
bool authSenhaCadastrada(int usuario);

// Recebem os digitos digitados (ex.: {1, 2, 3, 4})
bool authCadastrarSenha(int usuario, const int digitos[]);
bool authVerificarSenha(int usuario, const int digitos[]);

unsigned long authUltimoTempoMs();   // quanto o ultimo PBKDF2 demorou (para o relatorio)
unsigned long authIteracoes();

#endif

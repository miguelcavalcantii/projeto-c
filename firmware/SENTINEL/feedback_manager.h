// =============================================================
// SENTINEL - feedback_manager
// Responsabilidade: AVISAR o usuario do resultado (sucesso, erro,
// alerta, bloqueio). Substitui os LEDs:
//   antes: LED verde    -> agora: cadeado aberto + luz fixa
//   antes: LED vermelho -> agora: icone de X + luz piscando
// Se os LEDs (ou um buzzer) voltarem, so este arquivo muda.
// =============================================================
#ifndef FEEDBACK_MANAGER_H
#define FEEDBACK_MANAGER_H

#include <Arduino.h>

void feedbackSucesso(String detalhe);
void feedbackErro(String detalhe);
void feedbackAlerta(String detalhe);
void feedbackBloqueio(String detalhe);
void feedbackPiscar(int piscadas, unsigned long intervaloMs);  // pisca sem mudar o texto da tela
void feedbackAtualizar();   // chamar em TODA volta do loop() (faz a luz piscar)
bool feedbackPiscando();

#endif

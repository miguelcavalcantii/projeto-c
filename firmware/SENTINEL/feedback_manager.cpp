#include <Arduino.h>
#include "display_manager.h"
#include "feedback_manager.h"
#include "buzzer_manager.h"

// Controle do pisca-pisca da luz de fundo, sem usar delay()
static int trocasRestantes = 0;          // cada piscada = 2 trocas (apaga e acende)
static unsigned long intervaloMs = 0;
static unsigned long proximaTroca = 0;
static bool luzLigada = true;

static void comecarPiscar(int piscadas, unsigned long intervalo) {
  trocasRestantes = piscadas * 2;
  intervaloMs = intervalo;
  proximaTroca = millis() + intervalo;
  luzLigada = true;
  displayLuz(true);
}

void feedbackPiscar(int piscadas, unsigned long intervalo) {
  comecarPiscar(piscadas, intervalo);
}

// Antigo LED verde: icone de cadeado aberto + luz fixa
void feedbackSucesso(String detalhe) {
  trocasRestantes = 0;
  displayLuz(true);
  displayMostrarComIcone(ICONE_CADEADO_ABERTO, "ACESSO LIBERADO", "  " + detalhe);
  displayIcone(0, 1, ICONE_CERTO);
  buzzerTocar(SOM_SUCESSO);
}

// Antigo LED vermelho: icone de X + luz piscando rapido
void feedbackErro(String detalhe) {
  displayMostrarComIcone(ICONE_ERRO, "ACESSO NEGADO", "  " + detalhe);
  comecarPiscar(3, 150);
  buzzerTocar(SOM_ERRO);
}

void feedbackAlerta(String detalhe) {
  displayMostrarComIcone(ICONE_ALERTA, "ALERTA", "  " + detalhe);
  comecarPiscar(5, 300);
  buzzerTocar(SOM_ALERTA);
}

void feedbackBloqueio(String detalhe) {
  displayMostrarComIcone(ICONE_CADEADO, "SIST. BLOQUEADO", "  " + detalhe);
  displayIcone(0, 1, ICONE_RELOGIO);
  comecarPiscar(10, 500);
  buzzerTocar(SOM_SIRENE);
}

void feedbackAtualizar() {
  if (trocasRestantes <= 0) return;
  if (millis() >= proximaTroca) {
    luzLigada = !luzLigada;
    displayLuz(luzLigada);
    trocasRestantes--;
    proximaTroca = millis() + intervaloMs;
    if (trocasRestantes == 0) displayLuz(true); // sempre termina com a luz acesa
  }
}

bool feedbackPiscando() {
  return trocasRestantes > 0;
}

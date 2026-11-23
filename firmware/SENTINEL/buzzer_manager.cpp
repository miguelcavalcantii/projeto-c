#include <Arduino.h>
#include "config.h"
#include "buzzer_manager.h"

// Uma nota: frequencia em Hz (0 = silencio) e duracao em ms
struct Nota {
  int freq;
  int ms;
};

#define MAX_NOTAS 40

// Fila de notas do som que esta tocando agora
static Nota fila[MAX_NOTAS];
static int totalNotas = 0;
static int notaAtual = 0;
static unsigned long fimDaNota = 0;
static int prioridadeAtual = -1;   // -1 = nada tocando

// ---------- Funcoes internas ----------

static void ligarSom(int freq) {
  if (freq <= 0) {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  if (BUZZER_ATIVO) {
    digitalWrite(PIN_BUZZER, HIGH);   // buzzer ativo: so liga (ele tem o som pronto)
  } else {
    tone(PIN_BUZZER, freq);           // buzzer passivo: a ESP32 gera a frequencia
  }
}

static void adicionarNota(int freq, int ms) {
  if (totalNotas < MAX_NOTAS) {
    fila[totalNotas].freq = freq;
    fila[totalNotas].ms = ms;
    totalNotas++;
  }
}

// Quanto maior, mais importante (a sirene nunca e cortada por um bip)
static int prioridade(Som som) {
  if (som == SOM_SIRENE) return 3;
  if (som == SOM_ALERTA) return 2;
  if (som == SOM_LEMBRETE) return 0;
  return 1;
}

// Monta a lista de notas de cada som
static void montarSom(Som som) {
  totalNotas = 0;
  switch (som) {
    case SOM_INICIO:      // dois bips curtos subindo
      adicionarNota(1200, 80);
      adicionarNota(0, 50);
      adicionarNota(1800, 80);
      break;

    case SOM_SUCESSO:     // "bi-bip" agudo e alegre
      adicionarNota(1500, BUZZER_BIP_CURTO_MS);
      adicionarNota(0, 60);
      adicionarNota(2000, BUZZER_BIP_CURTO_MS + 50);
      break;

    case SOM_ERRO:        // um bip grave e longo
      adicionarNota(400, BUZZER_ERRO_MS);
      break;

    case SOM_ALERTA:      // 3 bips medios rapidos
      for (int i = 0; i < 3; i++) {
        adicionarNota(1000, BUZZER_BIP_CURTO_MS + 50);
        adicionarNota(0, 100);
      }
      break;

    case SOM_SIRENE:      // alarme: alterna grave e agudo
      for (int i = 0; i < BUZZER_SIRENE_CICLOS; i++) {
        adicionarNota(800, 250);
        adicionarNota(1300, 250);
      }
      break;

    case SOM_LEMBRETE:    // bip bem curto
      adicionarNota(1000, 60);
      break;
  }
}

// ---------- Funcoes publicas ----------

void buzzerIniciar() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void buzzerTocar(Som som) {
  if (!BUZZER_LIGADO) return;
  // Um som menos importante nao interrompe um mais importante
  if (prioridadeAtual >= 0 && prioridade(som) < prioridadeAtual) return;

  montarSom(som);
  prioridadeAtual = prioridade(som);
  notaAtual = 0;
  ligarSom(fila[0].freq);
  fimDaNota = millis() + fila[0].ms;
}

void buzzerAtualizar() {
  if (prioridadeAtual < 0) return;         // nada tocando
  if (millis() < fimDaNota) return;        // a nota atual ainda nao acabou

  notaAtual++;
  if (notaAtual >= totalNotas) {           // acabou o som
    buzzerParar();
    return;
  }
  ligarSom(fila[notaAtual].freq);
  fimDaNota = millis() + fila[notaAtual].ms;
}

void buzzerParar() {
  ligarSom(0);
  prioridadeAtual = -1;
  totalNotas = 0;
}

bool buzzerTocando() {
  return prioridadeAtual >= 0;
}

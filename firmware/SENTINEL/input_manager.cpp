#include <Arduino.h>
#include "config.h"
#include "input_manager.h"

// "static" aqui = variavel visivel so dentro deste arquivo.
// Assim nenhum outro modulo mexe nelas por engano.
static EventoEntrada eventoPendente = EV_NENHUM;

// --- Joystick ---
static EventoEntrada direcaoAnterior = EV_NENHUM;
static unsigned long proximaRepeticao = 0;

// --- Botao ---
static int leituraAnterior = HIGH;
static int estadoEstavel = HIGH;
static unsigned long inicioMudanca = 0;
static unsigned long inicioAperto = 0;
static bool longoJaEnviado = false;

void inputIniciar() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  // GPIO 34/35 sao so entrada e nao tem pull-up: nao precisa de pinMode
}

// Le os dois eixos e diz para que lado o joystick esta empurrado
static int ultimoX = 2048;
static int ultimoY = 2048;

static EventoEntrada lerDirecaoJoystick() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  ultimoX = x;
  ultimoY = y;
  if (JOY_INVERTER_X) x = 4095 - x;
  if (JOY_INVERTER_Y) y = 4095 - y;

  if (y < JOY_LIMITE_BAIXO) return EV_CIMA;
  if (y > JOY_LIMITE_ALTO)  return EV_BAIXO;
  if (x < JOY_LIMITE_BAIXO) return EV_ESQUERDA;
  if (x > JOY_LIMITE_ALTO)  return EV_DIREITA;
  return EV_NENHUM; // parado no centro (zona morta)
}

static void atualizarJoystick() {
  EventoEntrada direcao = lerDirecaoJoystick();
  unsigned long agora = millis();

  if (direcao == EV_NENHUM) {          // voltou ao centro
    direcaoAnterior = EV_NENHUM;
    return;
  }
  if (direcao != direcaoAnterior) {    // acabou de empurrar
    eventoPendente = direcao;
    direcaoAnterior = direcao;
    proximaRepeticao = agora + JOY_ATRASO_REPETICAO_MS;
  } else if (agora >= proximaRepeticao) { // continua segurando
    eventoPendente = direcao;
    proximaRepeticao = agora + JOY_INTERVALO_REPETICAO_MS;
  }
}

static void atualizarBotao() {
  int leitura = digitalRead(PIN_BUTTON);
  unsigned long agora = millis();

  // Debounce: so aceita a mudanca se ela ficou estavel por DEBOUNCE_MS
  if (leitura != leituraAnterior) {
    inicioMudanca = agora;
    leituraAnterior = leitura;
  }
  if (agora - inicioMudanca >= DEBOUNCE_MS && leitura != estadoEstavel) {
    estadoEstavel = leitura;
    if (estadoEstavel == LOW) {        // apertou
      inicioAperto = agora;
      longoJaEnviado = false;
    } else if (!longoJaEnviado) {      // soltou rapido = aperto curto
      eventoPendente = EV_OK;
    }
  }

  // Segurou tempo suficiente = aperto longo (envia so uma vez)
  if (estadoEstavel == LOW && !longoJaEnviado && agora - inicioAperto >= BOTAO_LONGO_MS) {
    eventoPendente = EV_OK_LONGO;
    longoJaEnviado = true;
  }
}

void inputAtualizar() {
  atualizarJoystick();
  atualizarBotao();   // depois do joystick: o botao tem prioridade
}

EventoEntrada inputLerEvento() {
  EventoEntrada e = eventoPendente;
  eventoPendente = EV_NENHUM;
  return e;
}

int inputJoyX() {
  return ultimoX;
}

int inputJoyY() {
  return ultimoY;
}

bool inputBotaoApertado() {
  return estadoEstavel == LOW;
}

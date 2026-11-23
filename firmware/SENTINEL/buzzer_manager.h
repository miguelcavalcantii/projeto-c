// =============================================================
// SENTINEL - buzzer_manager
// Responsabilidade: os SONS do sistema (bips e alarme).
//
// Cada som e uma lista de notas {frequencia, duracao}. O buzzer
// toca uma nota por vez usando millis(), SEM delay(): o resto do
// sistema (LCD, joystick, RFID) continua funcionando enquanto toca.
//
// Prioridade: um som mais importante interrompe um menos importante,
// mas um bip comum NAO corta a sirene do bloqueio.
// =============================================================
#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>

// Os sons que o sistema sabe tocar
enum Som {
  SOM_INICIO,       // ligou a placa (serve para testar a ligacao)
  SOM_SUCESSO,      // acesso liberado / senha salva
  SOM_ERRO,         // senha errada, cracha desconhecido
  SOM_ALERTA,       // 3a e 4a falha seguidas (atividade suspeita)
  SOM_SIRENE,       // BLOQUEIO, log violado, reinicio durante bloqueio
  SOM_LEMBRETE      // bip curto periodico enquanto esta bloqueado
};

void buzzerIniciar();
void buzzerTocar(Som som);
void buzzerAtualizar();   // chamar em TODA volta do loop()
void buzzerParar();
bool buzzerTocando();

#endif

// =============================================================
// SENTINEL - input_manager
// Responsabilidade: ler o joystick e o botao e transformar tudo em
// EVENTOS simples (cima, baixo, ok...). O resto do sistema nunca le
// pino diretamente: so pergunta "teve evento?".
// =============================================================
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

enum EventoEntrada {
  EV_NENHUM,
  EV_CIMA,
  EV_BAIXO,
  EV_ESQUERDA,
  EV_DIREITA,
  EV_OK,        // aperto curto do botao  -> confirmar
  EV_OK_LONGO   // botao segurado         -> cancelar / voltar
};

void inputIniciar();
void inputAtualizar();            // chamar em TODA volta do loop()
EventoEntrada inputLerEvento();   // devolve o ultimo evento e limpa

// Valores "crus" para o painel do PC mostrar o joystick ao vivo
int inputJoyX();                  // 0 a 4095
int inputJoyY();                  // 0 a 4095
bool inputBotaoApertado();

#endif

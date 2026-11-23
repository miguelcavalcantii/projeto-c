// =============================================================
// SENTINEL - Smart Embedded Network Threat and Intrusion Logging System
// Arquivo principal (Arduino IDE).
//
// VERSAO 0.6
//   - Senha com PBKDF2 + salt, uma por usuario
//   - IDS + bloqueio progressivo que sobrevive ao reinicio
//   - Auditoria com corrente de hashes HMAC-SHA256
//   - Usuarios (vetor de structs) + RFID (cartao + PIN = 2 fatores)
//   - Historico dos ultimos eventos (fila circular)
//
// ESTE PROGRAMA ESTA DIVIDIDO EM ABAS (a Arduino IDE junta todas):
//   SENTINEL.ino      -> variaveis, setup(), loop() e troca de estado
//   fluxo_cartao.ino  -> o que fazer quando um cracha e lido
//   fluxo_senha.ino   -> digitacao, verificacao e cadastro de senha
//   menus.ino         -> o que cada tela faz com o joystick/botao
//   telas.ino         -> desenho das telas no LCD
//
// Controles:
//   Joystick cima/baixo  -> navega / muda o digito / troca de pagina
//   Joystick esquerda    -> apaga o ultimo digito / volta
//   Botao (aperto curto) -> confirma
//   Botao (segurar)      -> cancela / volta
// =============================================================
#include "config.h"
#include "security_config.h"
#include "input_manager.h"
#include "display_manager.h"
#include "feedback_manager.h"
#include "buzzer_manager.h"
#include "telemetry_manager.h"
#include "authentication.h"
#include "security_engine.h"
#include "event_logger.h"
#include "user_manager.h"
#include "rfid_manager.h"
#include "historico.h"

// ---------- Tempos da interface ----------
#define BOOT_MS            1800    // duracao da animacao de inicio
#define RESULTADO_MS       3000    // tempo mostrando um resultado
#define RESULTADO_LONGO_MS 6000    // resultados importantes ficam mais tempo
#define ROTACAO_OCIOSO_MS  6000    // troca a mensagem da tela de espera
#define DESCANSO_MS        60000   // 1 min parado na espera -> apaga a luz
#define MENU_TIMEOUT_MS    30000   // 30 s sem mexer no menu -> volta para a espera
#define ROLAGEM_MS         350     // velocidade do texto correndo em "Sobre"

// =============================================================
// MAQUINA DE ESTADOS: o sistema esta sempre em UM destes estados
// =============================================================
enum Estado {
  ESTADO_INICIO,            // animacao de boot
  ESTADO_PRIMEIRO_USO,      // ainda nao existe senha do Admin
  ESTADO_OCIOSO,            // tela de espera
  ESTADO_MENU,
  ESTADO_ESCOLHER_USUARIO,  // "Entrar" sem cracha: escolhe o nome
  ESTADO_AGUARDAR_CARTAO,   // "Aproxime o cracha"
  ESTADO_SENHA,             // digitando senha
  ESTADO_RESULTADO,         // mostra um resultado por alguns segundos
  ESTADO_BLOQUEADO,         // forca bruta: contagem regressiva
  ESTADO_HISTORICO,         // ultimos eventos (fila circular)
  ESTADO_USUARIOS,          // lista de usuarios (so Admin)
  ESTADO_USUARIO_ACOES,     // o que fazer com o usuario escolhido
  ESTADO_AUDITORIA,
  ESTADO_STATUS,
  ESTADO_ALERTAS,
  ESTADO_SOBRE
};

// Nomes usados no painel do PC (mesma ordem do enum acima)
const char* NOMES_ESTADO[] = {"INICIO", "PRIMEIRO_USO", "OCIOSO", "MENU", "ESCOLHER_USUARIO",
                              "AGUARDAR_CARTAO", "SENHA", "RESULTADO", "BLOQUEADO", "HISTORICO",
                              "USUARIOS", "USUARIO_ACOES", "AUDITORIA", "STATUS", "ALERTAS", "SOBRE"};

// A tela de senha serve para 4 coisas diferentes
enum ModoSenha {
  MODO_ENTRAR,      // verificar para liberar o acesso
  MODO_NOVA,        // digitar uma senha nova
  MODO_CONFIRMAR,   // digitar a senha nova de novo
  MODO_AUTORIZAR    // senha do Admin antes de uma acao sensivel
};

// A tela "Aproxime o cracha" serve para 2 coisas
enum ModoCartao {
  CARTAO_SEGUNDO_FATOR,   // conferir se o cracha e do usuario escolhido
  CARTAO_CADASTRO         // associar um cracha novo a um usuario
};

// Acoes que exigem a senha do Admin
enum AcaoProtegida {
  ACAO_USUARIOS,
  ACAO_NOVO_LOG
};

// ---------- Estado atual ----------
Estado estado = ESTADO_INICIO;
unsigned long estadoDesde = 0;       // quando entrou no estado atual
unsigned long ultimaInteracao = 0;   // ultima vez que alguem mexeu em algo
unsigned long duracaoResultado = RESULTADO_MS;
bool voltarParaUsuarios = false;     // depois do resultado, volta para a tela do Admin
bool emDescanso = false;             // luz do LCD apagada para economizar

// ---------- Menus ----------
const String itensMenu[] = {"Entrar", "Historico", "Usuarios", "Auditoria", "Status", "Alertas", "Sobre", "Voltar"};  // max. 11 letras
const int TOTAL_ITENS = 8;
int itemSelecionado = 0;

const String itensAuditoria[] = {"Verificar", "Sim. ataque", "Novo log", "Voltar"};
const int TOTAL_ITENS_AUDITORIA = 4;
int itemAuditoria = 0;

String itensAcoes[] = {"Definir PIN", "Cad. cartao", "Tirar cart.", "Revogar", "Voltar"};
const int TOTAL_ITENS_ACOES = 5;
int itemAcao = 0;

// Lista montada na hora (so quem pode entrar, ou todos para o Admin)
int listaIndices[MAX_USUARIOS];      // posicao de cada item no vetor de usuarios
String listaNomes[MAX_USUARIOS];     // texto mostrado no LCD
int qtdLista = 0;
int itemLista = 0;

// ---------- Quem esta usando ----------
int usuarioAtual = 0;                // quem esta tentando entrar
int usuarioAlvo = 0;                 // quem o Admin esta editando
bool cartaoConfirmado = false;       // o 1o fator (cracha) ja foi validado?
ModoSenha modoSenha = MODO_ENTRAR;
ModoCartao modoCartao = CARTAO_SEGUNDO_FATOR;
AcaoProtegida acaoPendente = ACAO_USUARIOS;

// ---------- Senha ----------
int digitos[PIN_LENGTH];
int senhaNova[PIN_LENGTH];           // guarda a 1a digitacao para comparar com a 2a
int qtdDigitados = 0;
int digitoAtual = 0;

// ---------- Telas que mudam sozinhas ----------
int paginaOcioso = 0;
int paginaStatus = 0;
const int TOTAL_PAGINAS_STATUS = 8;
int posicaoHistorico = 0;
int alertaAtual = 0;
int posicaoRolagem = 0;
unsigned long ultimaAtualizacao = 0;
int ultimoProgresso = -10;

const String TEXTO_SOBRE = "Smart Embedded Network Threat and Intrusion Logging System - CESAR School";

// =============================================================
// Funcoes auxiliares
// =============================================================

// 5 -> "05"
String doisDigitos(unsigned long n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

// 3725 segundos -> "01:02:05"
String formatarTempo(unsigned long segundos) {
  unsigned long h = segundos / 3600;
  unsigned long m = (segundos % 3600) / 60;
  unsigned long s = segundos % 60;
  return doisDigitos(h) + ":" + doisDigitos(m) + ":" + doisDigitos(s);
}

// 75 segundos -> "01:15"
String formatarMinSeg(unsigned long segundos) {
  return doisDigitos(segundos / 60) + ":" + doisDigitos(segundos % 60);
}

// Todo evento importante vai para TRES lugares:
//   1) a auditoria (arquivo com corrente de hashes, fica na placa)
//   2) o painel do PC (aparece na hora, com o id e o inicio do hash)
//   3) o historico (fila circular que aparece no LCD)
void registrarEvento(String categoria, String tipo, String severidade, String mensagem, String usuario) {
  unsigned long id = auditRegistrar(categoria, tipo, severidade, usuario, mensagem);
  String hashCurto = (id > 0) ? auditUltimoHash().substring(0, 16) : "";
  telemetriaEvento(categoria, tipo, severidade, mensagem, usuario, id, hashCurto);
  historicoAdicionar(tipo, usuario);
}

// =============================================================
// Troca de estado (desenha a tela inicial de cada estado)
// =============================================================
void mudarEstado(Estado novo) {
  estado = novo;
  estadoDesde = millis();
  ultimaAtualizacao = millis();
  telemetriaEstado(NOMES_ESTADO[estado]);

  switch (estado) {
    case ESTADO_INICIO:
      ultimoProgresso = -10;
      displayMostrarComIcone(ICONE_ESCUDO, "SENTINEL " FIRMWARE_VERSION, "");
      break;
    case ESTADO_PRIMEIRO_USO:
      displayMostrarComIcone(ICONE_ALERTA, "Primeiro uso", "OK=senha Admin");
      break;
    case ESTADO_OCIOSO:
      paginaOcioso = 0;
      desenharOcioso();
      break;
    case ESTADO_MENU:
      displayMostrarMenu(itensMenu, TOTAL_ITENS, itemSelecionado);
      break;
    case ESTADO_ESCOLHER_USUARIO:
    case ESTADO_USUARIOS:
      itemLista = 0;
      desenharLista();
      break;
    case ESTADO_AGUARDAR_CARTAO:
      desenharAguardarCartao();
      break;
    case ESTADO_SENHA:
      limparSenha();
      digitoAtual = sortearDigito();
      displayMostrarSenha(tituloSenha(), qtdDigitados, digitoAtual);
      break;
    case ESTADO_RESULTADO:
    case ESTADO_BLOQUEADO:
      break; // a tela ja foi desenhada antes (pelo feedback)
    case ESTADO_HISTORICO:
      posicaoHistorico = 0;
      desenharHistorico();
      break;
    case ESTADO_USUARIO_ACOES:
      itemAcao = 0;
      desenharAcoes();
      break;
    case ESTADO_AUDITORIA:
      itemAuditoria = 0;
      displayMostrarMenu(itensAuditoria, TOTAL_ITENS_AUDITORIA, itemAuditoria);
      break;
    case ESTADO_STATUS:
      paginaStatus = 0;
      desenharStatus();
      break;
    case ESTADO_ALERTAS:
      alertaAtual = 0;
      displayMostrarComIcone(ICONE_ALERTA, "Testar alertas", "OK=prox <=volta");
      break;
    case ESTADO_SOBRE:
      posicaoRolagem = 0;
      displayMostrarComIcone(ICONE_ESCUDO, "SENTINEL " FIRMWARE_VERSION, "");
      desenharSobre();
      break;
  }
}

void mostrarResultado(unsigned long duracao) {
  duracaoResultado = duracao;
  mudarEstado(ESTADO_RESULTADO);
}

// Para onde ir quando uma tarefa termina
void irParaInicio() {
  if (secBloqueado()) mudarEstado(ESTADO_BLOQUEADO);
  else if (!authSenhaCadastrada(0)) mudarEstado(ESTADO_PRIMEIRO_USO);
  else if (voltarParaUsuarios) mudarEstado(ESTADO_USUARIO_ACOES);
  else mudarEstado(ESTADO_OCIOSO);
}

// Descanso de tela: apaga a luz se ninguem mexer na tela de espera.
// Devolve true se o evento foi usado so para "acordar" a tela.
bool controlarDescanso(EventoEntrada ev) {
  if (ev != EV_NENHUM) {
    ultimaInteracao = millis();
    if (emDescanso) {
      emDescanso = false;
      displayLuz(true);
      return true;   // esse toque so acordou a tela
    }
  }
  if (!emDescanso && estado == ESTADO_OCIOSO && millis() - ultimaInteracao >= DESCANSO_MS) {
    emDescanso = true;
    displayLuz(false);
  }
  return false;
}

// =============================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println();
  Serial.println("# === SENTINEL v" FIRMWARE_VERSION " ===");

  // Cada modulo e iniciado; se um falhar, o sistema avisa e continua
  inputIniciar();
  buzzerIniciar();
  authIniciar();
  secIniciar();
  usuariosIniciar();
  bool auditOk = auditIniciar();
  bool lcdOk = displayIniciar();
  bool rfidOk = rfidIniciar();
  Serial.println(lcdOk ? "# [OK] LCD" : "# [ERRO] LCD nao encontrado no I2C (teste 05)");
  Serial.println(auditOk ? "# [OK] Auditoria (LittleFS)" : "# [ERRO] LittleFS indisponivel");
  Serial.println(rfidOk ? "# [OK] RFID RC522" : "# [--] RFID nao encontrado (sistema segue sem cracha)");

  telemetriaBoot(lcdOk);
  registrarEvento("sistema", "BOOT", SEV_INFO, "SENTINEL v" FIRMWARE_VERSION " iniciado", "-");
  if (!lcdOk) registrarEvento("hardware", "LCD_AUSENTE", SEV_CRITICO, "LCD nao respondeu no I2C", "-");
  if (!auditOk) telemetriaEvento("hardware", "AUDITORIA_INDISPONIVEL", SEV_CRITICO, "LittleFS nao montou", "-", 0, "");
  if (rfidOk) {
    registrarEvento("rfid", "RFID_CONECTADO", SEV_INFO, "RC522 versao 0x" + String(rfidVersao(), HEX), "-");
  } else {
    registrarEvento("rfid", "RFID_AUSENTE", SEV_AVISO, "Leitor nao respondeu: modo so PIN", "-");
  }
  if (!authSenhaCadastrada(0)) {
    registrarEvento("auth", "SEM_SENHA", SEV_AVISO, "Senha do Admin nao cadastrada: primeiro uso", "-");
  }
  // Ligou ja bloqueada: alguem pode ter reiniciado para tentar furar o bloqueio
  if (secBloqueioRetomadoNoBoot()) {
    registrarEvento("ids", "REINICIO_DURANTE_BLOQUEIO", SEV_ALERTA,
                    "Bloqueio mantido apos reinicio: faltam " + String(secSegundosRestantes()) + " s", "-");
    buzzerTocar(SOM_SIRENE);
  } else {
    buzzerTocar(SOM_INICIO);
  }

  ultimaInteracao = millis();
  mudarEstado(ESTADO_INICIO);
}

void loop() {
  // 1) Atualiza os modulos (rapido, sem travar)
  inputAtualizar();
  feedbackAtualizar();
  buzzerAtualizar();
  secAtualizar();
  rfidAtualizar();
  telemetriaAtualizar();

  // 2) Avisos do leitor RFID (falha de comunicacao e uma regra do IDS)
  if (rfidPerdeuConexao()) {
    registrarEvento("hardware", "RFID_FALHA_COMUNICACAO", SEV_ALERTA, "Leitor RFID parou de responder", "-");
    buzzerTocar(SOM_ALERTA);   // leitor arrancado/desligado pode ser sabotagem
  }
  if (rfidReconectou()) {
    registrarEvento("rfid", "RFID_CONECTADO", SEV_INFO, "Leitor RFID detectado (0x" + String(rfidVersao(), HEX) + ")", "-");
  }

  // 3) Pega o evento do joystick/botao desta volta do loop
  EventoEntrada ev = inputLerEvento();
  if (controlarDescanso(ev)) ev = EV_NENHUM;

  // 4) Algum cracha foi aproximado?
  if (rfidCartaoDetectado()) {
    ultimaInteracao = millis();
    if (emDescanso) { emDescanso = false; displayLuz(true); }
    tratarCartaoLido(rfidUltimoId());
  }

  // 5) O tempo de bloqueio acabou?
  if (secAcabouDeDesbloquear()) {
    registrarEvento("ids", "DESBLOQUEIO", SEV_INFO, "Tempo de bloqueio encerrado", "-");
    if (estado == ESTADO_BLOQUEADO) irParaInicio();
  }

  // 6) Menus abandonados voltam sozinhos para a tela de espera
  bool emMenu = (estado == ESTADO_MENU || estado == ESTADO_ESCOLHER_USUARIO || estado == ESTADO_HISTORICO ||
                 estado == ESTADO_USUARIOS || estado == ESTADO_USUARIO_ACOES || estado == ESTADO_AUDITORIA ||
                 estado == ESTADO_STATUS || estado == ESTADO_ALERTAS || estado == ESTADO_SOBRE);
  if (emMenu && !feedbackPiscando() && millis() - ultimaInteracao >= MENU_TIMEOUT_MS) {
    voltarParaUsuarios = false;     // sai do modo Admin
    mudarEstado(ESTADO_OCIOSO);
  }

  // 7) Decide o que fazer conforme o estado atual
  switch (estado) {
    case ESTADO_INICIO:            tratarInicio();              break;
    case ESTADO_PRIMEIRO_USO:      tratarPrimeiroUso(ev);       break;
    case ESTADO_OCIOSO:            tratarOcioso(ev);            break;
    case ESTADO_MENU:              tratarMenu(ev);              break;
    case ESTADO_ESCOLHER_USUARIO:  tratarEscolherUsuario(ev);   break;
    case ESTADO_AGUARDAR_CARTAO:   tratarAguardarCartao(ev);    break;
    case ESTADO_SENHA:             tratarSenha(ev);             break;
    case ESTADO_BLOQUEADO:         tratarBloqueado(ev);         break;
    case ESTADO_HISTORICO:         tratarHistorico(ev);         break;
    case ESTADO_USUARIOS:          tratarUsuarios(ev);          break;
    case ESTADO_USUARIO_ACOES:     tratarUsuarioAcoes(ev);      break;
    case ESTADO_AUDITORIA:         tratarAuditoria(ev);         break;
    case ESTADO_STATUS:            tratarStatus(ev);            break;
    case ESTADO_ALERTAS:           tratarAlertas(ev);           break;
    case ESTADO_SOBRE:             tratarSobre(ev);             break;
    case ESTADO_RESULTADO:
      // mostra o resultado por alguns segundos (ou ate apertar OK)
      if (millis() - estadoDesde >= duracaoResultado || ev == EV_OK) irParaInicio();
      break;
  }
}

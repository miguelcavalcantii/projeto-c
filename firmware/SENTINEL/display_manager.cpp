#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "security_config.h"
#include "display_manager.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
static bool lcdOk = false;

// Copia do que esta na tela: 2 linhas de 16 caracteres (+1 do fim de texto)
static char copiaTela[LCD_ROWS][LCD_COLS + 1];
static bool telaMudou = true;

// Cada icone e uma grade de 5x8 pixels. Cada numero e uma linha:
// o bit 1 acende o pixel. Ex.: 31 = 11111 (linha toda acesa).
// (Os mesmos desenhos estao no painel do PC, para o espelho ficar igual.)
static byte desenhoCerto[8]   = {0, 1, 3, 22, 28, 8, 0, 0};
static byte desenhoErro[8]    = {0, 17, 10, 4, 10, 17, 0, 0};
static byte desenhoAlerta[8]  = {4, 14, 14, 14, 4, 0, 4, 0};
static byte desenhoCadeado[8] = {14, 17, 17, 31, 27, 27, 31, 0};
static byte desenhoAberto[8]  = {14, 16, 16, 31, 27, 27, 31, 0};
static byte desenhoEscudo[8]  = {31, 17, 21, 21, 17, 10, 4, 0};
static byte desenhoRelogio[8] = {0, 14, 21, 23, 17, 14, 0, 0};

static void limparCopia() {
  for (int l = 0; l < LCD_ROWS; l++) {
    for (int c = 0; c < LCD_COLS; c++) copiaTela[l][c] = ' ';
    copiaTela[l][LCD_COLS] = '\0';
  }
}

bool displayIniciar() {
  limparCopia();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Pergunta no barramento I2C se tem alguem no endereco do LCD
  Wire.beginTransmission(LCD_I2C_ADDR);
  lcdOk = (Wire.endTransmission() == 0);
  if (!lcdOk) return false;

  lcd.init();
  lcd.backlight();
  lcd.createChar(ICONE_CERTO, desenhoCerto);
  lcd.createChar(ICONE_ERRO, desenhoErro);
  lcd.createChar(ICONE_ALERTA, desenhoAlerta);
  lcd.createChar(ICONE_CADEADO, desenhoCadeado);
  lcd.createChar(ICONE_CADEADO_ABERTO, desenhoAberto);
  lcd.createChar(ICONE_ESCUDO, desenhoEscudo);
  lcd.createChar(ICONE_RELOGIO, desenhoRelogio);
  lcd.clear();
  return true;
}

bool displayOk() {
  return lcdOk;
}

// Escreve uma linha inteira (completa com espacos para apagar o texto antigo)
void displayLinha(int linha, String texto) {
  if (texto.length() > LCD_COLS) texto = texto.substring(0, LCD_COLS);
  while (texto.length() < LCD_COLS) texto += " ";

  for (int c = 0; c < LCD_COLS; c++) copiaTela[linha][c] = texto[c];
  telaMudou = true;

  if (lcdOk) {
    lcd.setCursor(0, linha);
    lcd.print(texto);
  }
}

void displayMostrar(String linha1, String linha2) {
  displayLinha(0, linha1);
  displayLinha(1, linha2);
  if (!lcdOk) Serial.println("# [LCD] " + linha1 + " | " + linha2);
}

// Desenha um icone (ou caractere especial) numa posicao da tela
void displayIcone(int coluna, int linha, byte icone) {
  copiaTela[linha][coluna] = (char)icone;
  telaMudou = true;
  if (lcdOk) {
    lcd.setCursor(coluna, linha);
    lcd.write(icone);
  }
}

void displayMostrarComIcone(byte icone, String linha1, String linha2) {
  displayMostrar(" " + linha1, linha2);  // coluna 0: lugar do icone
  displayIcone(0, 0, icone);
}

// Barra de progresso de 16 blocos. Ex.: 50% = 8 blocos cheios.
void displayBarra(int linha, int porcentagem) {
  if (porcentagem < 0) porcentagem = 0;
  if (porcentagem > 100) porcentagem = 100;
  int cheios = porcentagem * LCD_COLS / 100;
  displayLinha(linha, "");
  for (int c = 0; c < cheios; c++) displayIcone(c, linha, CHAR_BLOCO);
}

// Linha 1: "-> Entrar     1/5"   Linha 2: "   Status"
void displayMostrarMenu(const String itens[], int total, int selecionado) {
  String contador = String(selecionado + 1) + "/" + String(total);
  String linha1 = "  " + itens[selecionado];
  while (linha1.length() < LCD_COLS - contador.length()) linha1 += " ";
  linha1 += contador;

  int proximo = (selecionado + 1) % total;
  displayMostrar(linha1, "  " + itens[proximo]);
  displayIcone(0, 0, CHAR_SETA);
}

// Linha 1: "[cadeado] Digite a senha"  (ou "Nova senha", "Confirme a senha")
// Linha 2: "   *  * [5] _"   (so o digito sendo escolhido aparece)
void displayMostrarSenha(String titulo, int digitados, int digitoAtual) {
  String linha2 = "  ";
  for (int i = 0; i < PIN_LENGTH; i++) {
    if (i < digitados) linha2 += " * ";
    else if (i == digitados) linha2 += "[" + String(digitoAtual) + "]";
    else linha2 += " _ ";
  }
  displayMostrarComIcone(ICONE_CADEADO, titulo, linha2);
}

void displayLuz(bool ligada) {
  if (!lcdOk) return;
  if (ligada) lcd.backlight();
  else lcd.noBacklight();
}

String displayCopiaLinha(int linha) {
  return String(copiaTela[linha]);
}

bool displayMudou() {
  bool m = telaMudou;
  telaMudou = false;
  return m;
}

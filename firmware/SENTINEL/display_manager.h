// =============================================================
// SENTINEL - display_manager
// Responsabilidade: tudo que aparece no LCD. Nenhum outro modulo
// conversa com a biblioteca do LCD diretamente.
// Tambem guarda uma COPIA do que esta na tela (para o painel do PC
// poder mostrar um "espelho" do LCD).
// Se o LCD nao for encontrado, as telas sao impressas no Serial.
// =============================================================
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// Icones desenhados por nos (caracteres personalizados do LCD).
// O LCD aceita 8 desenhos (0 a 7). Nao usamos o 0 porque o byte 0
// marca o fim de um texto em C e atrapalharia o espelho do LCD.
#define ICONE_CERTO          1
#define ICONE_ERRO           2
#define ICONE_ALERTA         3
#define ICONE_CADEADO        4
#define ICONE_CADEADO_ABERTO 5
#define ICONE_ESCUDO         6
#define ICONE_RELOGIO        7

// Caracteres que ja existem na memoria do proprio LCD (HD44780)
#define CHAR_SETA   0x7E   // seta para a direita
#define CHAR_BLOCO  0xFF   // quadrado todo preenchido

bool displayIniciar();          // true = LCD encontrado no I2C
bool displayOk();

void displayMostrar(String linha1, String linha2);
void displayLinha(int linha, String texto);
void displayIcone(int coluna, int linha, byte icone);
void displayMostrarComIcone(byte icone, String linha1, String linha2);
void displayBarra(int linha, int porcentagem);
void displayMostrarMenu(const String itens[], int total, int selecionado);
void displayMostrarSenha(String titulo, int digitados, int digitoAtual);
void displayLuz(bool ligada);

// Para o espelho do LCD no painel do computador
String displayCopiaLinha(int linha);
bool displayMudou();            // true se a tela mudou desde a ultima consulta

#endif

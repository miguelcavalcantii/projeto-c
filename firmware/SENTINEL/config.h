// =============================================================
// SENTINEL - config.h
// Mapa de pinos e configuracoes gerais de hardware.
// Toda ligacao fisica do projeto esta definida AQUI, em um so lugar.
// Se mudar um fio na protoboard, mude apenas este arquivo.
// Documentacao completa: docs/01_hardware_e_gpios.md
// =============================================================
#ifndef CONFIG_H
#define CONFIG_H

// ---------- Versao do firmware ----------
#define FIRMWARE_VERSION "0.7.0"

// ---------- RFID RC522 (barramento SPI) ----------
// O firmware funciona COM ou SEM o leitor: ele detecta sozinho se o
// RC522 esta ligado (e tambem se for plugado com a placa ja ligada).
// ALIMENTAR SEMPRE COM 3,3 V. Ligacao: docs/09_rfid.md
#define PIN_RFID_SS    27
#define PIN_RFID_SCK   18
#define PIN_RFID_MOSI  23
#define PIN_RFID_MISO  19
#define PIN_RFID_RST   26

// ---------- LCD 16x2 com adaptador I2C (PCF8574) ----------
#define PIN_I2C_SDA    21
#define PIN_I2C_SCL    22
#define LCD_I2C_ADDR   0x27 // confirmar com o teste 05_i2c_scanner (pode ser 0x3F)
#define LCD_COLS       16
#define LCD_ROWS       2

// ---------- Joystick analogico ----------
// GPIO 34 e 35 sao SOMENTE ENTRADA e pertencem ao ADC1,
// que continua funcionando com o Wi-Fi ligado (o ADC2 nao).
#define PIN_JOY_X      34
#define PIN_JOY_Y      35
#define JOY_LIMITE_BAIXO 1000   // abaixo disso = empurrado para um lado
#define JOY_LIMITE_ALTO  3000   // acima disso  = empurrado para o outro
// Se "cima" e "baixo" (ou esquerda/direita) sairem trocados, mude para true:
#define JOY_INVERTER_X  false
#define JOY_INVERTER_Y  false
// Segurar o joystick repete o movimento (como segurar uma tecla)
#define JOY_ATRASO_REPETICAO_MS    500  // espera antes de comecar a repetir
#define JOY_INTERVALO_REPETICAO_MS 200  // intervalo entre repeticoes

// ---------- Botao de confirmacao ----------
// Ligado entre o GPIO e o GND, usando o pull-up interno.
// Solto = HIGH, apertado = LOW.
#define PIN_BUTTON     25
#define DEBOUNCE_MS    40
#define BOTAO_LONGO_MS 800      // segurar mais que isso = "cancelar/voltar"

// ---------- LEDs ----------
// REMOVIDOS por falta de cabos. A funcao deles foi para o LCD
// (feedback_manager) e para o buzzer.

// ---------- Buzzer (bip e alarme) ----------
// Ligacao: perna + (a mais comprida) no GPIO 32, perna - no GND.
#define PIN_BUZZER     32
#define BUZZER_LIGADO  true     // false = sistema mudo (para apresentar sem barulho)
#define BUZZER_ATIVO   false    // true se for buzzer ATIVO (adesivo em cima, fundo fechado)
                                // e o som sair estranho; passivo/nao sei = false
#define BUZZER_BIP_CURTO_MS   100    // bips de sucesso e alerta
#define BUZZER_ERRO_MS        500    // bip longo de senha negada
#define BUZZER_SIRENE_CICLOS  10     // sirene do bloqueio: 10 x 0,5 s = 5 segundos
#define BUZZER_LEMBRETE_MS    15000  // bloqueado: bip curto a cada 15 s

// ---------- Usuarios ----------
// Ate 4 usuarios. O 1o (posicao 0) e o ADMINISTRADOR: a senha criada no
// "Primeiro uso" e a dele. Nomes com no maximo 8 letras, sem acento.
#define NOME_USUARIO_0 "Admin"
#define NOME_USUARIO_1 "Miguel"
#define NOME_USUARIO_2 "Vinicius"
#define NOME_USUARIO_3 "Petrus"

// ---------- Serial ----------
#define SERIAL_BAUD    115200

#endif

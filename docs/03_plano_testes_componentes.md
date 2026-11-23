# 03 – Plano de testes individuais dos componentes (Etapa 2)

Regra: **não avançar para a Etapa 3 enquanto os testes 01 a 09 não passarem.** A coluna "Resultado obtido" é preenchida **pelo grupo**, com o que realmente aconteceu. Se falhar, anote o erro.

Para cada teste: abra o `.ino` da pasta `tests/hardware/NN_nome/` na Arduino IDE, grave na placa e abra o Monitor Serial em **115200**.

| # | Componente | Objetivo | Procedimento | Resultado esperado | Resultado obtido |
|---|---|---|---|---|---|
| 01 | ESP32 | Placa grava e roda | Gravar sem nada ligado | Serial mostra "ESP32-D0WD…", 2 núcleos, 240 MHz e o tempo contando | |
| 02 | LEDs | Saídas digitais | Ligar os LEDs com resistor no 32 e no 33 | Os dois alternam a cada 0,5 s | |
| 03 | Botão | Entrada + pull-up + debounce | Botão entre o 25 e o GND; apertar 10 vezes | Contador chega exatamente a 10 | |
| 04 | Joystick | ADC1 | Joystick em **3,3 V**, eixos no 34 e no 35 | Parado ≈ 1800–2000; nos extremos ≈ 0 e 4095; direções detectadas | Centro X=__ Y=__ |
| 05 | I2C | Descobrir o endereço do LCD | Fazer a seção 4 do doc 01 **antes**; rodar o scanner | Aparece 0x27 ou 0x3F (e talvez 0x3C, do display embutido) | Endereços: |
| 06 | LCD | Escrita no display | Ajustar `LCD_ADDR`; regular o contraste | "SENTINEL v0.1" e o contador de segundos | |
| 07 | RFID | SPI + leitura de UID | RC522 em **3,3 V** | Versão 0x91/0x92 (ou 0x88/0xB2, clone); UID e tipo do cartão aparecem | Versão: 0x__ Tipo do cartão: |
| 08 | Integrado | Tudo junto sem conflito | Todos ligados | O joystick muda o dígito, o botão acende o verde e o cartão acende o vermelho e aparece no LCD | |
| 09 | Buzzer | Saída tone() no GPIO 32 | Perna + no 32, perna − no GND | Os 4 sons (liberado, negado, alerta, sirene) se repetem a cada 3 s | |

## Problemas comuns

- **Não grava / "Failed to connect":** segure o botão BOOT da placa quando aparecer "Connecting…". Confira se o cabo USB transmite dados (não é só de carga) e se o driver CP210x ou CH340 está instalado.
- **LCD aceso mas sem texto:** contraste (potenciômetro atrás do módulo) ou endereço errado.
- **RC522 mostra versão 0x00 ou 0xFF:** fio do SS/MISO trocado, falta de GND comum ou alimentação errada.
- **Joystick fica preso em 4095:** VCC do módulo trocado com algum eixo, ou módulo alimentado em 5 V.
- **A placa reinicia sozinha quando o RFID ou o LCD liga:** a fonte USB está fraca. Troque de cabo ou de porta.

## Informações para coletar durante os testes

1. Versão do chip RC522 (teste 07).
2. Tipo do cartão e do chaveiro (teste 07: "MIFARE 1KB", "MIFARE Ultralight"...).
3. Endereço I2C do LCD e a tensão medida no SDA/SCL (teste 05 + doc 01, seção 4).
4. Endereço do display embutido da placa, se aparecer no scanner.
5. Valores de centro do joystick (teste 04).

# Histórico de versões

## v0.7.0 · 23/11/2026
- **Buzzer no GPIO 32:** módulo `buzzer_manager` toca sons sem travar o sistema (millis, sem delay) e com prioridade (um bip não corta a sirene).
- Sons: início, acesso liberado, negado, alerta (3ª/4ª falha), **sirene de 5 s no bloqueio**, bip de lembrete a cada 15 s enquanto bloqueado, bip ao mexer no sistema bloqueado, sirene para log violado e reinício durante bloqueio, alerta quando o leitor RFID some.
- Tempos e liga/desliga no `config.h`; teste de bancada `09_buzzer`.

## v0.6.0 · 17/11/2026
- **Usuários:** vetor de structs com até 4 pessoas, cada uma com PIN e crachá; área do Admin no LCD (definir PIN, cadastrar ou tirar crachá, revogar/reativar).
- **RFID RC522:** autodetecção e autoteste a cada 10 s, id protegido por HMAC (o UID nunca é guardado), crachá + PIN (2 fatores) e "falha fechada" quando o leitor some.
- **Histórico:** fila circular com os últimos 10 eventos no LCD.
- **Painel:** 8 telas com menu lateral: incidentes, usuários, laboratório de força bruta (O(10ⁿ)) e hardware.
- **Firmware dividido em abas** (`fluxo_cartao`, `fluxo_senha`, `menus`, `telas`).
- **Fluxogramas** em Mermaid e PNG.

## v0.5.0
- **IDS:** alerta na 3ª falha e bloqueio na 5ª, com tempo progressivo (30 s ×2 até 15 min), estado na NVS e detecção de reinício durante o bloqueio.
- **Auditoria:** corrente HMAC-SHA256 no LittleFS, com verificação, simulação de ataque, novo log protegido por senha e rotação com âncora.

## v0.4.0
- **Senha:** PBKDF2-HMAC-SHA256 (10.000 iterações) com salt na NVS, cadastro com confirmação e comparação em tempo constante.

## v0.3.0
- Telas caprichadas no LCD (ícones, barra de boot, descanso de tela) e primeiro painel via Web Serial.

## v0.2.0
- LEDs substituídos por avisos no LCD; joystick e botão viram eventos; primeira máquina de estados.

## v0.1.0
- Estrutura do projeto, mapa de GPIOs e testes de bancada de cada componente.

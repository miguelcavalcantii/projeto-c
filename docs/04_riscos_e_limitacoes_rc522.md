# 04 – Riscos técnicos e limitações do RC522

## 1. O que o RC522 é (e o que não é)

O MFRC522 é um leitor de 13,56 MHz para o padrão **ISO/IEC 14443A**. Na prática, com a biblioteca MFRC522, ele lê:
- O **UID** de qualquer cartão 14443A.
- **MIFARE Classic 1K/4K** (os cartões e chaveiros que costumam vir no kit), incluindo leitura e escrita de blocos com a autenticação **Crypto1**.
- MIFARE Ultralight / NTAG.

Ele **não** oferece, com essa biblioteca, autenticação moderna como a dos cartões **MIFARE DESFire EV2/EV3** (AES). O alcance é curto, de ~3 a 5 cm.

## 2. Por que o UID não é uma autenticação

| Problema | Explicação |
|---|---|
| UID não é segredo | Qualquer leitor lê o UID, sem senha. É como um "nome de usuário" impresso no cartão. |
| UID pode ser copiado | Existem cartões "magic" (Gen1a/CUID) em que **o UID pode ser regravado**. Copiar o UID de um cartão para outro é barato e rápido. |
| UID pode ser emulado | Dispositivos como Proxmark3, Flipper Zero e ChameleonMini **fingem ser** um cartão com qualquer UID. |
| Replay | O UID é sempre o mesmo em todas as leituras. Não existe desafio-resposta, então "gravar e repetir" funciona. |
| UID de 4 bytes não é único | Cartões de 4 bytes (NUID) podem repetir entre fabricantes. |

## 3. Por que MIFARE Classic também não resolve

Os setores do MIFARE Classic são protegidos pelo algoritmo **Crypto1**, que é **quebrado desde 2008** (ataques *darkside*, *nested* e *hardnested*). Com o equipamento certo, as chaves e todo o conteúdo do cartão podem ser recuperados e copiados para um cartão "magic".

## 4. O que o SENTINEL faz de forma honesta

| Modo (laboratório) | Fatores | O que protege | O que NÃO protege |
|---|---|---|---|
| 1 – Só UID | Algo que você tem (fraco) | Quase nada: serve como demonstração da falha | Clonagem, emulação, replay |
| 2 – UID + PIN | Ter + saber | Um cartão clonado **sozinho** não abre. O PIN é protegido por bloqueio | Clone + PIN observado por cima do ombro, PIN fraco |
| 3 – Cartão criptográfico (futuro) | Ter (forte) + saber | Desafio-resposta com AES: o cartão não pode ser copiado facilmente | Exige hardware que **não temos** (PN532 ou similar + DESFire) |

**O que não vamos afirmar:** que UID + PIN impede a clonagem. Ele apenas faz com que a clonagem **sozinha** não seja suficiente.

**O que não vamos simular:** criptografia no cartão. O RC522 com cartões Classic não oferece isso de forma segura, e fingir que oferece seria pior do que não ter.

**Como a arquitetura permite evoluir:** o `rfid_manager` entrega para o resto do sistema apenas um "identificador de credencial". Hoje esse identificador é um hash do UID; no futuro, pode vir de um leitor com DESFire. A autenticação não muda.

**Detalhe de privacidade:** o sistema guarda e mostra no painel um **hash curto do UID** (ex.: os primeiros 8 caracteres de um HMAC), nunca o UID completo. Assim o painel não entrega o dado de que alguém precisaria para clonar o cartão.

## 5. Riscos técnicos gerais do projeto

| Risco | Impacto | Mitigação |
|---|---|---|
| LCD com pull-up de 5 V no I2C | Danificar GPIOs da ESP32 | Medir a tensão; usar um conversor de nível (doc 01, seção 4) |
| Display embutido da placa usando pinos do projeto | Conflito, comportamento estranho | Descobrir os pinos dele (scanner/manual da placa) |
| Módulo RC522 clone de baixa qualidade | Leituras falhando | Teste 07 (versão do chip); cabos curtos |
| Fonte USB fraca com tudo ligado | Reboots aleatórios (*brownout*) | Cabo/porta melhor; o LCD consome mais com o backlight ligado |
| Sem RTC | Horário dos eventos sem referência no modo offline | NTP + contador de boot; DS3231 como melhoria |
| PIN de 4 dígitos | 10.000 combinações; um dump da flash permite força bruta offline | Bloqueio progressivo; documentar; Flash Encryption no futuro |
| Desgaste da flash ao salvar o estado com frequência | Vida útil da NVS | Salvar só quando mudar ou a cada ~15 s durante um bloqueio |
| HTTPS pesado na ESP32 | Memória e desempenho | Avaliar na Etapa 6; HTTP só na rede local + documentar o risco |
| Prazo curto | Escopo grande demais | Seguir as etapas; o dashboard só depois de o núcleo funcionar |

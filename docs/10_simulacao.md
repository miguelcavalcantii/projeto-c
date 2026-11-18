# 10 – Simulação do circuito (Wokwi) e montagem no circuito.io

## Por que o Wokwi e não o circuito.io?

| | circuito.io | Wokwi |
|---|---|---|
| Aceita o circuito como **código** | ❌ Não: os componentes são escolhidos clicando | ✅ Sim: arquivo `diagram.json` |
| Simula o firmware rodando | ❌ Não | ✅ Sim (ESP32, LCD I2C, joystick, botão) |
| Tem ESP32 + LCD I2C + joystick | Opções limitadas | ✅ Sim |
| Tem o RC522 | ❌ | ❌ (o firmware detecta a falta e segue só com PIN) |

## Wokwi: o circuito pronto em 2 minutos

Os arquivos estão em [`simulacao/`](../simulacao):
- `diagram.json`: o circuito (ESP32, LCD 16x2 I2C, joystick, botão e as ligações)
- `libraries.txt`: as bibliotecas que o Wokwi instala sozinho

### Só o desenho do circuito
1. Entre em **wokwi.com** → **Start from scratch** → **ESP32**.
2. Clique na aba **diagram.json**, apague tudo e cole o conteúdo de `simulacao/diagram.json`.
3. O circuito aparece montado. Se os fios ficarem tortos, arraste as peças: os fios acompanham.

### Rodar o SENTINEL na simulação
1. Faça os passos de cima.
2. Clique na seta ao lado das abas → **New file** e crie **`libraries.txt`** com o conteúdo de `simulacao/libraries.txt`.
3. Apague o conteúdo do **sketch.ino** e cole o do `firmware/SENTINEL/SENTINEL.ino`.
4. Para cada um dos outros arquivos de `firmware/SENTINEL/` (os `.ino`, `.h` e `.cpp`), use **New file** com **o mesmo nome** e cole o conteúdo. O menu da seta também tem **Upload file(s)**, que permite enviar vários arquivos de uma vez.
5. Clique em **▶ (Start the simulation)**.
6. Use o **botão azul** como OK e o **joystick**, clicando e arrastando, para navegar.

**Observações:**
- **Sem RC522:** o Wokwi não tem esse leitor. O LCD mostra "Pressione OK" e o Serial mostra `RFID nao encontrado`, que é o comportamento esperado.
- **Direção do joystick:** se cima e baixo saírem trocados na simulação, mude `JOY_INVERTER_Y` para `true` no `config.h`.
- **Painel:** ele não conecta no Wokwi, porque a simulação não é uma porta USB de verdade. Use o Monitor Serial do próprio Wokwi para ver as mensagens JSON.

## circuito.io: montar na mão
O circuito.io não importa arquivos. Para ter o desenho lá:
1. Entre em **circuito.io/app** e escolha a placa mais próxima disponível.
2. Na busca de componentes, adicione: **LCD 16x2 I2C**, **Joystick** e **Pushbutton**. Se tiver, adicione um **RFID** também.
3. O site faz as ligações sozinho, **com os pinos que ele escolher**. Elas não vão bater com as do SENTINEL. Para a montagem real, use sempre o [esquema de montagem](img/esquema_montagem.png) e a tabela de pinos do [README](../README.md#-hardware-e-ligações).

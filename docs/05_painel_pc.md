# 05 – Painel no computador (via USB)

> Status: **implementado e validado na placa (v0.3); ampliado na v0.6** (8 telas, usuários, laboratório de força bruta).

## 1. O que é

`painel/index.html` é uma página que roda **no computador**, aberta no Chrome ou no Edge. Ela conversa com a ESP32 **pelo cabo USB**, usando a **Web Serial API** do navegador. Não precisa de Wi-Fi, servidor nem instalação.

Ela mostra ao vivo:
- Estado do sistema, tempo ligado e memória livre.
- **Espelho do LCD**, com os mesmos ícones 5x8 desenhados no firmware.
- Joystick (posição X/Y) e botão em tempo real.
- Gráfico de eventos por minuto e distribuição por severidade.
- Linha do tempo de eventos, com filtro, busca e exportação para CSV.
- Situação de cada módulo, separando o que já funciona do que é de etapas futuras.
- Console serial bruto.

## 2. Por que USB agora e Wi-Fi depois

| | Painel via USB (agora) | Painel via Wi-Fi (Etapa 6/7) |
|---|---|---|
| Precisa de rede | Não | Sim (rede local) |
| Quem pode acessar | Só quem está com o cabo plugado | Qualquer um na rede, por isso exige login, sessão etc. |
| Complexidade | Baixa | Alta (autenticação, sessões, validação HTTP) |
| Uso | Desenvolvimento e demonstração | Produto final |

O painel USB foi a forma mais rápida de ter visualização ao vivo, já validando o **protocolo de eventos**. Na Etapa 6, os mesmos eventos passam a ser servidos pela ESP32 via Wi-Fi, e o visual é reaproveitado.

## 3. Como usar

1. Grave o firmware v0.3 na placa.
2. **Feche o Monitor Serial da Arduino IDE.** Só um programa por vez pode usar a porta.
3. Abra `painel/index.html` no Chrome ou no Edge (dê dois cliques no arquivo).
4. Clique em **Conectar ESP32** e escolha a porta (ex.: "CP2102", "USB-SERIAL CH340" ou "COM3").
5. Sem a placa, o botão **Modo demo** gera dados simulados. Essa tela fica marcada com uma faixa azul dizendo que os dados são simulados.

## 4. Protocolo (1 mensagem JSON por linha, 115200 baud)

**ESP32 → PC**

| `t` | Quando | Campos |
|---|---|---|
| `boot` | Ao ligar | `v` (versão), `chip`, `lcd` (true/false) |
| `state` | Mudou de tela | `s` (INICIO, OCIOSO, MENU, SENHA, RESULTADO, STATUS, ALERTAS, SOBRE) |
| `hb` | A cada 1 s | `up` (s), `heap` (bytes), `state`, `jx`, `jy` (0–4095), `btn`, `lcd`, `pc`, `buz` (1 = buzzer ligado, 0 = mudo), `snd` (1 = tocando agora) |
| `lcd` | A tela mudou | `l1`, `l2` (ícones do LCD como `\u0001`…`\u0007`) |
| `evt` | Evento de segurança | `id`, `up`, `cat`, `type`, `sev` (info, aviso, alerta, critico), `msg` |
| `audit` | Resposta ao `AUDIT?` | `ok`, `total`, `bad`, `why`, `anchor`, `epoch` |
| `recbegin` / `rec` | Resposta ao `REC?` | registros da auditoria (id, hashes...) |
| `user` | Resposta ao `USERS?` | `i`, `name`, `admin`, `active`, `pin`, `card` (4 primeiros caracteres do id protegido), `n` |
| `pong` | Resposta ao PING | – |

As linhas que começam com `#` são mensagens de depuração e só aparecem no console.

**PC → ESP32**

| Comando | Efeito |
|---|---|
| `PING` | Avisa que o painel está conectado. É enviado a cada 2 s; sem PING por 5 s, a ESP considera o painel desconectado. |
| `LCD?` | Pede a cópia atual do LCD |
| `AUDIT?` | Verifica a corrente da auditoria (máx. 1 a cada 2 s) |
| `REC?` | Últimos 25 registros da auditoria (máx. 1 a cada 2 s) |
| `USERS?` | Lista de usuários (sem senha e sem UID) |

## 5. Decisões de segurança

- **A senha nunca trafega pelo cabo.** O painel recebe só o evento "senha capturada", sem os dígitos. Isso vale também para o Serial em geral.
- **Validação de entrada na ESP32:** só `PING` e `LCD?` são aceitos. Qualquer outro comando gera o evento `CMD_DESCONHECIDO` (aviso). Comandos com mais de 32 caracteres são descartados sem ocupar memória (`CMD_GRANDE_DEMAIS`). Isso já serve de base para o teste 13 do plano ("dados inválidos enviados").
- **Proteção contra XSS no painel:** todo texto vindo da placa é inserido com `textContent`, nunca como HTML.
- **Limites no painel:** o buffer de leitura é descartado se passar de 4 KB sem quebra de linha, e o log guarda no máximo 500 eventos.
- **Limitação honesta:** o canal USB não tem autenticação. Quem tem acesso físico ao cabo tem acesso à porta serial. Isso é aceitável porque o canal é **só de leitura** (nenhum comando administrativo existe por ele). Comandos administrativos virão pelo painel Wi-Fi, com login.

## 6. Como foi verificado

| Verificação | Resultado |
|---|---|
| Painel em modo demo no Chrome headless (desktop 1440 px e celular 390 px) | Sem erros de JavaScript; layout correto nos dois |
| Firmware compilado com bibliotecas simuladas (g++) | Sem erros de sintaxe |
| JSON gerado pelo firmware (simulado no PC) validado com Python | Todas as mensagens são JSON válido |
| **Teste com a placa real** | **Pendente: fazer e registrar aqui** |

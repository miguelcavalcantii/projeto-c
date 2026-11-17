# 09 – RFID: ligação, cadastro de crachá e limitações

> Status: **firmware pronto (v0.6) e testado em simulação no PC.** O teste com o leitor real ainda precisa ser feito.

## 1. Antes de ligar: confira o módulo
- O RC522 costuma vir com a **barra de pinos solta**, não soldada. Sem solda o contato falha: os pinos precisam estar **soldados** ao módulo.
- O módulo tem 8 pinos: `SDA SCK MOSI MISO IRQ GND RST 3.3V`.
- ⚠️ **Nunca ligue o RC522 no 5 V.** Ele funciona só com 3,3 V.

## 2. Ligação (7 fios; o IRQ fica solto)

| RC522 | Vai em | Observação |
|---|---|---|
| SDA (SS) | **GPIO 27** (D27) | |
| SCK | **GPIO 18** (D18) | SPI |
| MOSI | **GPIO 23** (D23) | SPI |
| MISO | **GPIO 19** (D19) | SPI |
| IRQ | — | não liga |
| GND | trilho **−** da protoboard | mesmo trilho do joystick |
| RST | **GPIO 26** (D26) | |
| 3.3V | trilho **+** da protoboard | esse trilho vem do **3V3** da ESP |

Com cabos macho-fêmea: a **ponta fêmea vai no pino do RC522** e a **ponta macho** vai no pino da ESP (ou no trilho), do mesmo jeito que já foi feito com o joystick.

**Cabos:** hoje 9 dos 13 cabos estão em uso. O RFID precisa de 7, então faltam **3 cabos**.

**Pode plugar com a placa ligada?** O firmware tenta encontrar o leitor de novo a cada 10 s e avisa no painel (`RFID_CONECTADO`). Mesmo assim, o mais seguro é **ligar os fios com o USB desconectado**.

## 3. Como saber se funcionou
1. Grave o firmware v0.6 e abra o Monitor Serial (ou o painel).
2. No boot deve aparecer `# [OK] RFID RC522`, e no painel a página **Hardware** mostra "RFID RC522: online".
3. No LCD, a tela de espera passa a dizer **"Cracha ou OK"** (sem o leitor, ela diz "Pressione OK").
4. Status → página 4/8 mostra `RFID: ok 0x92` (0x91/0x92 = original; 0x88/0xB2 = clone, que também funciona).
5. Se aparecer "RFID: ausente", rode o teste `tests/hardware/07_rfid` e confira os fios e a solda.

## 4. Cadastrar um crachá (precisa, sim!)
Um crachá só é aceito depois de ser **cadastrado e associado a um usuário**, que é o processo de *enrollment*. O cadastro é feito **no próprio LCD**, depois de ligar o leitor:

1. Aperte **OK** → **Usuarios** → digite a **senha do Admin**.
2. Escolha a pessoa (ex.: `Miguel P`).
3. Se ela ainda não tiver PIN: **Definir PIN** → digite e confirme. Sem PIN, ninguém entra, porque o crachá sozinho não basta.
4. **Cad. cartao** → aproxime o crachá do leitor → **"Cracha salvo"**.
5. Na lista, o usuário passa a aparecer como `Miguel PC` (P = tem PIN, C = tem crachá).

**Para testar:** volte para a tela de espera, aproxime o crachá, e deve aparecer "Senha Miguel". Digite o PIN e veja **ACESSO LIBERADO**. No painel o evento aparece como "2 fatores: cracha + PIN".

**Outras ações do Admin:** *Tirar cart.* (desassocia o crachá) e *Revogar / Reativar* (bloqueia a pessoa sem apagar nada). O Admin não pode ser revogado, para o sistema não ficar trancado.

**Os nomes** dos usuários estão em `config.h` (`NOME_USUARIO_0` a `NOME_USUARIO_3`). O usuário 0 é sempre o Admin.

## 5. Vai funcionar com o crachá da faculdade?
Depende da tecnologia do crachá:

| Tipo de crachá | O RC522 lê? |
|---|---|
| 13,56 MHz MIFARE Classic / Ultralight / NTAG | ✅ Sim |
| 13,56 MHz MIFARE DESFire | ⚠️ Lê o UID; alguns usam **UID aleatório** (muda a cada leitura), e aí o cadastro não "pega" |
| 125 kHz (EM4100, HID Prox, "cartão de proximidade" antigo) | ❌ Não: é outra frequência |

**Teste rápido:** aproxime o crachá na tela de espera. Se nada acontecer, ele é provavelmente de 125 kHz. Se aparecer "ACESSO NEGADO" com o evento `CARTAO_DESCONHECIDO`, o leitor leu o crachá, e basta cadastrá-lo. Se o crachá não funcionar, use o **cartão ou o chaveiro que veio com o kit do RC522**.

**Cuidado ético e de privacidade:** o SENTINEL **não guarda nem mostra o UID** do crachá, só um `HMAC(chave, UID)`. Ainda assim, use só o seu próprio crachá e não tente copiar ou emular crachás de outras pessoas.

## 6. Decisões de segurança no código
- **Crachá = 1º fator, PIN = 2º fator.** O UID pode ser copiado (docs/04), então o crachá **nunca** libera sozinho.
- **Quem tem crachá cadastrado sempre precisa dele.** Se o leitor estiver desligado, o acesso é **negado** (`MFA_INDISPONIVEL`), o que se chama "falhar fechado". Desligar o leitor não serve para pular o crachá.
- **Crachá desconhecido, revogado ou de outra pessoa conta como falha no IDS**, e 5 falhas seguidas bloqueiam o sistema.
- **Autoteste a cada 10 s:** se o leitor parar de responder, gera `RFID_FALHA_COMUNICACAO`, que é a regra de "falhas repetidas de comunicação".
- **Busca linear no vetor de usuários** para achar o dono do crachá (O(n), com n = 4).

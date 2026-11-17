# 06 – Etapa 3: senha protegida com PBKDF2

> Status: **implementado (v0.4).** A lógica foi testada no PC (resultados abaixo). **O teste na placa ainda precisa ser feito e registrado.**

## 1. Como funciona

### Cadastro (primeiro uso ou "Nova senha")
1. O usuário digita a senha nova e depois **confirma** (digita de novo). Se as duas digitações forem diferentes, nada é salvo.
2. A ESP32 sorteia um **salt** de 16 bytes no gerador de números aleatórios do hardware.
3. Ela calcula `hash = PBKDF2-HMAC-SHA256(senha, salt, 10.000 iterações)`.
4. Na **NVS** (memória que sobrevive ao desligar) são gravados apenas o **salt**, o **hash** e o **número de iterações**. **A senha em si nunca é gravada.**

### Verificação ("Entrar")
1. A ESP lê o salt e o hash da NVS.
2. Recalcula o PBKDF2 com a senha digitada e o **mesmo salt**.
3. Compara os dois hashes com uma **comparação de tempo constante**.
4. Mostra **ACESSO LIBERADO** se forem iguais. Se forem diferentes, mostra **ACESSO NEGADO** e soma uma falha seguida.

### Trocar a senha
Para trocar, é preciso digitar a **senha atual** primeiro. Errar a senha atual também conta como falha.

## 2. Por que cada decisão

| Decisão | Motivo |
|---|---|
| Hash em vez da senha | Quem ler a memória não encontra a senha escrita |
| **Salt** individual | Dois sistemas com a mesma senha têm hashes diferentes; impede o uso de tabelas prontas (*rainbow tables*) |
| **PBKDF2** com 10.000 iterações | Cada tentativa custa 10.000 HMACs, o que deixa a força bruta lenta. Um SHA-256 simples seria rápido demais |
| Nº de iterações salvo junto | Permite aumentar no futuro sem invalidar a senha já cadastrada |
| Comparação de tempo constante | O tempo de resposta não revela "quantos bytes acertou" (*timing attack*) |
| Apagar os vetores depois do uso | A senha não fica "esquecida" na RAM |
| Dígito inicial sorteado | Quem observa não descobre a senha contando os movimentos do joystick |
| Senha nunca enviada ao Serial ou ao painel | Só eventos como "ACESSO_NEGADO" saem da placa |

## 3. Implementação

- `crypto_manager.cpp`: PBKDF2 implementado passo a passo sobre o HMAC-SHA256 do mbedTLS (U1 = HMAC(senha, salt‖1); Ui = HMAC(senha, Ui-1); resultado = XOR de todos).
- `authentication.cpp`: cadastro e verificação, gravação na NVS (biblioteca `Preferences`).
- `SENTINEL.ino`: estados `PRIMEIRO_USO` e `SENHA`, com 4 modos: ENTRAR, ATUAL, NOVA e CONFIRMAR.

## 4. Verificação feita no PC (antes de ir para a placa)

| Verificação | Resultado |
|---|---|
| PBKDF2 do firmware × `hashlib.pbkdf2_hmac` do Python, 3 casos (incluindo o vetor oficial `password`/`salt`/4096 → `c5e478d5…`) | **Idêntico nos 3** |
| Cadastrar `1234` → verificar `1234` / `1235` | Aceitou / recusou |
| Simular reinício (nova leitura da NVS) e verificar `1234` | Continuou cadastrada e aceitou |
| Comparação segura com vetores iguais / diferentes | true / false |

## 5. Roteiro de teste na placa (preencher "Obtido")

| # | Teste | Procedimento | Esperado | Obtido |
|---|---|---|---|---|
| 3.1 | Primeiro uso | Gravar com a memória apagada | LCD mostra "Primeiro uso / OK = criar senha" | |
| 3.2 | Senhas diferentes no cadastro | Digitar 1234 e confirmar com 1235 | "Não conferem"; nada é salvo; volta para "Primeiro uso" | |
| 3.3 | Cadastro | Digitar e confirmar a mesma senha | "Senha salva"; painel mostra `SENHA_CADASTRADA` com o tempo do PBKDF2 | |
| 3.4 | Senha correta (teste 3 do plano) | Menu → Entrar → senha certa | "ACESSO LIBERADO", evento `ACESSO_LIBERADO` | |
| 3.5 | Senha incorreta (teste 4 do plano) | Menu → Entrar → senha errada | "ACESSO NEGADO / Falhas: 1", evento `ACESSO_NEGADO` | |
| 3.6 | Alerta de força bruta | Errar 3 vezes seguidas | Evento `POSSIVEL_FORCA_BRUTA` (alerta) no painel | |
| 3.7 | Persistência | Reiniciar a placa (EN/RST) e entrar com a senha | Continua cadastrada e é aceita | |
| 3.8 | Trocar senha | Menu → Nova senha → atual → nova → confirma | Senha antiga deixa de funcionar | |
| 3.9 | Tempo do PBKDF2 | Status → página 4 | Anotar o tempo: ____ ms | |

## 6. Como apagar a senha (laboratório)

**Não existe** "porta dos fundos" no firmware para apagar a senha, de propósito. Para começar do zero:
Arduino IDE → **Ferramentas → Erase All Flash Before Sketch Upload → Enabled** → gravar de novo → depois voltar para **Disabled**.

Isso exige acesso físico e a IDE. Na prática equivale a reinstalar o equipamento.

## 7. Limitações (documentadas, não escondidas)

- **PIN de 4 dígitos = 10.000 combinações.** Quem conseguir **copiar a flash** (acesso físico + ferramenta) pode testar as 10.000 senhas no PC, porque o PBKDF2 só deixa isso mais lento. A defesa real contra quem está no teclado é o **bloqueio da Etapa 4**. Contra a cópia da flash, a defesa seria a **Flash Encryption** da ESP32 (melhoria futura).
- **As falhas seguidas ficam só na RAM nesta etapa.** Reiniciar zera o contador. Isso é corrigido na **Etapa 4** (contador e bloqueio gravados na NVS).
- **Gerador aleatório:** segundo a documentação da Espressif, o gerador de hardware é mais forte com o Wi-Fi/Bluetooth ligado. Com o Wi-Fi da Etapa 6, isso melhora.
- **O cálculo do PBKDF2 trava a tela por uma fração de segundo** (por isso aparece "Verificando..."). Se ficar lento demais na placa, `KDF_ITERATIONS` pode ser ajustado em `security_config.h`.
- **Um PIN do sistema** (não por usuário) nesta etapa. Com o RFID, cada cartão terá o seu próprio PIN.

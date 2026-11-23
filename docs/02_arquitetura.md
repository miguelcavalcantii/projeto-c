# 02 – Arquitetura do SENTINEL

## 1. Decisão: Arduino IDE (em vez de PlatformIO)

| | Arduino IDE 2.x | PlatformIO |
|---|---|---|
| Curva de aprendizado | Baixa: o grupo já conhece | Média |
| Vários arquivos .h/.cpp | Sim (abas na mesma pasta) | Sim (src/ e include/) |
| Testes unitários automáticos | Não tem nativo | Tem (Unity) |
| Envio dos arquivos do dashboard | Plugin "LittleFS Upload" ou HTML embutido no código | Nativo |

**Escolha:** Arduino IDE, pela familiaridade e pelo prazo curto. A estrutura modular continua: cada módulo é um par `.h/.cpp` na pasta do sketch. Os testes de segurança serão sketches de teste e roteiros manuais documentados, já que não vamos usar o Unity do PlatformIO.

**Configuração da IDE:** placa "ESP32 Dev Module", pacote **esp32 by Espressif Systems (versão 3.x)**, Upload Speed 921600, Partition Scheme "Default 4MB with spiffs".

## 2. Dependências

| Biblioteca | Origem | Uso | Etapa |
|---|---|---|---|
| esp32 (core Espressif 3.x) | Gerenciador de Placas | Base | 1 |
| MFRC522 (GithubCommunity) | Gerenciador de Bibliotecas | RFID | 2 |
| LiquidCrystal I2C (Frank de Brabander) | Gerenciador de Bibliotecas | LCD | 2 |
| Wire, SPI | Já vem no core | Barramentos | 2 |
| Preferences (NVS) | Já vem no core | Usuários, configuração, estado de bloqueio | 3–4 |
| mbedTLS | Já vem no core | SHA-256, HMAC, PBKDF2 | 3–5 |
| LittleFS | Já vem no core | Log de auditoria e arquivos do painel | 5–7 |
| WiFi, WebServer | Já vem no core | Rede e painel | 6 |

Obs.: a LiquidCrystal I2C pode mostrar um aviso de que "foi feita para AVR". Esse aviso pode ser ignorado, porque ela funciona na ESP32. Só duas bibliotecas são externas; o resto já vem com a placa. Isso segue a regra de não adicionar bibliotecas desnecessárias.

## 3. Camadas

```
┌──────────────── Comunicação ────────────────┐
│ telemetry_manager (painel USB, v0.3)        │
│ wifi_manager      web_server (painel/API)   │
├──────────────── Segurança ──────────────────┤
│ security_engine (IDS, bloqueio)             │
│ crypto_manager (SHA-256, HMAC, PBKDF2)      │
│ event_logger (auditoria encadeada)          │
├──────────────── Regras de negócio ──────────┤
│ authentication (máquina de estados MFA)     │
│ user_manager (usuários, permissões)         │
├──────────────── Hardware ───────────────────┤
│ rfid_manager  display_manager  input_manager│
│ (RC522)       (LCD)            (joystick+botão)
│               feedback_manager (avisos no LCD; substitui os LEDs) + buzzer_manager (sons)
└─────────────────────────────────────────────┘
```

Regra: uma camada só chama a camada de **baixo**. Por exemplo, o `authentication` pede "tem cartão?" ao `rfid_manager`, mas o `rfid_manager` não sabe nada sobre senha. Assim, trocar o RC522 por um PN532 no futuro mexe só no `rfid_manager`.

## 4. Máquina de estados da autenticação (Etapa 3)

```
 OCIOSO ──cartão lido──▶ CARTAO_LIDO ──desconhecido──▶ NEGADO
   ▲                         │ autorizado
   │                         ▼
   │                    DIGITANDO_PIN ──cancelar/timeout──▶ OCIOSO
   │                         │ 4 dígitos
   │                         ▼
   │                    VERIFICANDO ──errado──▶ NEGADO ──(5 erros)──▶ BLOQUEADO
   │                         │ certo                                    │
   │                         ▼                                          │
   └────(3 s)─────────── LIBERADO                  (tempo acabou) ◀─────┘
```

Cada estado é um `case` de um `switch` dentro do `loop()`. Ninguém usa `delay()` longo: os tempos são controlados com `millis()`. Assim, o Wi-Fi e o painel continuam respondendo enquanto alguém digita a senha.

## 5. Armazenamento

| Dado | Onde | Motivo |
|---|---|---|
| Usuários (nome, ID do cartão, hash+salt do PIN, permissão, ativo) | NVS (Preferences) | Poucos registros, precisa sobreviver ao reboot |
| Configuração (limites, tempos) | NVS | Idem |
| Estado de bloqueio (contador de erros, tempo restante, nº de bloqueios) | NVS | **Reiniciar não pode apagar o bloqueio** |
| Credenciais Wi-Fi e do admin | NVS, cadastradas no 1º boot pelo Serial | Nunca ficam no código-fonte |
| Log de auditoria | LittleFS (arquivo) | A NVS não é boa para centenas de registros |

## 6. Decisões de segurança já previstas (detalhadas nas etapas)

- **PIN:** PBKDF2-HMAC-SHA256 com salt individual de 16 bytes. Limitação honesta: um PIN de 4 dígitos tem só 10.000 combinações, então quem conseguir copiar a memória flash quebra o hash rapidamente, mesmo com PBKDF2. A proteção real é **bloqueio + tempo progressivo**. Proteger a flash (Flash Encryption) fica como melhoria futura documentada.
- **Bloqueio que sobrevive ao reboot:** sem um RTC, a ESP32 não sabe quanto tempo passou desligada. Solução: salvar na NVS o **tempo restante** de bloqueio a cada ~15 s. No boot, se estava bloqueada, ela continua bloqueada pelo tempo restante. Desligar a placa não "adianta o relógio".
- **Horário dos eventos:** NTP quando houver Wi-Fi. Sem Wi-Fi, o horário é registrado como "boot N + segundos desde o boot". Com um RTC DS3231 (futuro), isso fica resolvido.
- **Auditoria:** `hash_atual = HMAC-SHA256(chave_do_dispositivo, dados_do_evento + hash_anterior)`. Usar HMAC (e não só SHA-256) impede que alguém altere um registro e simplesmente recalcule a cadeia sem conhecer a chave.
- **Painel web:** HTTP na rede local, com login, sessão com token aleatório e expiração. HTTPS na ESP32 é possível, mas pesado e com certificado autoassinado. Vamos avaliar na Etapa 6 e documentar a escolha.

## 7. Estrutura de pastas

```
SENTINEL/
├── README.md
├── firmware/SENTINEL/        ← abrir SENTINEL.ino na Arduino IDE
│   ├── SENTINEL.ino          ← variáveis, setup, loop, troca de estado
│   ├── fluxo_cartao.ino      ← abas (a IDE junta todas)
│   ├── fluxo_senha.ino
│   ├── menus.ino
│   ├── telas.ino
│   ├── config.h              ← pinos e nomes dos usuários
│   ├── security_config.h     ← limites padrão
│   └── *_manager.h/.cpp      ← módulos (um por responsabilidade)
├── painel/index.html         ← painel SOC (Web Serial)
├── tests/hardware/           ← um sketch por componente (Etapa 2)
└── docs/                     ← documentação técnica e acadêmica
```

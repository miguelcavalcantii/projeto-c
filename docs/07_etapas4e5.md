# 07 – Etapas 4 e 5: IDS, bloqueio e auditoria criptográfica

> Status: **implementado (v0.5).** A lógica foi simulada no PC (resultados na seção 5). **O teste na placa ainda precisa ser feito e registrado.**

## 1. Etapa 4: detecção de intrusão e bloqueio (`security_engine`)

### Regras
| Senha errada seguida | O que acontece | Evento |
|---|---|---|
| 1ª e 2ª | Registra e mostra "Restam N" | `ACESSO_NEGADO` (aviso) |
| 3ª e 4ª | **Alerta** no LCD e no painel | `POSSIVEL_FORCA_BRUTA` (alerta) |
| 5ª | **Bloqueio** com contagem regressiva | `BLOQUEIO_ATIVADO` (crítico) |
| Mexer durante o bloqueio | Registra a tentativa (1ª e depois a cada 5) | `TENTATIVA_DURANTE_BLOQUEIO` (alerta) |
| Reiniciar durante o bloqueio | O bloqueio **continua** | `REINICIO_DURANTE_BLOQUEIO` (alerta) |
| Senha certa | Zera falhas e nível | `ACESSO_LIBERADO` |

Os limites ficam em `security_config.h` (3 e 5) e também na NVS. A função `secDefinirLimites()` (que o painel web da Etapa 6 vai usar) **recusa valores perigosos**, por exemplo alerta ≥ bloqueio, bloqueio maior que 20 tentativas ou tempo maior que 1 h.

### Tempo progressivo
30 s → 60 s → 120 s → 240 s → … até no máximo **15 min**. Nunca é permanente, para o equipamento não ficar inutilizável. Acertar a senha volta o próximo bloqueio para 30 s (recuperação controlada).

### Proteção contra reinício
- As falhas e o tempo restante ficam na **NVS**. Uma falha é gravada **na hora**: desligar a placa logo depois de errar não apaga o erro.
- Durante o bloqueio, o tempo restante é gravado a cada **10 s**. Se a placa reiniciar, ela perde no máximo 10 s de progresso, ou seja, o bloqueio fica **um pouco mais longo, nunca mais curto**.
- Gravar só a cada 10 s, e não a cada segundo, poupa o desgaste da memória flash.

## 2. Etapa 5: auditoria criptográfica (`event_logger`)

### Formato de cada registro (uma linha no arquivo `/audit.log`, no LittleFS)
```
id;boot;uptime;categoria;tipo;severidade;usuario;detalhe;hashAnterior;hashAtual
```
`hashAtual = HMAC-SHA256(chave, "id;boot;…;detalhe;hashAnterior")`

- **Corrente:** cada registro carrega o hash do anterior. O primeiro de todos aponta para 64 zeros.
- **Chave:** 32 bytes aleatórios, criados uma única vez e guardados na NVS. Sem a chave, quem alterar o arquivo não consegue recalcular os hashes. É por isso que se usa **HMAC** e não só SHA-256.
- **Data e hora:** ainda não há relógio (RTC) nem internet, então cada registro usa o **número do boot + tempo ligado**. Com o NTP (Etapa 6) ou um DS3231, entra a data real.

### Verificação ("Auditoria → Verificar" no LCD, ou "Verificar corrente" no painel)
Para cada linha, ela confere:
1. Se a linha tem os 10 campos.
2. Se o **id** é o esperado, o que detecta uma linha apagada no meio.
3. Se o **hash anterior** é igual ao hash da linha de cima, o que detecta uma corrente quebrada.
4. Se o **HMAC recalculado** é igual ao gravado, o que detecta conteúdo alterado.
5. No fim, se o último id bate com o que está na memória, o que detecta o final apagado durante a sessão.

### Demonstração de ataque ("Auditoria → Sim. ataque")
Altera o **tipo** de um registro do meio (por exemplo, `ACESSO_NEGADO` vira `ACESSO_LIBERADO`) **sem recalcular o HMAC**, como faria um invasor que não tem a chave. Depois, o "Verificar" mostra **LOG VIOLADO, registro #N**.
Por transparência, a própria simulação é registrada (`SIMULACAO_ADULTERACAO`). Num produto real essa opção não existiria.

### "Novo log" (protegido por senha)
Descarta o histórico e começa uma corrente nova. Exige a senha (`MODO_AUTORIZAR`), e uma senha errada conta como falha no IDS. Os ids **nunca repetem**, e o contador de **época** sobe a cada novo log, deixando visível que o histórico foi zerado.

### Rotação automática
Com 400 registros, o arquivo "vira a página": o último hash vira a **âncora** (gravada na NVS) e o próximo arquivo continua a corrente a partir dela. Num produto real, o arquivo antigo seria enviado para um servidor nesse momento (checkpoint).

## 3. Limitações (documentadas)
- **A corrente não impede apagar tudo.** Apagar o arquivo inteiro, ou restaurar uma cópia antiga, não é detectado depois de um reinício. Só aparece pela época ou por comparação com cópias externas. A defesa completa é enviar **checkpoints** (o último hash) para fora da placa (servidor ou painel), como melhoria futura.
- **A chave do HMAC fica na mesma flash.** Quem copiar a flash inteira pode recalcular a corrente. As defesas seriam Flash Encryption, um elemento seguro (ATECC608) ou checkpoints externos.
- **Os eventos de teste ("Alertas")** vão para o painel, mas **não** para a auditoria, porque não são eventos reais.
- **O tempo do bloqueio é contado com a placa ligada.** Desligada, o relógio não anda: o bloqueio só termina com a placa ligada pelo tempo todo. Isso é conservador, mas pode ser contornado com um RTC.

## 4. Painel
- Faixa vermelha **"SISTEMA BLOQUEADO"** com a contagem regressiva e o nível.
- Módulos **IDS** (falhas, tentativas restantes, total de bloqueios) e **Auditoria** (registros, época).
- Seção **Auditoria criptográfica**: botões "Verificar corrente" (`AUDIT?`) e "Carregar registros" (`REC?`), tabela com hash anterior e atual, e o registro violado destacado em vermelho.
- Os comandos pesados têm **limite de 1 a cada 2 s**. Pedir mais rápido que isso gera o evento `CMD_EXCESSIVO` (regra de "repetição excessiva de solicitações"). O canal continua **só de leitura**.
- A tabela de eventos mostra o **id da auditoria** e o **início do hash** de cada evento.

## 5. Verificação feita no PC (simulação com as mesmas funções do firmware)

| Cenário | Resultado obtido na simulação |
|---|---|
| Falhas 1 a 5 | registrada, registrada, ALERTA, ALERTA, BLOQUEIO 30 s |
| 12 s de bloqueio + reinício | Ligou bloqueada, com o evento de reinício; faltavam 20 s (o real era 18 s; nunca menos) |
| Bloqueios seguidos | 30 s → 60 s → 120 s |
| Acertar a senha e errar 5 de novo | Bloqueio volta a 30 s |
| `secDefinirLimites(6, 5, …)` | Recusado |
| 10 registros → verificar | Íntegra (10) |
| Reinício e novo registro | Corrente continua íntegra (11) |
| Simular ataque | **Violada no #6:** "conteúdo alterado (HMAC não confere)" |
| Novos registros depois do ataque | Continua violada (não "conserta") |
| Apagar uma linha do meio | **Violada:** "registro faltando ou fora de ordem" |
| Novo log | Íntegra; época sobe; ids continuam (não repetem) |
| 450 eventos (rotação em 400) | Íntegra; corrente continua pela âncora |
| Painel em modo demo (Chrome headless) | Sem erros de JavaScript; faixa de bloqueio e auditoria violada exibidas |

## 6. Roteiro de teste na placa (preencher "Obtido")

| # | Teste (nº do plano original) | Procedimento | Esperado | Obtido |
|---|---|---|---|---|
| 4.1 | Tentativas consecutivas (5) | Errar a senha 4 vezes | "Restam 4..1"; alerta na 3ª e na 4ª | |
| 4.2 | Bloqueio (6) | Errar a 5ª | "SIST. BLOQUEADO", contagem de 00:30, faixa vermelha no painel | |
| 4.3 | Reinício durante o bloqueio (7) | Apertar EN/RST no meio da contagem | Volta bloqueada; evento `REINICIO_DURANTE_BLOQUEIO` | |
| 4.4 | Bloqueio progressivo | Depois de liberar, errar mais 5 | Bloqueio de 01:00 | |
| 4.5 | Recuperação | Acertar a senha; errar 5 de novo | Bloqueio volta a 00:30 | |
| 5.1 | Corrente íntegra | Auditoria → Verificar | "Log integro, N reg. ok" | |
| 5.2 | Alteração de registro (8) | Auditoria → Sim. ataque → Verificar | "LOG VIOLADO! Registro #N"; painel destaca a linha | |
| 5.3 | Novo log | Auditoria → Novo log → senha | "Novo log criado, Época 1"; verificar fica íntegro | |
| 5.4 | Corrente no painel | Painel → Carregar registros | Hash anterior de cada linha = hash da linha de baixo | |
| 5.5 | Persistência | Reiniciar e verificar | Continua íntegra; boot nº aumentou | |

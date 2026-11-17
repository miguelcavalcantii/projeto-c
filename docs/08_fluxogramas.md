# 08 – Fluxogramas

Os diagramas abaixo são desenhados pelo próprio GitHub (Mermaid). As versões em imagem, para colocar nos slides, estão em `docs/img/`. Os arquivos-fonte ficam em `docs/fluxogramas/*.mmd`.

## 1. Visão geral do sistema
![Visão geral](img/1_visao_geral.png)

```mermaid
flowchart TD
    A([Liga a placa]) --> B[Inicia módulos:<br/>LCD, joystick, NVS, LittleFS, RFID]
    B --> C{Senha do Admin<br/>cadastrada?}
    C -- Não --> D[Primeiro uso:<br/>cadastrar senha do Admin]
    D --> E
    C -- Sim --> F{Bloqueio ativo<br/>na NVS?}
    F -- Sim --> G[Tela SISTEMA BLOQUEADO<br/>contagem regressiva]
    G -->|tempo acabou| E
    F -- Não --> E[Tela de espera<br/>'Crachá ou OK']
    E -->|aproxima crachá| H[Fluxo de autenticação<br/>crachá + PIN]
    E -->|botão OK| I[Menu principal]
    I --> J[Entrar] --> H
    I --> K[Histórico<br/>fila circular]
    I --> L[Usuários<br/>pede senha do Admin]
    I --> M[Auditoria<br/>verificar / simular ataque]
    I --> N[Status / Alertas / Sobre]
    H --> O{Resultado}
    O -- liberado --> P[ACESSO LIBERADO] --> E
    O -- negado --> Q[IDS conta a falha] --> R{5ª falha?}
    R -- Não --> E
    R -- Sim --> G
```

## 2. Autenticação em dois fatores (crachá + PIN)
![Autenticação MFA](img/2_autenticacao_mfa.png)

```mermaid
flowchart TD
    A([Crachá aproximado]) --> B[Calcula id protegido<br/>HMAC da chave + UID]
    B --> C{Busca linear no<br/>vetor de usuários}
    C -- não achou --> X1[CARTAO_DESCONHECIDO] --> F
    C -- achou --> D{Usuário ativo?}
    D -- Não --> X2[CARTAO_REVOGADO] --> F
    D -- Sim --> E[1º fator OK<br/>'algo que você tem']
    E --> G[Digita o PIN com o joystick<br/>dígito inicial sorteado]
    G --> H[PBKDF2-HMAC-SHA256<br/>10.000 iterações + salt]
    H --> I{Hash igual ao<br/>salvo na NVS?}
    I -- Sim --> J[2º fator OK<br/>'algo que você sabe']
    J --> K[ACESSO LIBERADO<br/>zera falhas · registra na auditoria]
    I -- Não --> X3[ACESSO_NEGADO] --> F
    F[[IDS: registrar falha]]
```

## 3. Detecção de força bruta e bloqueio
![IDS e bloqueio](img/3_ids_bloqueio.png)

```mermaid
flowchart TD
    A([Falha de autenticação]) --> B[falhas = falhas + 1<br/>grava na NVS na hora]
    B --> C{falhas >= 5?}
    C -- Sim --> D["tempo = min(30 s × 2^nível, 15 min)"]
    D --> E[nível = nível + 1<br/>falhas = 0]
    E --> F[BLOQUEIO_ATIVADO<br/>tela com contagem regressiva]
    F --> G{Reiniciou a placa?}
    G -- Sim --> H[Lê o tempo restante da NVS<br/>REINICIO_DURANTE_BLOQUEIO]
    H --> F
    G -- Não --> I{Tempo acabou?}
    I -- Não --> F
    I -- Sim --> J([DESBLOQUEIO])
    C -- Não --> K{falhas >= 3?}
    K -- Sim --> L[POSSIVEL_FORCA_BRUTA<br/>alerta no LCD e no painel]
    K -- Não --> M[ACESSO NEGADO<br/>'Restam N']
    N([Acesso liberado]) --> O[falhas = 0<br/>nível = 0]
```

## 4. Auditoria criptográfica (registrar e verificar)
![Auditoria](img/4_auditoria.png)

```mermaid
flowchart LR
    subgraph Registrar
      A([Evento]) --> B[corpo = id;boot;uptime;categoria;<br/>tipo;severidade;usuário;detalhe;hashAnterior]
      B --> C["hash = HMAC-SHA256(chave, corpo)"]
      C --> D[Grava a linha no<br/>/audit.log]
      D --> E[hashAnterior = hash]
    end
    subgraph Verificar
      F([Para cada linha]) --> G{id é o esperado?}
      G -- Não --> V1[VIOLADO:<br/>registro apagado]
      G -- Sim --> H{hashAnterior =<br/>hash da linha de cima?}
      H -- Não --> V2[VIOLADO:<br/>corrente quebrada]
      H -- Sim --> I{HMAC recalculado =<br/>hash gravado?}
      I -- Não --> V3[VIOLADO:<br/>conteúdo alterado]
      I -- Sim --> J[Próxima linha] --> F
    end
```

## 5. Arquitetura (módulos e camadas)
![Arquitetura](img/5_arquitetura.png)

A regra da arquitetura é que uma camada só usa as camadas de baixo. Por exemplo, a máquina de estados pede "tem cartão?" ao `rfid_manager`, mas o `rfid_manager` não sabe nada de senha.

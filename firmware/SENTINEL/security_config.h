// =============================================================
// SENTINEL - security_config.h
// Valores PADRAO das regras de seguranca.
// Os limites do IDS tambem ficam na NVS (security_engine). Aqui ficam
// os padroes usados na primeira inicializacao.
// =============================================================
#ifndef SECURITY_CONFIG_H
#define SECURITY_CONFIG_H

// ---------- Senha (PIN) ----------
#define PIN_LENGTH               4     // quantidade de digitos do PIN
#define SENHA_TIMEOUT_MS         30000 // 30 s sem mexer na tela de senha -> cancela

// ---------- Regras do IDS (tentativas incorretas consecutivas) ----------
#define IDS_ALERT_AFTER          3     // 3a tentativa errada -> alerta
#define IDS_LOCK_AFTER           5     // 5a tentativa errada -> bloqueio

// ---------- Bloqueio temporario com espera progressiva ----------
#define LOCK_BASE_SECONDS        30    // primeiro bloqueio: 30 s
#define LOCK_MULTIPLIER          2     // cada novo bloqueio dobra o tempo
#define LOCK_MAX_SECONDS         900   // teto de 15 min (nunca bloqueia para sempre)

// ---------- Derivacao de chave da senha (Etapa 3) ----------
// PBKDF2-HMAC-SHA256. O numero de iteracoes sera medido na placa
// na Etapa 3 para ficar abaixo de ~1 s por verificacao.
#define KDF_ITERATIONS           10000
#define KDF_SALT_BYTES           16
#define KDF_HASH_BYTES           32

// ---------- Auditoria (Etapa 5) ----------
#define AUDIT_MAX_REGISTROS      400   // ao encher, o log "vira a pagina" mantendo a corrente (ancora)
#define AUDIT_REGISTROS_PAINEL   25    // quantos registros o painel recebe por vez

// ---------- Usuarios e RFID ----------
#define MAX_USUARIOS             4     // tamanho do vetor de usuarios
#define CARTAO_TIMEOUT_MS        15000 // 15 s esperando o cracha -> cancela
#define RFID_TESTE_MS            10000 // a cada 10 s confere se o leitor ainda responde

// ---------- Historico ----------
#define HISTORICO_TAMANHO        10    // fila circular com os ultimos 10 eventos

#endif

// =============================================================
// SENTINEL - fluxo_senha.ino (aba)
// Digitacao, verificacao e cadastro de senha.
// =============================================================

// Sorteia o numero inicial de cada posicao da senha.
// Motivo de seguranca: se sempre comecasse no 0, quem estivesse olhando
// poderia contar quantas vezes o joystick foi empurrado e descobrir a senha.
int sortearDigito() {
  return random(0, 10);
}

// Apaga as senhas da memoria assim que nao forem mais necessarias
void limparSenha() {
  for (int i = 0; i < PIN_LENGTH; i++) digitos[i] = 0;
  qtdDigitados = 0;
}

void limparSenhaNova() {
  for (int i = 0; i < PIN_LENGTH; i++) senhaNova[i] = 0;
}

String tituloSenha() {
  if (modoSenha == MODO_NOVA) return "Nova senha";
  if (modoSenha == MODO_CONFIRMAR) return "Confirme senha";
  if (modoSenha == MODO_AUTORIZAR) return "Senha do Admin";
  return "Senha " + usuarioNome(usuarioAtual);
}

void iniciarSenha(ModoSenha modo) {
  modoSenha = modo;
  mudarEstado(ESTADO_SENHA);
}

void mostrarVerificando() {
  // O PBKDF2 leva uma fracao de segundo: avisamos antes de calcular
  displayMostrarComIcone(ICONE_RELOGIO, "Verificando...", "  PBKDF2-SHA256");
}

// Conta a falha no IDS e mostra o aviso certo. Devolve true se bloqueou.
// E usada para senha errada, cracha desconhecido, cracha revogado...
bool registrarFalha(String tipo, String descricao, String usuario) {
  int falhaNumero = secFalhas() + 1;               // antes de registrar (o bloqueio zera)
  ResultadoFalha resultado = secRegistrarFalha();
  registrarEvento("auth", tipo, SEV_AVISO, descricao + " (falha " + String(falhaNumero) + ")", usuario);

  if (resultado == FALHA_BLOQUEIO) {
    registrarEvento("ids", "BLOQUEIO_ATIVADO", SEV_CRITICO,
                    "Forca bruta: bloqueio de " + String(secUltimaDuracao()) + " s (nivel " +
                    String(secNivel()) + ")", "-");
    feedbackBloqueio(formatarMinSeg(secUltimaDuracao()));
    return true;
  }
  if (resultado == FALHA_ALERTA) {
    registrarEvento("ids", "POSSIVEL_FORCA_BRUTA", SEV_ALERTA,
                    String(falhaNumero) + " falhas seguidas, restam " + String(secTentativasRestantes()), "-");
    feedbackAlerta("Restam " + String(secTentativasRestantes()));
    return false;
  }
  feedbackErro("Restam " + String(secTentativasRestantes()));
  return false;
}

// Depois de uma falha: vai para o bloqueio ou mostra o resultado
void depoisDaFalha(bool bloqueou) {
  cartaoConfirmado = false;
  voltarParaUsuarios = false;
  if (bloqueou) mudarEstado(ESTADO_BLOQUEADO);
  else mostrarResultado(RESULTADO_MS);
}

// Chamada quando os 4 digitos foram digitados
void senhaCompleta() {
  if (modoSenha == MODO_ENTRAR) {
    String nome = usuarioNome(usuarioAtual);
    mostrarVerificando();
    bool certa = authVerificarSenha(usuarioAtual, digitos);
    limparSenha();
    if (certa) {
      secRegistrarSucesso();
      usuarioContarAcesso(usuarioAtual);
      String fatores = cartaoConfirmado ? "2 fatores: cracha + PIN" : "1 fator: so PIN";
      registrarEvento("auth", "ACESSO_LIBERADO", SEV_INFO,
                      fatores + " (PBKDF2 " + String(authUltimoTempoMs()) + " ms)", nome);
      cartaoConfirmado = false;
      feedbackSucesso("Ola, " + nome);
      mostrarResultado(RESULTADO_MS);
    } else {
      depoisDaFalha(registrarFalha("ACESSO_NEGADO", "PIN incorreto", nome));
    }
  }

  else if (modoSenha == MODO_AUTORIZAR) {
    // Acoes sensiveis exigem a senha do Admin (usuario 0)
    mostrarVerificando();
    bool certa = authVerificarSenha(0, digitos);
    limparSenha();
    if (!certa) {
      depoisDaFalha(registrarFalha("ADMIN_NEGADO", "Senha do Admin incorreta", usuarioNome(0)));
      return;
    }
    secRegistrarSucesso();
    if (acaoPendente == ACAO_USUARIOS) {
      registrarEvento("admin", "ADMIN_AUTORIZADO", SEV_INFO, "Entrou na gestao de usuarios", usuarioNome(0));
      montarListaAdmin();
      mudarEstado(ESTADO_USUARIOS);
    } else {
      auditNovoLog();
      registrarEvento("auditoria", "LOG_REINICIADO", SEV_AVISO,
                      "Novo log (epoca " + String(auditEpoca()) + "), historico anterior descartado", usuarioNome(0));
      displayMostrarComIcone(ICONE_CERTO, "Novo log criado", "  Epoca " + String(auditEpoca()));
      mostrarResultado(RESULTADO_MS);
    }
  }

  else if (modoSenha == MODO_NOVA) {
    for (int i = 0; i < PIN_LENGTH; i++) senhaNova[i] = digitos[i];
    limparSenha();
    iniciarSenha(MODO_CONFIRMAR);
  }

  else if (modoSenha == MODO_CONFIRMAR) {
    String nome = usuarioNome(usuarioAlvo);
    bool iguais = true;
    for (int i = 0; i < PIN_LENGTH; i++) {
      if (digitos[i] != senhaNova[i]) iguais = false;
    }
    limparSenha();

    if (!iguais) {
      limparSenhaNova();
      registrarEvento("auth", "SENHA_NAO_CONFERE", SEV_INFO, "As duas digitacoes foram diferentes", nome);
      feedbackErro("Nao conferem");
    } else {
      displayMostrarComIcone(ICONE_RELOGIO, "Salvando...", "  PBKDF2-SHA256");
      bool salvou = authCadastrarSenha(usuarioAlvo, senhaNova);
      limparSenhaNova();
      if (salvou) {
        registrarEvento("auth", "SENHA_CADASTRADA", SEV_INFO,
                        "Hash salvo na NVS (" + String(authIteracoes()) + " iter, " +
                        String(authUltimoTempoMs()) + " ms)", nome);
        displayMostrarComIcone(ICONE_CERTO, "Senha salva", "  " + nome);
        buzzerTocar(SOM_SUCESSO);
      } else {
        registrarEvento("auth", "ERRO_NVS", SEV_CRITICO, "Falha ao gravar a senha na memoria", nome);
        feedbackErro("Erro ao salvar");
      }
    }
    mostrarResultado(RESULTADO_MS);
  }
}

void cancelarSenha(String tipo, String descricao) {
  limparSenha();
  limparSenhaNova();
  cartaoConfirmado = false;
  registrarEvento("auth", tipo, tipo == "SENHA_TIMEOUT" ? SEV_AVISO : SEV_INFO, descricao, "-");
  if (!authSenhaCadastrada(0)) mudarEstado(ESTADO_PRIMEIRO_USO);   // cadastro e obrigatorio
  else if (voltarParaUsuarios) mudarEstado(ESTADO_USUARIO_ACOES);
  else mudarEstado(ESTADO_MENU);
}

void tratarSenha(EventoEntrada ev) {
  if (ev == EV_CIMA) {
    digitoAtual = (digitoAtual + 1) % 10;          // 9 -> 0
    displayMostrarSenha(tituloSenha(), qtdDigitados, digitoAtual);
  } else if (ev == EV_BAIXO) {
    digitoAtual = (digitoAtual + 9) % 10;          // 0 -> 9
    displayMostrarSenha(tituloSenha(), qtdDigitados, digitoAtual);
  } else if (ev == EV_ESQUERDA) {
    if (qtdDigitados > 0) qtdDigitados--;          // apaga o ultimo
    displayMostrarSenha(tituloSenha(), qtdDigitados, digitoAtual);
  } else if (ev == EV_OK) {
    digitos[qtdDigitados] = digitoAtual;
    qtdDigitados++;
    digitoAtual = sortearDigito();
    if (qtdDigitados == PIN_LENGTH) senhaCompleta();
    else displayMostrarSenha(tituloSenha(), qtdDigitados, digitoAtual);
  } else if (ev == EV_OK_LONGO) {
    cancelarSenha("SENHA_CANCELADA", "Usuario cancelou a digitacao");
    return;
  }

  // Ninguem mexeu por muito tempo: cancela por seguranca
  if (estado == ESTADO_SENHA && millis() - ultimaInteracao >= SENHA_TIMEOUT_MS) {
    cancelarSenha("SENHA_TIMEOUT", "Digitacao abandonada por 30 s");
  }
}

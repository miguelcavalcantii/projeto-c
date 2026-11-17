// =============================================================
// SENTINEL - fluxo_cartao.ino (aba)
// O que fazer quando um cracha e aproximado do leitor RFID.
//
// Autenticacao em 2 fatores (MFA):
//   1o fator: ALGO QUE VOCE TEM  -> o cracha (identifica o usuario)
//   2o fator: ALGO QUE VOCE SABE -> o PIN daquele usuario
// =============================================================

// "C:3fa1" = inicio do id protegido (nunca o UID real)
String idCurto(String id) {
  return "C:" + id.substring(0, 4);
}

// Ponto de entrada: decide o que o cracha significa na tela atual
void tratarCartaoLido(String id) {
  if (estado == ESTADO_BLOQUEADO) {
    registrarTentativaBloqueio("cracha " + idCurto(id));
    return;
  }
  if (estado == ESTADO_AGUARDAR_CARTAO) {
    cartaoNaTelaDeEspera(id);
    return;
  }
  bool podeEntrar = (estado == ESTADO_OCIOSO || estado == ESTADO_MENU || estado == ESTADO_ESCOLHER_USUARIO ||
                     estado == ESTADO_HISTORICO || estado == ESTADO_STATUS || estado == ESTADO_SOBRE ||
                     estado == ESTADO_ALERTAS);
  if (podeEntrar) entrarComCartao(id);
  // Nas outras telas (digitando senha, area do Admin...) o cracha e ignorado
}

// Cracha aproximado na tela de espera: identifica o usuario (1o fator)
void entrarComCartao(String id) {
  int dono = usuarioBuscarPorCartao(id);   // busca linear no vetor de usuarios
  if (dono < 0) {
    depoisDaFalha(registrarFalha("CARTAO_DESCONHECIDO",
                                 "Cracha nao cadastrado (" + idCurto(id) + ", " + rfidUltimoTipo() + ")", "-"));
    return;
  }
  String nome = usuarioNome(dono);
  if (!usuarioAtivo(dono)) {
    depoisDaFalha(registrarFalha("CARTAO_REVOGADO", "Credencial revogada (" + idCurto(id) + ")", nome));
    return;
  }
  if (!authSenhaCadastrada(dono)) {
    registrarEvento("auth", "SEM_PIN", SEV_AVISO, "Usuario sem PIN: Admin precisa cadastrar", nome);
    feedbackAlerta("Sem PIN");
    mostrarResultado(RESULTADO_MS);
    return;
  }
  usuarioAtual = dono;
  cartaoConfirmado = true;
  voltarParaUsuarios = false;
  registrarEvento("rfid", "CARTAO_IDENTIFICADO", SEV_INFO, "1o fator ok (" + idCurto(id) + ")", nome);
  iniciarSenha(MODO_ENTRAR);               // agora o 2o fator: o PIN
}

// "Entrar" pelo menu, escolhendo o nome
void entrarComUsuario(int indice) {
  usuarioAtual = indice;
  cartaoConfirmado = false;
  voltarParaUsuarios = false;

  if (usuarioTemCartao(indice)) {
    // Quem tem cracha cadastrado SEMPRE precisa dele.
    // Se o leitor nao responde, o acesso e negado ("falha fechada"):
    // desligar o leitor nao pode virar um jeito de pular o cracha.
    if (!rfidPresente()) {
      registrarEvento("rfid", "MFA_INDISPONIVEL", SEV_AVISO, "Usuario exige cracha e o leitor nao responde",
                      usuarioNome(indice));
      feedbackErro("Leitor ausente");
      mostrarResultado(RESULTADO_MS);
      return;
    }
    modoCartao = CARTAO_SEGUNDO_FATOR;
    mudarEstado(ESTADO_AGUARDAR_CARTAO);
    return;
  }
  iniciarSenha(MODO_ENTRAR);               // usuario sem cracha: so PIN
}

// Cracha lido enquanto a tela pedia "Aproxime cracha"
void cartaoNaTelaDeEspera(String id) {
  if (modoCartao == CARTAO_SEGUNDO_FATOR) {
    String nome = usuarioNome(usuarioAtual);
    if (id == usuarioCartao(usuarioAtual)) {
      cartaoConfirmado = true;
      registrarEvento("rfid", "CARTAO_IDENTIFICADO", SEV_INFO, "Cracha confere com o usuario", nome);
      iniciarSenha(MODO_ENTRAR);
    } else {
      depoisDaFalha(registrarFalha("CARTAO_NAO_CONFERE", "Cracha de outra pessoa ou desconhecido", nome));
    }
    return;
  }

  // CARTAO_CADASTRO (area do Admin)
  String nome = usuarioNome(usuarioAlvo);
  if (usuarioAssociarCartao(usuarioAlvo, id)) {
    registrarEvento("admin", "CARTAO_CADASTRADO", SEV_INFO,
                    "Cracha " + idCurto(id) + " (" + rfidUltimoTipo() + ", UID de " +
                    String(rfidUltimoTamanhoUid()) + " bytes)", nome);
    displayMostrarComIcone(ICONE_CERTO, "Cracha salvo", "  " + nome);
  } else {
    registrarEvento("admin", "CARTAO_EM_USO", SEV_AVISO, "Cracha ja pertence a outro usuario", nome);
    displayMostrarComIcone(ICONE_ERRO, "Ja tem dono", "  outro usuario");
    feedbackPiscar(3, 150);
  }
  mostrarResultado(RESULTADO_MS);
}

void voltarDaTelaDeCartao() {
  if (modoCartao == CARTAO_CADASTRO) mudarEstado(ESTADO_USUARIO_ACOES);
  else mudarEstado(ESTADO_MENU);
}

void tratarAguardarCartao(EventoEntrada ev) {
  if (ev == EV_OK_LONGO || ev == EV_ESQUERDA) {
    registrarEvento("rfid", "CARTAO_CANCELADO", SEV_INFO, "Leitura do cracha cancelada", "-");
    voltarDaTelaDeCartao();
    return;
  }
  if (millis() - estadoDesde >= CARTAO_TIMEOUT_MS) {
    registrarEvento("rfid", "CARTAO_TIMEOUT", SEV_AVISO, "Nenhum cracha em 15 s", "-");
    voltarDaTelaDeCartao();
  }
}

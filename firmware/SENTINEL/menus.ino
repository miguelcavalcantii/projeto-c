// =============================================================
// SENTINEL - menus.ino (aba)
// O que cada tela faz quando o joystick/botao e usado.
// =============================================================

// Navegacao generica: sobe/desce dando a volta no fim da lista
int moverSelecao(int atual, int total, EventoEntrada ev) {
  if (total <= 0) return 0;
  if (ev == EV_CIMA) return (atual + total - 1) % total;
  if (ev == EV_BAIXO) return (atual + 1) % total;
  return atual;
}

// ---------- Inicio e espera ----------

void tratarInicio() {
  // Barra de progresso de 0 a 100% durante BOOT_MS
  unsigned long passado = millis() - estadoDesde;
  int progresso = passado * 100 / BOOT_MS;
  if (progresso / 10 != ultimoProgresso / 10) {   // so redesenha a cada 10%
    ultimoProgresso = progresso;
    displayBarra(1, progresso);
  }
  if (passado >= BOOT_MS) irParaInicio();
}

void tratarPrimeiroUso(EventoEntrada ev) {
  if (ev == EV_OK) {
    usuarioAlvo = 0;                // a 1a senha e a do Admin
    voltarParaUsuarios = false;
    iniciarSenha(MODO_NOVA);
  }
}

void tratarOcioso(EventoEntrada ev) {
  if (ev == EV_OK) {
    itemSelecionado = 0;
    mudarEstado(ESTADO_MENU);
    return;
  }
  // Atualiza a cada 1 s (o relogio anda) e troca de mensagem periodicamente
  if (millis() - ultimaAtualizacao >= 1000) {
    ultimaAtualizacao = millis();
    paginaOcioso = ((millis() - estadoDesde) / ROTACAO_OCIOSO_MS) % 3;
    desenharOcioso();
  }
}

// ---------- Menu principal ----------

void tratarMenu(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    itemSelecionado = moverSelecao(itemSelecionado, TOTAL_ITENS, ev);
    displayMostrarMenu(itensMenu, TOTAL_ITENS, itemSelecionado);
  } else if (ev == EV_OK) {
    if (itemSelecionado == 0) { montarListaEntrar(); mudarEstado(ESTADO_ESCOLHER_USUARIO); }
    if (itemSelecionado == 1) mudarEstado(ESTADO_HISTORICO);
    if (itemSelecionado == 2) {                  // area do Admin: pede a senha dele
      acaoPendente = ACAO_USUARIOS;
      voltarParaUsuarios = false;
      iniciarSenha(MODO_AUTORIZAR);
    }
    if (itemSelecionado == 3) mudarEstado(ESTADO_AUDITORIA);
    if (itemSelecionado == 4) mudarEstado(ESTADO_STATUS);
    if (itemSelecionado == 5) mudarEstado(ESTADO_ALERTAS);
    if (itemSelecionado == 6) mudarEstado(ESTADO_SOBRE);
    if (itemSelecionado == 7) mudarEstado(ESTADO_OCIOSO);
  } else if (ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_OCIOSO);
  }
}

// ---------- Listas de usuarios ----------

// Para "Entrar": so usuarios ativos e com senha. "+C" = precisa de cracha.
void montarListaEntrar() {
  qtdLista = 0;
  for (int i = 0; i < MAX_USUARIOS; i++) {
    if (usuarioAtivo(i) && authSenhaCadastrada(i)) {
      listaIndices[qtdLista] = i;
      listaNomes[qtdLista] = usuarioNome(i) + (usuarioTemCartao(i) ? " +C" : "");
      qtdLista++;
    }
  }
}

// Para o Admin: todos. P = tem PIN, C = tem cracha, X = revogado.
void montarListaAdmin() {
  qtdLista = 0;
  for (int i = 0; i < MAX_USUARIOS; i++) {
    String marcas = "";
    if (!usuarioAtivo(i)) {
      marcas = "X";
    } else {
      if (authSenhaCadastrada(i)) marcas += "P";
      if (usuarioTemCartao(i)) marcas += "C";
    }
    listaIndices[qtdLista] = i;
    listaNomes[qtdLista] = usuarioNome(i) + " " + marcas;
    qtdLista++;
  }
}

void tratarEscolherUsuario(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    itemLista = moverSelecao(itemLista, qtdLista, ev);
    desenharLista();
  } else if (ev == EV_OK && qtdLista > 0) {
    entrarComUsuario(listaIndices[itemLista]);
  } else if (ev == EV_ESQUERDA || ev == EV_OK_LONGO || ev == EV_OK) {
    mudarEstado(ESTADO_MENU);
  }
}

void tratarUsuarios(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    itemLista = moverSelecao(itemLista, qtdLista, ev);
    desenharLista();
  } else if (ev == EV_OK) {
    usuarioAlvo = listaIndices[itemLista];
    voltarParaUsuarios = true;          // os resultados voltam para esta area
    mudarEstado(ESTADO_USUARIO_ACOES);
  } else if (ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    voltarParaUsuarios = false;         // saiu da area do Admin
    mudarEstado(ESTADO_MENU);
  }
}

void tratarUsuarioAcoes(EventoEntrada ev) {
  String nome = usuarioNome(usuarioAlvo);

  if (ev == EV_CIMA || ev == EV_BAIXO) {
    itemAcao = moverSelecao(itemAcao, TOTAL_ITENS_ACOES, ev);
    desenharAcoes();
    return;
  }
  if (ev == EV_ESQUERDA || ev == EV_OK_LONGO || (ev == EV_OK && itemAcao == 4)) {
    montarListaAdmin();
    mudarEstado(ESTADO_USUARIOS);
    return;
  }
  if (ev != EV_OK) return;

  if (itemAcao == 0) {                       // Definir PIN
    iniciarSenha(MODO_NOVA);
  } else if (itemAcao == 1) {                // Cadastrar cracha
    if (!rfidPresente()) {
      displayMostrarComIcone(ICONE_ALERTA, "RFID ausente", "  ligue o leitor");
      mostrarResultado(RESULTADO_MS);
    } else {
      modoCartao = CARTAO_CADASTRO;
      mudarEstado(ESTADO_AGUARDAR_CARTAO);
    }
  } else if (itemAcao == 2) {                // Tirar cracha
    usuarioRemoverCartao(usuarioAlvo);
    registrarEvento("admin", "CARTAO_REMOVIDO", SEV_AVISO, "Cracha desassociado do usuario", nome);
    displayMostrarComIcone(ICONE_CERTO, "Cracha removido", "  " + nome);
    mostrarResultado(RESULTADO_MS);
  } else if (itemAcao == 3) {                // Revogar / Reativar
    if (usuarioAlvo == 0) {
      displayMostrarComIcone(ICONE_ALERTA, "Admin nao pode", "  ser revogado");
    } else if (usuarioAtivo(usuarioAlvo)) {
      usuarioDefinirAtivo(usuarioAlvo, false);
      registrarEvento("admin", "USUARIO_REVOGADO", SEV_AVISO, "Credenciais revogadas", nome);
      displayMostrarComIcone(ICONE_ERRO, "Revogado", "  " + nome);
    } else {
      usuarioDefinirAtivo(usuarioAlvo, true);
      registrarEvento("admin", "USUARIO_REATIVADO", SEV_INFO, "Credenciais reativadas", nome);
      displayMostrarComIcone(ICONE_CERTO, "Reativado", "  " + nome);
    }
    mostrarResultado(RESULTADO_MS);
  }
}

// ---------- Historico (fila circular) ----------

void tratarHistorico(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    posicaoHistorico = moverSelecao(posicaoHistorico, historicoQuantidade(), ev);
    desenharHistorico();
  } else if (ev == EV_OK || ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_MENU);
  }
}

// ---------- Bloqueio ----------

// Mexer em qualquer coisa durante o bloqueio e suspeito: registra
// (a 1a tentativa e depois a cada 5, para nao encher o log)
void registrarTentativaBloqueio(String origem) {
  unsigned long n = secTentativaDuranteBloqueio();
  if (n == 1 || n % 5 == 0) {
    registrarEvento("ids", "TENTATIVA_DURANTE_BLOQUEIO", SEV_ALERTA,
                    String(n) + " tentativa(s) com o sistema bloqueado (" + origem + ")", "-");
  }
}

void tratarBloqueado(EventoEntrada ev) {
  if (ev != EV_NENHUM) registrarTentativaBloqueio("joystick/botao");
  // Atualiza a contagem regressiva 1 vez por segundo
  // (espera 1,5 s no inicio para o aviso "SIST. BLOQUEADO" aparecer antes)
  if (millis() - estadoDesde >= 1500 && millis() - ultimaAtualizacao >= 1000) {
    ultimaAtualizacao = millis();
    desenharBloqueio();
  }
}

// ---------- Auditoria ----------

void auditoriaVerificar() {
  displayMostrarComIcone(ICONE_RELOGIO, "Verificando...", "  corrente HMAC");
  ResultadoVerificacao r = auditVerificar();
  if (r.integra) {
    registrarEvento("auditoria", "AUDITORIA_OK", SEV_INFO, String(r.totalVerificados) + " registros integros", "-");
    displayMostrarComIcone(ICONE_CERTO, "Log integro", "  " + String(r.totalVerificados) + " reg. ok");
  } else {
    registrarEvento("auditoria", "AUDITORIA_VIOLADA", SEV_CRITICO,
                    "Registro #" + String(r.idComErro) + ": " + r.motivo, "-");
    displayMostrarComIcone(ICONE_ERRO, "LOG VIOLADO!", "  Registro #" + String(r.idComErro));
    feedbackPiscar(8, 250);
  }
  mostrarResultado(RESULTADO_LONGO_MS);
}

void auditoriaSimularAtaque() {
  unsigned long alvo = auditSimularAdulteracao();
  if (alvo == 0) {
    displayMostrarComIcone(ICONE_ALERTA, "Log muito curto", "  gere eventos");
  } else {
    // Registramos que foi uma simulacao de laboratorio (transparencia)
    registrarEvento("lab", "SIMULACAO_ADULTERACAO", SEV_ALERTA,
                    "Registro #" + String(alvo) + " alterado sem recalcular o HMAC", "-");
    displayMostrarComIcone(ICONE_ALERTA, "LAB: invasor", "  alterou o #" + String(alvo));
  }
  mostrarResultado(RESULTADO_LONGO_MS);
}

void tratarAuditoria(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    itemAuditoria = moverSelecao(itemAuditoria, TOTAL_ITENS_AUDITORIA, ev);
    displayMostrarMenu(itensAuditoria, TOTAL_ITENS_AUDITORIA, itemAuditoria);
  } else if (ev == EV_OK) {
    if (itemAuditoria == 0) auditoriaVerificar();
    if (itemAuditoria == 1) auditoriaSimularAtaque();
    if (itemAuditoria == 2) {
      acaoPendente = ACAO_NOVO_LOG;
      voltarParaUsuarios = false;
      iniciarSenha(MODO_AUTORIZAR);
    }
    if (itemAuditoria == 3) mudarEstado(ESTADO_MENU);
  } else if (ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_MENU);
  }
}

// ---------- Status, alertas e sobre ----------

void tratarStatus(EventoEntrada ev) {
  if (ev == EV_CIMA || ev == EV_BAIXO) {
    paginaStatus = moverSelecao(paginaStatus, TOTAL_PAGINAS_STATUS, ev);
    desenharStatus();
  } else if (ev == EV_OK || ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_MENU);
  } else if (millis() - ultimaAtualizacao >= 1000) {
    ultimaAtualizacao = millis();
    desenharStatus();   // atualiza o relogio e a memoria
  }
}

void tratarAlertas(EventoEntrada ev) {
  // Estes eventos sao so para testar os avisos: vao para o painel,
  // mas NAO para a auditoria (nao sao eventos reais de seguranca)
  if (ev == EV_OK) {
    if (alertaAtual == 0) {
      feedbackSucesso("Teste: verde");
      telemetriaEvento("teste", "FEEDBACK_SUCESSO", SEV_INFO, "Teste do aviso de acesso liberado", "-", 0, "");
    }
    if (alertaAtual == 1) {
      feedbackErro("Teste: vermelho");
      telemetriaEvento("teste", "FEEDBACK_ERRO", SEV_AVISO, "Teste do aviso de acesso negado", "-", 0, "");
    }
    if (alertaAtual == 2) {
      feedbackAlerta("Teste: suspeito");
      telemetriaEvento("teste", "FEEDBACK_ALERTA", SEV_ALERTA, "Teste do aviso de atividade suspeita", "-", 0, "");
    }
    if (alertaAtual == 3) {
      feedbackBloqueio("Teste: 30s");
      telemetriaEvento("teste", "FEEDBACK_BLOQUEIO", SEV_CRITICO, "Teste do aviso de bloqueio", "-", 0, "");
    }
    alertaAtual = (alertaAtual + 1) % 4;
  } else if (ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_MENU);
  }
}

void tratarSobre(EventoEntrada ev) {
  if (ev == EV_OK || ev == EV_ESQUERDA || ev == EV_OK_LONGO) {
    mudarEstado(ESTADO_MENU);
  } else if (millis() - ultimaAtualizacao >= ROLAGEM_MS) {
    ultimaAtualizacao = millis();
    posicaoRolagem++;
    desenharSobre();
  }
}

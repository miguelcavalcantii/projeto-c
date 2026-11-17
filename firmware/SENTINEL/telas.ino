// =============================================================
// SENTINEL - telas.ino (aba)
// Desenho das telas no LCD. Aqui so se DESENHA: nenhuma regra de
// seguranca fica neste arquivo.
// =============================================================

// Tela de espera: linha 1 fixa, linha 2 alterna
void desenharOcioso() {
  String linha1 = "  SENTINEL";
  if (telemetriaPainelConectado()) linha1 += "   PC";
  displayLinha(0, linha1);
  displayIcone(0, 0, ICONE_ESCUDO);
  displayIcone(15, 0, ICONE_CADEADO);

  if (paginaOcioso == 0) {
    displayLinha(1, rfidPresente() ? "  Cracha ou OK" : "  Pressione OK");
  } else if (paginaOcioso == 1) {
    displayLinha(1, "  " + formatarTempo(millis() / 1000));
    displayIcone(0, 1, ICONE_RELOGIO);
  } else {
    displayLinha(1, "  Sistema seguro");
    displayIcone(0, 1, ICONE_CERTO);
  }
}

void desenharBloqueio() {
  displayMostrarComIcone(ICONE_CADEADO, "SIST. BLOQUEADO",
                         "  Aguarde " + formatarMinSeg(secSegundosRestantes()));
  displayIcone(0, 1, ICONE_RELOGIO);
}

void desenharAguardarCartao() {
  String linha1 = (modoCartao == CARTAO_CADASTRO) ? "Cadastro cracha" : "Aproxime cracha";
  displayMostrarComIcone(ICONE_CADEADO, linha1, "  " + usuarioNome(modoCartao == CARTAO_CADASTRO ? usuarioAlvo : usuarioAtual));
}

// Lista de usuarios (escolher para entrar, ou a lista do Admin)
void desenharLista() {
  if (qtdLista == 0) {
    displayMostrar("Ninguem com PIN", "Admin: Usuarios");
    return;
  }
  displayMostrarMenu(listaNomes, qtdLista, itemLista);
}

void desenharAcoes() {
  // O item 3 muda conforme a situacao do usuario
  itensAcoes[3] = usuarioAtivo(usuarioAlvo) ? "Revogar" : "Reativar";
  displayMostrarMenu(itensAcoes, TOTAL_ITENS_ACOES, itemAcao);
}

// Historico: "ACESSO_LIBE 1/10" e "Miguel 00:12:30"
void desenharHistorico() {
  int total = historicoQuantidade();
  if (total == 0) {
    displayMostrar("Historico", "  (vazio)");
    return;
  }
  ItemHistorico item = historicoPegar(posicaoHistorico);
  String contador = String(posicaoHistorico + 1) + "/" + String(total);
  String linha1 = item.tipo;
  int espaco = LCD_COLS - contador.length() - 1;
  if ((int)linha1.length() > espaco) linha1 = linha1.substring(0, espaco);
  while ((int)linha1.length() < espaco) linha1 += " ";
  linha1 += " " + contador;

  String usuario = item.usuario;
  if (usuario.length() > 7) usuario = usuario.substring(0, 7);
  while (usuario.length() < 7) usuario += " ";
  displayMostrar(linha1, usuario + " " + formatarTempo(item.uptime));
}

int contarUsuariosComSenha() {
  int n = 0;
  for (int i = 0; i < MAX_USUARIOS; i++) {
    if (authSenhaCadastrada(i)) n++;
  }
  return n;
}

void desenharStatus() {
  String titulo = "Status";
  String pagina = String(paginaStatus + 1) + "/" + String(TOTAL_PAGINAS_STATUS);
  while (titulo.length() < LCD_COLS - pagina.length()) titulo += " ";
  titulo += pagina;

  String linha2;
  if (paginaStatus == 0) linha2 = "Ligado " + formatarTempo(millis() / 1000);
  if (paginaStatus == 1) linha2 = "Memoria " + String(ESP.getFreeHeap() / 1024) + " KB";
  if (paginaStatus == 2) linha2 = "Usuarios: " + String(contarUsuariosComSenha()) + "/" + String(MAX_USUARIOS);
  if (paginaStatus == 3) linha2 = rfidPresente() ? "RFID: ok 0x" + String(rfidVersao(), HEX) : "RFID: ausente";
  if (paginaStatus == 4) linha2 = "F:" + String(secFalhas()) + " Bloqueios:" + String(secTotalBloqueios());
  if (paginaStatus == 5) linha2 = "Log: " + String(auditTotalRegistros()) + " reg";
  if (paginaStatus == 6) linha2 = "PBKDF2 " + String(authUltimoTempoMs()) + " ms";
  if (paginaStatus == 7) linha2 = telemetriaPainelConectado() ? "Painel PC: on" : "Painel PC: off";
  displayMostrar(titulo, linha2);
}

// Texto correndo da direita para a esquerda (efeito "letreiro")
void desenharSobre() {
  String texto = "                " + TEXTO_SOBRE + "  ";
  if (posicaoRolagem > (int)texto.length() - LCD_COLS) posicaoRolagem = 0;
  displayLinha(1, texto.substring(posicaoRolagem, posicaoRolagem + LCD_COLS));
}

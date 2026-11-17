#include <Preferences.h>
#include "config.h"
#include "security_config.h"
#include "user_manager.h"

static Preferences nvs;
static Usuario usuarios[MAX_USUARIOS];   // o vetor de structs

static const char* NOMES[MAX_USUARIOS] = {NOME_USUARIO_0, NOME_USUARIO_1, NOME_USUARIO_2, NOME_USUARIO_3};

static bool indiceValido(int i) {
  return i >= 0 && i < MAX_USUARIOS;
}

// Grava UM usuario na NVS (chaves: "ativo1", "cart1", "aces1"...)
static void salvarUsuario(int i) {
  nvs.begin("sentinel_usr", false);
  nvs.putBool(("ativo" + String(i)).c_str(), usuarios[i].ativo);
  nvs.putString(("cart" + String(i)).c_str(), usuarios[i].cartao);
  nvs.putULong(("aces" + String(i)).c_str(), usuarios[i].acessos);
  nvs.end();
}

void usuariosIniciar() {
  nvs.begin("sentinel_usr", true);
  for (int i = 0; i < MAX_USUARIOS; i++) {
    usuarios[i].nome = NOMES[i];
    usuarios[i].ativo = nvs.getBool(("ativo" + String(i)).c_str(), true);
    usuarios[i].cartao = nvs.getString(("cart" + String(i)).c_str(), "");
    usuarios[i].acessos = nvs.getULong(("aces" + String(i)).c_str(), 0);
  }
  nvs.end();
  usuarios[0].ativo = true;   // o Admin nunca fica revogado (evita trancar o sistema)
}

String usuarioNome(int i)          { return indiceValido(i) ? usuarios[i].nome : "?"; }
bool usuarioAtivo(int i)           { return indiceValido(i) && usuarios[i].ativo; }
bool usuarioTemCartao(int i)       { return indiceValido(i) && usuarios[i].cartao.length() > 0; }
String usuarioCartao(int i)        { return indiceValido(i) ? usuarios[i].cartao : ""; }
unsigned long usuarioAcessos(int i){ return indiceValido(i) ? usuarios[i].acessos : 0; }

int usuarioBuscarPorCartao(String idCartao) {
  if (idCartao.length() == 0) return -1;
  // Busca linear: olha posicao por posicao ate achar
  for (int i = 0; i < MAX_USUARIOS; i++) {
    if (usuarios[i].cartao == idCartao) return i;
  }
  return -1;
}

bool usuarioAssociarCartao(int i, String idCartao) {
  if (!indiceValido(i)) return false;
  int dono = usuarioBuscarPorCartao(idCartao);
  if (dono >= 0 && dono != i) return false;   // um cartao so pode ter um dono
  usuarios[i].cartao = idCartao;
  salvarUsuario(i);
  return true;
}

void usuarioRemoverCartao(int i) {
  if (!indiceValido(i)) return;
  usuarios[i].cartao = "";
  salvarUsuario(i);
}

void usuarioDefinirAtivo(int i, bool ativo) {
  if (!indiceValido(i) || i == 0) return;     // o Admin nao pode ser revogado
  usuarios[i].ativo = ativo;
  salvarUsuario(i);
}

void usuarioContarAcesso(int i) {
  if (!indiceValido(i)) return;
  usuarios[i].acessos++;
  salvarUsuario(i);
}

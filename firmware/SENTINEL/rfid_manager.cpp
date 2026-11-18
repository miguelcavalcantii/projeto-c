#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include "config.h"
#include "security_config.h"
#include "crypto_manager.h"
#include "rfid_manager.h"

#define INTERVALO_LEITURA_MS  150    // procura cartao ~6 vezes por segundo
#define ESPERA_MESMO_CARTAO   1500   // ignora leituras repetidas por 1,5 s

static MFRC522 leitor(PIN_RFID_SS, PIN_RFID_RST);
static Preferences nvs;
static byte chave[32];                 // chave para proteger o UID (fica na NVS)

static bool presente = false;
static byte versao = 0;
static String ultimoId = "";
static String ultimoTipo = "";
static int ultimoTamanhoUid = 0;
static unsigned long ultimaProcura = 0;
static unsigned long ultimaLeitura = 0;
static unsigned long ultimoTeste = 0;
static bool avisoPerdeu = false;
static bool avisoVoltou = false;

// 0x00 e 0xFF significam "ninguem respondeu" no barramento SPI
static bool versaoValida(byte v) {
  return v != 0x00 && v != 0xFF;
}

static byte lerVersao() {
  return leitor.PCD_ReadRegister(MFRC522::VersionReg);
}

bool rfidIniciar() {
  SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
  leitor.PCD_Init();
  // Ganho maximo da antena: aumenta o alcance (modulos clone costumam
  // vir com ganho baixo e so leem com o cartao praticamente encostado)
  leitor.PCD_SetAntennaGain(MFRC522::RxGain_max);
  versao = lerVersao();
  presente = versaoValida(versao);

  // Chave criada uma unica vez com o gerador aleatorio do hardware
  nvs.begin("sentinel_rf", false);
  if (nvs.getBytesLength("chave") != 32) {
    cryptoGerarAleatorio(chave, 32);
    nvs.putBytes("chave", chave, 32);
  } else {
    nvs.getBytes("chave", chave, 32);
  }
  nvs.end();

  ultimoTeste = millis();
  return presente;
}

void rfidAtualizar() {
  // Autoteste: o leitor ainda esta respondendo?
  if (millis() - ultimoTeste < RFID_TESTE_MS) return;
  ultimoTeste = millis();

  byte v = lerVersao();
  if (presente && !versaoValida(v)) {
    presente = false;          // parou de responder (fio solto, sem energia...)
    avisoPerdeu = true;
  } else if (!presente) {
    leitor.PCD_Init();         // tenta de novo: talvez tenha acabado de ser plugado
    leitor.PCD_SetAntennaGain(MFRC522::RxGain_max);
    v = lerVersao();
    if (versaoValida(v)) {
      presente = true;
      versao = v;
      avisoVoltou = true;
    }
  }
}

bool rfidCartaoDetectado() {
  if (!presente) return false;
  if (millis() - ultimaProcura < INTERVALO_LEITURA_MS) return false;
  ultimaProcura = millis();
  if (millis() - ultimaLeitura < ESPERA_MESMO_CARTAO) return false;

  if (!leitor.PICC_IsNewCardPresent()) return false;
  if (!leitor.PICC_ReadCardSerial()) return false;

  // Protege o UID: so o HMAC sai daqui (o UID puro nao e guardado em lugar nenhum)
  byte resultado[HASH_BYTES];
  cryptoHmacSha256(chave, 32, leitor.uid.uidByte, leitor.uid.size, resultado);
  ultimoId = cryptoParaHex(resultado, 8);      // 8 bytes = 16 caracteres
  ultimoTamanhoUid = leitor.uid.size;
  ultimoTipo = String(leitor.PICC_GetTypeName(leitor.PICC_GetType(leitor.uid.sak)));

  leitor.PICC_HaltA();         // "adormece" o cartao para nao ler de novo sem parar
  leitor.PCD_StopCrypto1();
  ultimaLeitura = millis();
  return true;
}

bool rfidPresente()        { return presente; }
byte rfidVersao()          { return versao; }
String rfidUltimoId()      { return ultimoId; }
String rfidUltimoTipo()    { return ultimoTipo; }
int rfidUltimoTamanhoUid() { return ultimoTamanhoUid; }

bool rfidPerdeuConexao() {
  bool a = avisoPerdeu;
  avisoPerdeu = false;
  return a;
}

bool rfidReconectou() {
  bool a = avisoVoltou;
  avisoVoltou = false;
  return a;
}

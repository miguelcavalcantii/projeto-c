// TESTE 08 - Integrado (todos os componentes juntos)
// So rode depois que os testes 01 a 07 passarem individualmente.
// Esperado:
//   - LCD mostra "Aguardando cartao".
//   - Joystick para cima/baixo muda o digito 0-9 na linha de baixo.
//   - Botao: pisca o LED verde e mostra o digito escolhido no Serial.
//   - Cartao aproximado: LED vermelho pisca e o LCD mostra "Cartao lido".
// Objetivo: confirmar que SPI (RFID) e I2C (LCD) funcionam AO MESMO TEMPO, sem conflito de pinos.

#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>

#define PIN_SS 27
#define PIN_RST 26
#define PIN_JOY_Y 35
#define PIN_BUTTON 25
#define PIN_LED_GREEN 32
#define PIN_LED_RED 33
#define LCD_ADDR 0x27   // usar o endereco do teste 05

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
MFRC522 rfid(PIN_SS, PIN_RST);

int digito = 0;
unsigned long ultimoMovimento = 0;
int ultimoBotao = HIGH;
unsigned long ledDesligaEm = 0;

void mostrarDigito() {
  lcd.setCursor(0, 1);
  lcd.print("Digito: ");
  lcd.print(digito);
  lcd.print("       ");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  SPI.begin(18, 19, 23, PIN_SS);
  rfid.PCD_Init();

  lcd.setCursor(0, 0);
  lcd.print("Aguardando cartao");
  mostrarDigito();
  Serial.println("=== TESTE 08 - Integrado ===");
}

void loop() {
  // --- Joystick: muda o digito no maximo a cada 250 ms ---
  int y = analogRead(PIN_JOY_Y);
  if (millis() - ultimoMovimento >= 250) {
    if (y < 1000) { digito = (digito + 1) % 10; mostrarDigito(); ultimoMovimento = millis(); }
    else if (y > 3000) { digito = (digito + 9) % 10; mostrarDigito(); ultimoMovimento = millis(); }
  }

  // --- Botao: detecta o momento do aperto (HIGH -> LOW) ---
  int botao = digitalRead(PIN_BUTTON);
  if (botao == LOW && ultimoBotao == HIGH) {
    Serial.print("Confirmado digito ");
    Serial.println(digito);
    digitalWrite(PIN_LED_GREEN, HIGH);
    ledDesligaEm = millis() + 300;
    delay(30); // debounce simples, suficiente para este teste
  }
  ultimoBotao = botao;

  // --- RFID ---
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    lcd.setCursor(0, 0);
    lcd.print("Cartao lido!     ");
    Serial.println("Cartao detectado");
    digitalWrite(PIN_LED_RED, HIGH);
    ledDesligaEm = millis() + 300;
    rfid.PICC_HaltA();
  }

  // --- Apaga os LEDs depois de 300 ms ---
  if (ledDesligaEm != 0 && millis() > ledDesligaEm) {
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    ledDesligaEm = 0;
  }
}

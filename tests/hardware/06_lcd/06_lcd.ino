// TESTE 06 - LCD 16x2 I2C
// Biblioteca: "LiquidCrystal I2C" (Frank de Brabander) - instalar pelo Gerenciador de Bibliotecas.
// Antes: rode o teste 05 e coloque o endereco encontrado abaixo.
// Se a tela acender mas nao mostrar texto: gire o potenciometro azul atras do modulo (contraste).
// Obs.: o LCD nao tem acentos. Escreva sem acento.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27   // TROQUE se o scanner mostrar outro (ex.: 0x3F)

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
unsigned long ultimo = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SENTINEL v0.1");
  lcd.setCursor(0, 1);
  lcd.print("Teste do LCD");
  Serial.println("=== TESTE 06 - LCD ===");
}

void loop() {
  // Atualiza um contador na segunda linha a cada 1 s
  if (millis() - ultimo >= 1000) {
    ultimo = millis();
    lcd.setCursor(0, 1);
    lcd.print("Tempo: ");
    lcd.print(millis() / 1000);
    lcd.print(" s     "); // espacos apagam restos do texto anterior
  }
}

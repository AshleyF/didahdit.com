#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

const int buttonPin = 2;
const int buzzerPin = 3;

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Morse Decoder");
  lcd.setCursor(0, 1);
  lcd.print("didahdit.com");
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  pinMode(buzzerPin, OUTPUT);
}

long ms = 0;

void loop() {
  int button = digitalRead(buttonPin);
  if (button == LOW) {
    tone(buzzerPin, 600, 10);
    if (ms == 0) ms = millis(); // mark start
  } else {
    if (ms != 0) {
      long len = millis() - ms;
      if (len > 0) update(len);
      ms = 0;
    }
  }
}

long last = -10000;
int cur0 = 0;
int cur1 = 0;

char* morse = "  ETIANMSURWDKGOHVF?L?PJBXCYZQ";
int p = 0;

void update(long ms) {
  long m = millis();
  long len = m - last;
  last = m;
  if (len > 600) { // letter break
    lcd.setCursor(0, 1);
    lcd.print("                ");
    cur0++;
    cur1 = 0;
    p = 1;
  }
  if (len > 5000) { // new message
    lcd.clear();
    cur0 = 0;
  }
  lcd.setCursor(cur1++, 1);
  p = p * 2; // dit doubles, dah doubles + 1
  if (ms > 200) {
    p++;
    lcd.print('-');
  } else {
    lcd.print('.');
  }
  lcd.setCursor(cur0, 0);
  if (p > 30) {
    lcd.print('?');
  } else {
    lcd.print(morse[p]);
  }
}

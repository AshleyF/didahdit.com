#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

const bool MODE_B = true;

const int PADDLE_LEFT = 0;
const int PADDLE_RIGHT = 1;
const int STRAIGHT_KEY = 2;
const int BUZZER = 3;

const int FREQ = 600;
const int WPM = 15;
const int DIT = 60000 / (WPM * 50);
const int DAH = DIT * 3;
const int PAUSE = DIT;
const int FARNSWORTH = 3;
const int NEW_MESSAGE = 4000;
const int DEBOUNCE = DIT / 4;

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Morse Decoder");
  lcd.setCursor(0, 1);
  lcd.print("didahdit.com");
  pinMode(STRAIGHT_KEY, INPUT);
  digitalWrite(STRAIGHT_KEY, HIGH);
  pinMode(BUZZER, OUTPUT);
}

enum State {
  Waiting,
  Key,
  Left,
  Right,
  LeftRight,
  RightLeft,
} state = Waiting;

long debounceTimeout = 0;

void setDebounceTimeout() {
  if (debounceTimeout == 0) {
    debounceTimeout = millis() + DEBOUNCE;
  }
}

bool oppositeLast = false; // true=Dah, false=Dit

bool lastOpenEndedTone = false;
long toneStart = 0;
long toneUntil = 0;
long quietUntil = 0;
long predictDah = 0;
long repeat = 0;
bool lastSqueeze = false;

long quietStart = -NEW_MESSAGE; // cause initial LCD clear
void playTone(bool dah, bool openEnded) {
  long len = dah ? DAH : DIT;
  lastOpenEndedTone = openEnded;
  if (!openEnded) {
    decode(dah);
    quietUntil = millis() + len + DIT;
  }
  oppositeLast = !dah;
  tone(BUZZER, FREQ, len);
  toneStart = millis();
  toneUntil = toneStart + len;
  quietStart = toneUntil;
}

void stopTone() {
  noTone(BUZZER);
  quietStart = millis();
}

void loop() {
  long ms = millis();
  bool key = digitalRead(STRAIGHT_KEY) == LOW;
  bool left = digitalRead(PADDLE_LEFT) == LOW;
  bool right = digitalRead(PADDLE_RIGHT) == LOW;
  switch (state) {
    case Waiting:
      debounceTimeout = 0;
      if (toneUntil != 0 && ms > toneUntil) {
        stopTone();
        if (lastOpenEndedTone) {
          decode(ms - toneStart >= DAH);
        }
        toneUntil = 0;
        predictDah = 0;
        if (quietUntil == 0) {
          quietUntil = ms + PAUSE;
        }
      }
      if (MODE_B && lastSqueeze && ms > quietUntil) {
        lastSqueeze = false;
        toneUntil = ms + (oppositeLast ? DAH : DIT);
        playTone(oppositeLast, false);
      }
      if (ms > quietUntil) {
        if (key) {
          pause(ms - quietStart);
          state = Key;
          break;
        }
        if (left) {
          pause(ms - quietStart);
          state = Left;
          break;
        }
        if (right) {
          pause(ms - quietStart);
          state = Right;
          break;
        }
      }
      break;
    case Key:
      setDebounceTimeout();
      if (!key && ms > debounceTimeout) {
        state = Waiting;
        break;
      }
      if (toneUntil == 0) {
        playTone(true, true); // maximum, open-ended
        quietUntil = 0;
        toneUntil = ms + DIT; // minimum
        predictDah = ms + DAH / 2;
        repeat = ms + DAH + PAUSE;
      }
      if (predictDah != 0 && ms > predictDah) {
        toneUntil += DIT * 2;
        predictDah = 0;
      }
      if (ms > repeat) {
        if (lastOpenEndedTone) {
          decode(true); // must have been a dah to repeat
        }
        playTone(true, false);
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      break;
    case Left:
      setDebounceTimeout();
      if (!left && ms > debounceTimeout) {
        state = Waiting;
        break;
      }
      if (right) {
        state = LeftRight;
        break;
      }
      if (toneUntil == 0) {
        playTone(false, false);
        quietUntil = 0;
        toneUntil = ms + DIT;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        playTone(false, false);
        lastSqueeze = false;
        toneUntil = ms + DIT;
        repeat = toneUntil + PAUSE;
      }
      break;
    case Right:
      setDebounceTimeout();
      if (!right && ms > debounceTimeout) {
        state = Waiting;
        break;
      }
      if (left) {
        state = RightLeft;
        break;
      }
      if (toneUntil == 0) {
        playTone(true, false);
        quietUntil = 0;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        playTone(true, false);
        lastSqueeze = false;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      break;
    case LeftRight:
      setDebounceTimeout();
      if (!right && ms > debounceTimeout) {
        state = Left;
        break;
      }
      lastSqueeze = true;
      if (toneUntil == 0) {
        playTone(true, false);
        quietUntil = 0;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        long send = oppositeLast ? DAH : DIT;
        playTone(oppositeLast, false);
        toneUntil = ms + send;
        repeat = toneUntil + PAUSE;
      }
    case RightLeft:
      setDebounceTimeout();
      if (!left && ms > debounceTimeout) {
        state = Right;
        break;
      }
      lastSqueeze = true;
      if (toneUntil == 0) {
        playTone(false, false);
        quietUntil = 0;
        toneUntil = ms + DIT;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        long send = oppositeLast ? DAH : DIT;
        playTone(oppositeLast, false);
        toneUntil = ms + send;
        repeat = toneUntil + PAUSE;
      }
      break;
  }
}

int cur0 = 0;
int cur1 = 0;

char* morse = "  ETIANMSURWDKGOHVF?L?PJBXCYZQ??54?3???2??+????16=/?????7???8?90";
int p = 0;
bool backspace = false;

void decode(bool dah) {
  lcd.setCursor(cur1++, 1);
  p = p * 2; // dit doubles, dah doubles + 1
  if (dah) {
    p++;
    lcd.print('-');
  } else {
    lcd.print('.');
  }
  lcd.setCursor(cur0, 0);
  if (p >= 64) {
    backspace = (p >= 256); // ........
    lcd.print(backspace ? ' ' : '?');
    if (backspace && cur0 > 0) {
      lcd.setCursor(--cur0, 0);
      lcd.print(' ');
    }
  } else {
    lcd.print(morse[p]);
  }
}

void pause(long len) {
  if (cur0 >= 15) cur0 = -1; // wrap
  if (len > DIT * FARNSWORTH) { // letter break
    lcd.setCursor(0, 1);
    lcd.print("                ");
    if (!backspace) cur0++;      
    cur1 = 0;
    p = 1;
    backspace = false;
  }
  if (len > 7 * DIT * FARNSWORTH && cur0 != 0) { 
    lcd.setCursor(++cur0, 0);
    lcd.print(' ');
  }
  if (len > NEW_MESSAGE) { // new message
    lcd.clear();
    cur0 = 0;
  }
}
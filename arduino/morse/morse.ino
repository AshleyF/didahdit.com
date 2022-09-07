#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

bool MODE_B = true;
bool TONE = false;

const int PADDLE_LEFT = 8;
const int PADDLE_RIGHT = 9;
const int STRAIGHT_KEY = 2;
const int RADIO = 3;
const int BUZZER = 13;

const int FREQ = 600;
const int WPM = 20;
const int DIT = 60000 / (WPM * 50);
const int DAH = DIT * 3;
const int LETTER = DIT * 3;
const int WORD = DIT * 7;
const int PAUSE = DIT;
const int SEND_FARNSWORTH = 3;
const int COPY_FARNSWORTH = 3;
const int DEBOUNCE = DIT / 4;
const int SCROLL = DIT * 100;

const int DIT_CHAR = 0;
const int DAH_CHAR = 1;
const int SPACE_CHAR = 2;
const int BACKSPACE_CHAR = 3;
const int PROSIGN_CHAR = 4;
const int UNKNOWN_CHAR = 5;
const int LEFT_BRACKET_CHAR = 6;
const int RIGHT_BRACKET_CHAR = 7;

const int MAX_MESSAGE = 16;
char message[MAX_MESSAGE];
int messageLen = 0;

const int MAX_OFFSCREEN = 1000;
char offscreen[MAX_OFFSCREEN];
int offscreenLen = 0;

const int MAX_CODE = 15;
char code[MAX_CODE];
int codeLen = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Morse Decoder v1");
  lcd.setCursor(0, 1);
  lcd.print("www.didahdit.com");
  pinMode(STRAIGHT_KEY, INPUT);
  digitalWrite(STRAIGHT_KEY, HIGH);
  pinMode(PADDLE_LEFT, INPUT);
  digitalWrite(PADDLE_LEFT, HIGH);
  pinMode(PADDLE_RIGHT, INPUT);
  digitalWrite(PADDLE_RIGHT, HIGH);
  pinMode(RADIO, OUTPUT);

  byte ditChar[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b00100,
    0b00000,
    0b00000,
    0b00000,
    0b00000
  };

  byte dahChar[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b00000,
    0b00000,
    0b00000,
    0b00000
  };

  byte spaceChar[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b10001,
    0b11111
  };

  byte backspaceChar[8] = {
    0b00011,
    0b00101,
    0b01001,
    0b10001,
    0b10001,
    0b01001,
    0b00101,
    0b00011
  };

  byte prosignChar[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b10001,
    0b01010,
    0b00000,
    0b00000,
    0b00000
  };

  byte unknownChar[8] = {
    0b11111,
    0b10001,
    0b00000,
    0b10001,
    0b10001,
    0b00000,
    0b10001,
    0b11111,
  };
  
  byte leftBracketChar[8] = {
    0b00000,
    0b00000,
    0b00001,
    0b00010,
    0b00001,
    0b00000,
    0b00000,
    0b00000,
  };
  
  byte rightBracketChar[8] = {
    0b00000,
    0b00000,
    0b10000,
    0b01000,
    0b10000,
    0b00000,
    0b00000,
    0b00000,
  };
  
  lcd.createChar(DIT_CHAR, ditChar);
  lcd.createChar(DAH_CHAR, dahChar);
  lcd.createChar(SPACE_CHAR, spaceChar);
  lcd.createChar(BACKSPACE_CHAR, backspaceChar);
  lcd.createChar(PROSIGN_CHAR, prosignChar);
  lcd.createChar(UNKNOWN_CHAR, unknownChar);
  lcd.createChar(LEFT_BRACKET_CHAR, leftBracketChar);
  lcd.createChar(RIGHT_BRACKET_CHAR, rightBracketChar);
}

enum State {
  Waiting,
  Protocol,
  Key,
  Left,
  Right,
  LeftRight,
  RightLeft,
} state = Waiting, lastState = Waiting;

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

long quietStart = 0;
void playTone(bool dah, bool openEnded, bool copy) {
  long len = dah ? DAH : DIT;
  lastOpenEndedTone = openEnded;
  if (!openEnded && copy) {
    decode(dah);
    quietUntil = millis() + len + DIT;
  }
  oppositeLast = !dah;
  if (TONE) {
    tone(BUZZER, FREQ, len);
  }
  digitalWrite(RADIO, HIGH);
  toneStart = millis();
  toneUntil = toneStart + len;
  quietStart = toneUntil;
}

void stopTone() {
  if (TONE) {
    noTone(BUZZER);
  }
  digitalWrite(RADIO, LOW);
  quietStart = millis();
}

void serialDit() {
  Serial.write('.');
}

void serialDah() {
  Serial.write('-');
}

void serialKeyUpdate(bool leftDown, bool rightDown, bool straightDown) {
  Serial.write((byte)(
    (leftDown     ? 0b010 : 0b000) |
    (rightDown    ? 0b001 : 0b000) |
    (straightDown ? 0b100 : 0b000)));
}

void loop() {
  long ms = millis();
  bool key = digitalRead(STRAIGHT_KEY) == LOW;
  bool left = digitalRead(PADDLE_LEFT) == LOW;
  bool right = digitalRead(PADDLE_RIGHT) == LOW;
  if (state != lastState) {
    switch (state) {
      case Left:
        serialKeyUpdate(true, false, false);
        break;
      case Right:
        serialKeyUpdate(false, true, false);
        break;
      case LeftRight:
      case RightLeft:
        serialKeyUpdate(true, true, false);
        break;
      case Key:
        serialKeyUpdate(false, false, true);
        break;
      default:
        serialKeyUpdate(false, false, false);
        break;
    }
    lastState = state;
  }
  if (state != Key && toneUntil != 0 && ms > toneUntil) stopTone();
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
        playTone(oppositeLast, false, true);
      }
      if (ms > quietUntil) {
        if (ms > toneUntil && Serial.available() > 0) {
          state = Protocol;
          break;
        }
        if (key) {
          state = Key;
          break;
        }
        if (left) {
          state = Left;
          break;
        }
        if (right) {
          state = Right;
          break;
        }

        long time = ms - quietStart;
        pause(time);
        if (time > SCROLL) {
          int len = offscreenLen + messageLen;
          if (len > 15) {
            int scroll = ((time - SCROLL) / 350) % len;
            lcd.setCursor(0, 0);
            for (int i = 0; i < 16; i++) {
              int j = scroll + i;
              lcd.write(offscreen[j % len]);
            }
          }
        }
      }
      break;
    case Protocol:
      switch (Serial.read()) {
        case '.':
          playTone(false, false, false);
          quietUntil = 0;
          toneUntil = ms + DIT;
          break;
        case '-':
          playTone(true, false, false);
          quietUntil = 0;
          toneUntil = ms + DAH;
          break;
        case ' ':
          quietUntil = ms + LETTER * SEND_FARNSWORTH;
          break;
        case '/':
          quietUntil = ms + WORD * SEND_FARNSWORTH;
          break;
      }
      state = Waiting;
      break;
    case Key:
      setDebounceTimeout();
      if (!key && ms > debounceTimeout) {
        state = Waiting;
        break;
      }
      if (toneUntil == 0) {
        playTone(true, true, true); // maximum, open-ended
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
        playTone(true, false, true);
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
        playTone(false, false, true);
        quietUntil = 0;
        toneUntil = ms + DIT;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        playTone(false, false, true);
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
        playTone(true, false, true);
        quietUntil = 0;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        playTone(true, false, true);
        lastSqueeze = false;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      break;
    case LeftRight:
      setDebounceTimeout();
      if (ms > debounceTimeout) {
        if (!left && right) {
          state = Right;
          break;
        }
        if (left && !right) {
          state = Left;
          break;
        }
        if (!left && !right) {
          state = Waiting;
          break;
        }
      }
      lastSqueeze = true;
      if (toneUntil == 0) {
        playTone(true, false, true);
        quietUntil = 0;
        toneUntil = ms + DAH;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        long send = oppositeLast ? DAH : DIT;
        playTone(oppositeLast, false, true);
        toneUntil = ms + send;
        repeat = toneUntil + PAUSE;
      }
    case RightLeft:
      setDebounceTimeout();
      if (ms > debounceTimeout) {
        if (!left && right) {
          state = Right;
          break;
        }
        if (left && !right) {
          state = Left;
          break;
        }
        if (!left && !right) {
          state = Waiting;
          break;
        }
      }
      lastSqueeze = true;
      if (toneUntil == 0) {
        playTone(false, false, true);
        quietUntil = 0;
        toneUntil = ms + DIT;
        repeat = toneUntil + PAUSE;
      }
      if (ms > repeat) {
        long send = oppositeLast ? DAH : DIT;
        playTone(oppositeLast, false, true);
        toneUntil = ms + send;
        repeat = toneUntil + PAUSE;
      }
      break;
  }
}

String morse = "  ETIANMSURWDKGOHVF L PJBXCYZQ  54 3   2& +    16=/   ( 7   8 90     $      ?_    \"  .    @   '  -        ;! )     ,    :";
int p = 1;

bool dirty = true;

void updateLcd() {
  if (dirty) lcd.clear();
  lcd.setCursor(0, 0);
  for (int m = 0; m < messageLen && m < 16; m++) {
    lcd.write(message[m]);
  }
  if (p == 1) lcd.write((byte)SPACE_CHAR);
  lcd.setCursor(0, 1);
  for (int c = 0; c < codeLen && c < 16; c++) {
    lcd.write(code[c]);
  }
}

void appendCode(char c) {
  if (codeLen < MAX_CODE) {
    code[codeLen++] = c;
    updateLcd();
  }
}

void scrollLeftIfNeeded() {
  for (int j = 0; j < messageLen; j++) {
    offscreen[offscreenLen + j] = message[j];
  }
  if (messageLen == MAX_MESSAGE) {
    offscreenLen++;
    if (offscreenLen >= MAX_OFFSCREEN) {
      offscreenLen = 0;
    }
    messageLen--;
    for (int i = 0; i < MAX_MESSAGE - 1; i++) {
      message[i] = message[i + 1];
    }
    dirty = true;
  }
}

String prosign = "";

void setProsign(String sign) {
  if (messageLen > 0) {
    prosign = sign;
    message[messageLen - 1] = (byte)PROSIGN_CHAR;
    updateLcd();
  }
}

bool recentBackspace = false;

void decode(bool dah) {
  p = p * 2; // dit doubles, dah doubles + 1
  if (dah) {
    p++;
    appendCode(DAH_CHAR);
    serialDah();
  } else {
    appendCode(DIT_CHAR);
    serialDit();
  }
  switch (p) {
    case 31: // backspace
      if (messageLen > 0) {
        message[messageLen - 1] = (byte)BACKSPACE_CHAR;
        updateLcd();
      }
      break;
    case 21:
      setProsign("AA");
      break;
    case 34:
      setProsign("VE");
      break;
    case 37:
      setProsign("INT");
      break;
    case 42:
      setProsign("AR");
      break;
    case 53:
      setProsign("CT");
      break;
    case 69:
      setProsign("SK");
      break;
    case 103:
      setProsign("NJ");
      break;
    case 568:
      setProsign("SOS");
      break;
    case 256: // correction (clear)
      messageLen = 0;
      offscreenLen = 0;
      codeLen = 0;
      dirty = true;
      updateLcd();
      break;
    default:
      if (p >= 121) {
        if (messageLen > 0) {
          message[messageLen - 1] = (byte)UNKNOWN_CHAR;
          updateLcd();
        }
      } else {
        recentBackspace = false;
        char c = morse[p];
        if (p < 4) messageLen++; // first dit/dah
        scrollLeftIfNeeded();
        message[messageLen - 1] = c == ' ' ? (byte)UNKNOWN_CHAR : c;
        updateLcd();
      }
      break;
  }
}

void pause(long len) {
  if (len > DIT * COPY_FARNSWORTH && p != 1) { // letter break
    Serial.write(' ');
    if (messageLen > 0) {
      switch (message[messageLen - 1]) {
        case BACKSPACE_CHAR: // backspace
          messageLen = messageLen > 1 ? messageLen - 2 : 0;
          if (message[messageLen] == RIGHT_BRACKET_CHAR) { // delete whole prosign
            while (message[messageLen--] != LEFT_BRACKET_CHAR) {}
            messageLen++;
          }
          dirty = true;
          recentBackspace = true;
          updateLcd();
          break;
        case PROSIGN_CHAR: // expand prosign
          messageLen--;
          message[messageLen++] = (byte)LEFT_BRACKET_CHAR;
          scrollLeftIfNeeded();
          for (int i = 0; i < prosign.length(); i++) {
            message[messageLen++] = prosign.charAt(i);
            scrollLeftIfNeeded();
          }
          message[messageLen++] = (byte)RIGHT_BRACKET_CHAR;
          scrollLeftIfNeeded();
          break;
      }
    }
    codeLen = 0;
    p = 1;
    dirty = true;
    updateLcd();
  }
  if (((!recentBackspace && len > WORD * COPY_FARNSWORTH) || (recentBackspace && len > 3 * WORD * COPY_FARNSWORTH)) && messageLen != 0 && message[messageLen - 1] != ' ') { // word break
    Serial.write("/");
    message[messageLen++] = ' ';
    scrollLeftIfNeeded();
    updateLcd();
  }
}
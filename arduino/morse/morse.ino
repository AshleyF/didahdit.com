#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

const int PADDLE_LEFT = 8;
const int PADDLE_RIGHT = 9;
const int STRAIGHT_KEY = 2;
const int RADIO = 3;
const int BUZZER = 13;

const int FREQ = 750;

long WPM;
long DIT;
long DAH;
long LETTER;
long WORD;
long PAUSE;
long SEND_FARNSWORTH;
long COPY_FARNSWORTH;
long SCROLL;
long DEBOUNCE;

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

bool sideTone = true;

enum Mode {
  IambicA,
  IambicB,
  Ultimatic,
} mode = Ultimatic;

enum DitDah {
  None,
  Dit,
  Dah,
} memory = None;

enum State {
  Waiting,
  Protocol,
  Key,
  Left,
  Right,
  LeftRight,
  RightLeft,
} state = Waiting, lastState = Waiting;

void setSpeed(long wpm) {
  WPM = wpm;
  DIT = 60000000 / (WPM * 50);
  DAH = DIT * 3;
  LETTER = DIT * 3;
  WORD = DIT * 7;
  PAUSE = DIT;
  SEND_FARNSWORTH = 3;
  COPY_FARNSWORTH = 3;
  SCROLL = DIT * 100;
  DEBOUNCE = DIT;
}

void setup() {
  setSpeed(25);
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

bool left = false;
long lastLeftChange = 0;
bool right = false;
long lastRightChange = 0;
bool key = false;
long lastKeyChange = 0;

DitDah oppositeLast = None;

bool lastOpenEndedTone = false;
long toneStart = 0;
long toneUntil = 0;
long quietUntil = 0;
long predictDah = 0;
long repeat = 0;
bool lastSqueeze = false;

long quietStart = 0;
void playTone(DitDah ditDah, bool openEnded, bool copy) {
  if (ditDah != None) {
    long len = ditDah == Dah ? DAH : DIT;
    lastOpenEndedTone = openEnded;
    if (!openEnded && copy) {
      decode(ditDah);
      quietUntil = micros() + len + DIT;
    }
    oppositeLast = ditDah == Dit ? Dah : Dit;
    if (sideTone) {
      tone(BUZZER, FREQ, len / 1000);
    }
    digitalWrite(RADIO, HIGH);
    toneStart = micros();
    toneUntil = toneStart + len;
    quietStart = toneUntil;
  }
}

bool playMemory() {
  if (memory == None) return false;
  playTone(memory, false, true);
  memory = None;
  return true;
}

void stopTone() {
  if (sideTone) {
    noTone(BUZZER);
  }
  digitalWrite(RADIO, LOW);
  quietStart = micros();
}

void serialDit() {
  Serial.write('.');
  Serial.flush();
}

void serialDah() {
  Serial.write('-');
  Serial.flush();
}

void serialKeyUpdate(bool leftDown, bool rightDown, bool straightDown) {
  Serial.write((byte)(
    (leftDown     ? 0b001 : 0b000) |
    (rightDown    ? 0b010 : 0b000) |
    (straightDown ? 0b100 : 0b000)));
  Serial.flush();
}

void loop() {
  long now = micros();
  bool keyState = digitalRead(STRAIGHT_KEY) == LOW;
  bool leftState = digitalRead(PADDLE_LEFT) == LOW;
  bool rightState = digitalRead(PADDLE_RIGHT) == LOW;

  if (keyState != key) {
    if ((now - lastKeyChange) > DEBOUNCE) {
      key = keyState;
      lastKeyChange = now;
    }
  }

  if (leftState != left) {
    if ((now - lastLeftChange) > DEBOUNCE) {
      left = leftState;
      lastLeftChange = now;
    }
  }

  if (rightState != right) {
    if ((now - lastRightChange) > DEBOUNCE) {
      right = rightState;
      lastRightChange = now;
    }
  }

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
  if (state != Key && toneUntil != 0 && now > toneUntil) stopTone();
  switch (state) {
    case Waiting:
      if (toneUntil != 0 && now > toneUntil) {
        stopTone();
        if (lastOpenEndedTone) {
          decode(now - toneStart >= DAH ? Dah : Dit);
        }
        toneUntil = 0;
        predictDah = 0;
        if (quietUntil == 0) {
          quietUntil = now + PAUSE;
        }
      }
      if (mode == IambicB && lastSqueeze && now > toneUntil && now > quietUntil) {
        lastSqueeze = false;
        toneUntil = now + (oppositeLast == Dah ? DAH : DIT);
        playTone(oppositeLast, false, true);
      }
      if (now > quietUntil && now > toneUntil) {
        if (mode != IambicB) {
          playMemory();
        }
        if (Serial.available() > 0) {
          state = Protocol;
          break;
        }
      }
      if (key) {
        state = Key;
        break;
      }
      if (left) {
        memory = Dit;
        state = Left;
        break;
      }
      if (right) {
        memory = Dah;
        state = Right;
        break;
      }
      if (now > quietUntil) {
        long time = now - quietStart;
        pause(time);
        if (time > SCROLL) {
          int len = offscreenLen + messageLen;
          if (len > 15) {
            int scroll = ((time - SCROLL) / 350000) % len;
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
      lcd.clear();
      switch (Serial.read()) {
        case '.':
          playTone(Dit, false, false);
          quietUntil = 0;
          toneUntil = now + DIT;
          break;
        case '-':
          playTone(Dah, false, false);
          quietUntil = 0;
          toneUntil = now + DAH;
          break;
        case ' ':
          quietUntil = now + LETTER * SEND_FARNSWORTH;
          break;
        case '/':
          quietUntil = now + WORD * SEND_FARNSWORTH;
          break;
        case 'A':
          mode = IambicA;
          break;
        case 'B':
          mode = IambicB;
          break;
        case 'U':
          mode = Ultimatic;
          break;
        case 'Z':
          sideTone = !sideTone;
          break;
        case 205:
          setSpeed(5);
          break;
        case 210:
          setSpeed(10);
          break;
        case 215:
          setSpeed(15);
          break;
        case 220:
          setSpeed(20);
          break;
        case 221:
          setSpeed(21);
          break;
        case 222:
          setSpeed(22);
          break;
        case 223:
          setSpeed(23);
          break;
        case 224:
          setSpeed(24);
          break;
        case 225:
          setSpeed(25);
          break;
        case 230:
          setSpeed(30);
          break;
        case 240:
          setSpeed(40);
          break;
        case 250:
          setSpeed(50);
          break;
        //default:
        //  if (b > 200) {
        //    setSpeed(b - 200);
        //  }
        //  break;
      }
      state = Waiting;
      break;
    case Key:
      if (!key) {
        state = Waiting;
        break;
      }
      if (now > quietUntil) {
        if (toneUntil == 0) {
          playTone(Dah, true, true); // maximum, open-ended
          quietUntil = 0;
          toneUntil = now + DIT; // minimum
          predictDah = now + DAH / 2;
          repeat = now + DAH + PAUSE;
        }
        if (predictDah != 0 && now > predictDah) {
          toneUntil += DIT * 2;
          predictDah = 0;
        }
        if (now > repeat) {
          if (lastOpenEndedTone) {
            decode(Dah); // must have been a dah to repeat
          }
          playTone(Dah, false, true);
          toneUntil = now + DAH;
          repeat = toneUntil + PAUSE;
        }
      }
      break;
    case Left:
      if (!left) {
        state = Waiting;
        break;
      }
      if (right) {
        memory = Dah;
        state = LeftRight;
        break;
      }
      if (now > quietUntil && now > toneUntil) {
        if (toneUntil == 0) {
          playMemory();
          quietUntil = 0;
          toneUntil = now + DIT;
          repeat = toneUntil + PAUSE;
        }
        if (now > repeat) {
          if (!playMemory()) {
            playTone(Dit, false, true);
            if (mode == IambicB) lastSqueeze = false;
            toneUntil = now + DIT;
          }
          repeat = toneUntil + PAUSE;
        }
      }
      break;
    case Right:
      if (!right) {
        state = Waiting;
        break;
      }
      if (left) {
        memory = Dit;
        state = RightLeft;
        break;
      }
      if (now > quietUntil && now > toneUntil) {
        if (toneUntil == 0) {
          playMemory();
          quietUntil = 0;
          toneUntil = now + DAH;
          repeat = toneUntil + PAUSE;
        }
        if (now > repeat) {
          if (!playMemory()) {
            playTone(Dah, false, true);
            if (mode == IambicB) lastSqueeze = false;
            toneUntil = now + DAH;
          }
          repeat = toneUntil + PAUSE;
        }
      }
      break;
    case LeftRight:
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
      if (mode == IambicB) lastSqueeze = true;
      if (toneUntil == 0) {
        playMemory();
        quietUntil = 0;
        toneUntil = now + DAH;
        repeat = toneUntil + PAUSE;
      }
      if (now > repeat) {
        if (!playMemory()) {
          if (mode == IambicA || mode == IambicB) {
            long send = oppositeLast == Dah ? DAH : DIT;
            playTone(oppositeLast, false, true);
            toneUntil = now + send;
          } else if (mode == Ultimatic) {
            playTone(Dah, false, true);
            toneUntil = now + DAH;
          }
        }
        repeat = toneUntil + PAUSE;
      }
    case RightLeft:
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
      if (mode == IambicB) lastSqueeze = true;
      if (toneUntil == 0) {
        playMemory();
        quietUntil = 0;
        toneUntil = now + DIT;
        repeat = toneUntil + PAUSE;
      }
      if (now > repeat) {
        if (!playMemory()) {
          if (mode == IambicA || mode == IambicB) {
            long send = oppositeLast == Dah ? DAH : DIT;
            playTone(oppositeLast, false, true);
            toneUntil = now + send;
          } else if (mode == Ultimatic) {
            playTone(Dit, false, true);
            toneUntil = now + DIT;
          }
        }
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

void decode(DitDah ditDah) {
  p = p * 2; // dit doubles, dah doubles + 1
  if (ditDah == Dah) {
    p++;
    appendCode(DAH_CHAR);
    serialDah();
  } else if (ditDah == Dit) {
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
    Serial.flush();
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
    Serial.flush();
    message[messageLen++] = ' ';
    scrollLeftIfNeeded();
    updateLcd();
  }
}
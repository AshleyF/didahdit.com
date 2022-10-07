#include "Keyboard.h"

// FEATURE: Upgrade dit to dah mid-dit by pressing right paddle while dit in progress (Ultimatic-only?)
// FEATURE: Sending speed dictated by PC-side (tone+durration, quiet+durration)

const int PADDLE_LEFT = 8;
const int PADDLE_RIGHT = 9;
const int RADIO = 10;
const int TONE = 4;

enum Mode {
  CurtisA,
  CurtisB,
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
  Left,
  Right,
  LeftRight,
  RightLeft,
} state = Waiting, lastState = Waiting;

long WPM;
long DIT;
long DAH;
long LETTER;
long WORD;
long PAUSE;
long DEBOUNCE;

void setSpeed(long wpm) {
  WPM = wpm;
  DIT = 60000000 / (WPM * 50);
  DAH = DIT * 3;
  LETTER = DIT * 3;
  WORD = DIT * 7;
  PAUSE = DIT;
  DEBOUNCE = DIT;
}

void setMode(Mode m) {
  mode = m;
}

void setup() {
  Serial.begin(115200);
  pinMode(PADDLE_LEFT, INPUT);
  digitalWrite(PADDLE_LEFT, HIGH);
  pinMode(PADDLE_RIGHT, INPUT);
  digitalWrite(PADDLE_RIGHT, HIGH);
  pinMode(RADIO, OUTPUT);
  pinMode(TONE, OUTPUT);
  setSpeed(20);
  Keyboard.begin();
}

bool enableTone = true;
bool enableKeyboard = true;
bool enableMemory = true;
bool left = false;
bool leftProtocol = false;
long lastLeftChange = 0;
bool right = false;
bool rightProtocol = false;
long lastRightChange = 0;
long now;
long toneUntil = 0;
long quietUntil = 0;
bool lastSqueeze = false;
DitDah oppositeLast = None;

DitDah opposite(DitDah ditDah) {
  return ditDah == Dit ? Dah : Dit;
}

void playTone(DitDah ditDah) {
  if (ditDah != None && now > quietUntil) {
    long len = ditDah == Dah ? DAH : DIT;
    oppositeLast = opposite(ditDah);
    digitalWrite(RADIO, HIGH);
    if (enableTone) analogWrite(TONE, 1);

    Serial.write(ditDah == Dit ? '.' : '-');
    Serial.flush();
    toneUntil = micros() + len;
    quietUntil = toneUntil + PAUSE;
    decode(ditDah);
  }
}

void stopTone() {
  digitalWrite(RADIO, LOW);
  analogWrite(TONE, 0);
  toneUntil = 0;
}

bool playMemory() {
  if (memory == None) return false;
  if (enableMemory) playTone(memory);
  memory = None;
  return true;
}

void serialKeyUpdate(bool leftDown, bool rightDown) {
  Serial.write((byte)(
    (leftDown  ? 0b10 : 0b00) |
    (rightDown ? 0b01 : 0b00)));
  Serial.flush();
}

void debounce(bool leftState, bool rightState) {
  if (!leftProtocol && leftState != left && (now - lastLeftChange) > DEBOUNCE) {
      left = leftState;
      lastLeftChange = now;
  }
  if (!rightProtocol && rightState != right && (now - lastRightChange) > DEBOUNCE) {
    right = rightState;
    lastRightChange = now;
  }
}

void serialRelayState() {
  if (state != lastState) {
    switch (state) {
      case Left:
        serialKeyUpdate(true, false);
        break;
      case Right:
        serialKeyUpdate(false, true);
        break;
      case LeftRight:
      case RightLeft:
        serialKeyUpdate(true, true);
        break;
      default:
        serialKeyUpdate(false, false);
        break;
    }
    lastState = state;
  }
}

void leftRightPaddle(bool current, DitDah currentDitDah, bool other, State oppositeState) {
  if (!current) {
    state = Waiting;
    return;
  }
  if (other) {
    memory = opposite(currentDitDah);
    state = oppositeState;
    return;
  }
  if (now > quietUntil) {
    if (!playMemory()) {
      playTone(currentDitDah);
      if (mode == CurtisB) lastSqueeze = false;
    }
  }
}

void squeezePaddle(DitDah currentDitDah) {
  if (!left && right) {
    state = Right;
    return;
  }
  if (left && !right) {
    state = Left;
    return;
  }
  if (!left && !right) {
    state = Waiting;
    return;
  }
  if (mode == CurtisB) lastSqueeze = true;
  if (now > quietUntil) {
    if (toneUntil == 0) playMemory();
    if (mode == CurtisA || mode == CurtisB) {
      playTone(oppositeLast);
    } else if (mode == Ultimatic) {
      playTone(currentDitDah);
    }
  }
}

void protocol() {
  int b = Serial.read();
  switch (b) {
    case '.':
      playTone(Dit);
      break;
    case '-':
      playTone(Dah);
      break;
    case ' ':
      quietUntil = now + LETTER;
      break;
    case '/':
      quietUntil = now + WORD;
      break;
    case 'A':
      setMode(CurtisA);
      break;
    case 'B':
      setMode(CurtisB);
      break;
    case 'U':
      setMode(Ultimatic);
      break;
    case 'M':
      enableMemory = true;
      break;
    case 'm':
      enableMemory = false;
      break;
    case 'T':
      enableTone = true;
      break;
    case 't':
      enableTone = false;
      break;
    case 'K':
      enableKeyboard = true;
      break;
    case 'k':
      enableKeyboard = false;
      break;
    default:
      if (b > 200) {
        setSpeed(b - 200);
      }
      break;
  }
  state = Waiting;
}

char* morse = "  etianmsurwdkgohvf l pjbxcyzq  54 3   2& +    16=/   ( 7   8 90     $      ?_    \"  .    @   '  -        ;! )     ,    :";
int p = 1;
long sinceLastOutput = -1;
long silentSince = -1; // -1 = no tone sent, 0 = tone sent
bool shift = false;
bool autoSpace = true;

void writeProsign(char* sign) {
  for (int i = 0; i < strlen(sign); i++) {
    Keyboard.write(sign[i]);
    sinceLastOutput = now;
  }
}

void decode(DitDah ditDah) {
  if (!enableKeyboard) return;
  switch (ditDah) {
    case Dit:
      p *= 2;
      silentSince = 0;
      autoSpace = true;
      break;
    case Dah:
      p = p * 2 + 1;
      silentSince = 0;
      autoSpace = true;
      break;
    case None:
      if (silentSince == 0) silentSince = now;
      if (autoSpace && silentSince == -1 && now - sinceLastOutput > WORD * 3) { // TODO: farnsworth?
        autoSpace = false;
        Keyboard.write(' ');
      }
      break;
  }
  if (silentSince > 0 && now - silentSince > LETTER) {
    silentSince = -1;
    switch (p) {
      case 31: // backspace
        Keyboard.write(KEY_BACKSPACE);
        autoSpace = false;
        break;
      case 19: // ..-- space on Google Morse keyboard
        Keyboard.write(' ');
        autoSpace = false;
        break;
      case 21: // .-.- return on Google Morse keyboard
        //writeProsign("<AA>");
        Keyboard.write(KEY_RETURN);
        autoSpace = false;
        break;
      case 34:
        writeProsign("<VE>");
        break;
      case 37:
        writeProsign("<INT>");
        break;
      case 42:
        writeProsign("<AR>");
        break;
      case 53:
        writeProsign("<CT>");
        break;
      case 66: // ....-. shift on Google Morse keyboard
        shift = true;
        break;
      case 69:
        writeProsign("<SK>");
        break;
      case 103:
        writeProsign("<NJ>");
        break;
      case 197:
        writeProsign("<BK>");
        break;
      case 568:
        writeProsign("<SOS>");
        break;
      case 256: // correction (clear)
        writeProsign("<ERR>");
        break;
      default:
        if (p < 121) {
          char c = morse[p];
          if (c != ' ') {
            Keyboard.write(shift ? toupper(c) : c);
            sinceLastOutput = now;
            shift = false;
          }
        }
        break;
    }
    p = 1;
  }
}

void waiting() {
  int peek = Serial.peek();
  if (peek >= 0 && peek < 4) { // key signal?
    state = Protocol;
    return;
  }
  if (now > quietUntil) {
    playMemory();
    if (Serial.available() > 0) {
      state = Protocol;
      return;
    }
    if (mode == CurtisB && lastSqueeze) {
      lastSqueeze = false;
      playTone(oppositeLast);
    }
  }
  if (left) {
    memory = Dit;
    state = Left;
  } else if (right) {
    memory = Dah;
    state = Right;
  }
  decode(None);
}

void loop() {
  now = micros();
  debounce(digitalRead(PADDLE_LEFT) == LOW, digitalRead(PADDLE_RIGHT) == LOW);
  serialRelayState();
  int peek = Serial.peek();
  if (peek != -1 && peek < 4) { // key signal?
    Serial.read();
    leftProtocol = (peek & 0b10) != 0;
    rightProtocol = (peek & 0b01) != 0;
  }
  if (leftProtocol) left = true;
  if (rightProtocol) right = true;
  if (toneUntil != 0 && now > toneUntil) stopTone();
  switch (state) {
    case Waiting:
      waiting();
      break;
    case Protocol:
      protocol();
      break;
    case Left:
      leftRightPaddle(left, Dit, right, LeftRight);
      break;
    case Right:
      leftRightPaddle(right, Dah, left, RightLeft);
      break;
    case LeftRight:
      squeezePaddle(Dah);
    case RightLeft:
      squeezePaddle(Dit);
      break;
  }
}
// BUG: Squeeze left+right and release between first dit and dah -- extra dit still sent
// FEATURE: Upgrade dit to dah mid-dit by pressing right paddle while dit in progress (Ultimatic-only?)

const int PADDLE_LEFT = 8;
const int PADDLE_RIGHT = 9;
const int RADIO = 10;

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
long FARNSWORTH;
long DEBOUNCE;

void setSpeed(long wpm) {
  WPM = wpm;
  DIT = 60000000 / (WPM * 50);
  DAH = DIT * 3;
  LETTER = DIT * 3;
  WORD = DIT * 7;
  PAUSE = DIT;
  FARNSWORTH = 3;
  DEBOUNCE = DIT;
}

void setMode(Mode m) {
  mode = m;
}

void setup() {
  Serial.begin(9600);
  pinMode(PADDLE_LEFT, INPUT);
  digitalWrite(PADDLE_LEFT, HIGH);
  pinMode(PADDLE_RIGHT, INPUT);
  digitalWrite(PADDLE_RIGHT, HIGH);
  pinMode(RADIO, OUTPUT);
  setSpeed(20);
}

bool left = false;
long lastLeftChange = 0;
bool right = false;
long lastRightChange = 0;

DitDah oppositeLast = None;

long toneUntil = 0;
long quietUntil = 0;
bool lastSqueeze = false;

void relay(DitDah ditDah) {
  switch (ditDah) {
    case Dit:
      Serial.write('.');
      break;
    case Dah:
      Serial.write('-');
      break;
  }
  Serial.flush();
}

void playTone(DitDah ditDah) {
  if (ditDah != None) {
    long len = ditDah == Dah ? DAH : DIT;
    oppositeLast = ditDah == Dit ? Dah : Dit;
    digitalWrite(RADIO, HIGH);
    relay(ditDah);
    toneUntil = micros() + len;
    quietUntil = toneUntil + PAUSE;
  }
}

void stopTone() {
  digitalWrite(RADIO, LOW);
  toneUntil = 0;
}

bool playMemory() {
  if (memory == None) return false;
  playTone(memory);
  memory = None;
  return true;
}

void serialKeyUpdate(bool leftDown, bool rightDown) {
  Serial.write((byte)(
    (leftDown  ? 0b001 : 0b000) |
    (rightDown ? 0b010 : 0b000)));
  Serial.flush();
}

void loop() {
  long now = micros();
  bool leftState = digitalRead(PADDLE_LEFT) == LOW;
  bool rightState = digitalRead(PADDLE_RIGHT) == LOW;

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
  if (toneUntil != 0 && now > toneUntil) stopTone();
  int b;
  switch (state) {
    case Waiting:
      if (now > quietUntil) {
        if (mode != IambicB) {
          playMemory();
        }
        if (Serial.available() > 0) {
          state = Protocol;
          break;
        }
        if (mode == IambicB && lastSqueeze) {
          lastSqueeze = false;
          playTone(oppositeLast);
        }
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
      break;
    case Protocol:
      b = Serial.read();
      switch (b) {
        case '.':
          playTone(Dit);
          break;
        case '-':
          playTone(Dah);
          break;
        case ' ':
          quietUntil = now + LETTER * FARNSWORTH;
          break;
        case '/':
          quietUntil = now + WORD * FARNSWORTH;
          break;
        case 'A':
          setMode(IambicA);
          break;
        case 'B':
          setMode(IambicB);
          break;
        case 'U':
          setMode(Ultimatic);
          break;
        default:
          if (b > 200) {
            setSpeed(b - 200);
          }
          break;
      }
      state = Waiting;
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
      if (now > quietUntil) {
        if (toneUntil == 0) {
          playMemory();
        }
        if (now > quietUntil) {
          if (!playMemory()) {
            playTone(Dit);
            if (mode == IambicB) lastSqueeze = false;
          }
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
      if (now > quietUntil) {
        if (toneUntil == 0) {
          playMemory();
        }
        if (now > quietUntil) {
          if (!playMemory()) {
            playTone(Dah);
            if (mode == IambicB) lastSqueeze = false;
          }
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
      }
      if (now > quietUntil) {
        if (!playMemory()) {
          if (mode == IambicA || mode == IambicB) {
            playTone(oppositeLast);
          } else if (mode == Ultimatic) {
            playTone(Dah);
          }
        }
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
      }
      if (now > quietUntil) {
        if (!playMemory()) {
          if (mode == IambicA || mode == IambicB) {
            playTone(oppositeLast);
          } else if (mode == Ultimatic) {
            playTone(Dit);
          }
        }
      }
      break;
  }
}
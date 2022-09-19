const int PADDLE_LEFT = 8;
const int PADDLE_RIGHT = 9;
const int RADIO = 10;

const int FREQ = 750;

long WPM;
long DIT;
long DAH;
long LETTER;
long WORD;
long PAUSE;
long SEND_FARNSWORTH;
long COPY_FARNSWORTH;
long DEBOUNCE;

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

void setSpeed(long wpm) {
  WPM = wpm;
  DIT = 60000000 / (WPM * 50);
  DAH = DIT * 3;
  LETTER = DIT * 3;
  WORD = DIT * 7;
  PAUSE = DIT;
  SEND_FARNSWORTH = 3;
  COPY_FARNSWORTH = 3;
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
  setSpeed(25);
  setMode(Ultimatic);
}

bool left = false;
long lastLeftChange = 0;
bool right = false;
long lastRightChange = 0;

DitDah oppositeLast = None;

long toneUntil = 0;
long quietUntil = 0;
bool lastSqueeze = false;

void playTone(DitDah ditDah) {
  if (ditDah != None) {
    long len = ditDah == Dah ? DAH : DIT;
    oppositeLast = ditDah == Dit ? Dah : Dit;
    digitalWrite(RADIO, HIGH);
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
  switch (state) {
    case Waiting:
      if (mode == IambicB && lastSqueeze && now > toneUntil && now > quietUntil) {
        lastSqueeze = false;
        playTone(oppositeLast);
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
      switch (Serial.read()) {
        case '.':
          playTone(Dit);
          break;
        case '-':
          playTone(Dah);
          break;
        case ' ':
          quietUntil = now + LETTER * SEND_FARNSWORTH;
          break;
        case '/':
          quietUntil = now + WORD * SEND_FARNSWORTH;
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
      if (now > quietUntil && now > toneUntil) {
        if (toneUntil == 0) {
          playMemory();
          quietUntil = 0;
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
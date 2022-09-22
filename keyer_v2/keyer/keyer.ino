// FEATURE: Upgrade dit to dah mid-dit by pressing right paddle while dit in progress (Ultimatic-only?)
// FEATURE: Sending speed dictated by PC-side (tone+durration, quiet+durration)

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
    Serial.write(ditDah == Dit ? '.' : '-');
    Serial.flush();
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

void debounce(bool leftState, bool rightState) {
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
      if (mode == IambicB) lastSqueeze = false;
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
  if (mode == IambicB) lastSqueeze = true;
  if (now > quietUntil) {
    if (toneUntil == 0) playMemory();
    if (mode == IambicA || mode == IambicB) {
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
}

void waiting() {
  if (now > quietUntil) {
    if (mode != IambicB) playMemory();
    if (Serial.available() > 0) {
      state = Protocol;
      return;
    }
    if (mode == IambicB && lastSqueeze) {
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
}

void loop() {
  now = micros();
  debounce(digitalRead(PADDLE_LEFT) == LOW, digitalRead(PADDLE_RIGHT) == LOW);
  serialRelayState();
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
class Keyer {
    constructor(elementCallback, toneCallback) {
        this.elementCallback = elementCallback; // 'dit'/'dah' (when queued) 'char'/'word' (just-in-time)
        this.toneCallback = toneCallback || (() => {}); // 'dit'/'dah' (just-in-time or after decoded) 'tone'/'silent' (straight)
        this.keyerMode = 'B'; // 'U'/'A'/'B'/'S'
        this.queueBufferLen = 1;
        this.elementTiming = null; // setSpeed() to init
        this.queue = []; // 'dit'/'dah'
        this.lastPressed = null; // 'dit'/'dah'
        this.lastQueued = null; // 'dit'/'dah'
        this.squeezedLastElement = false;
        this.currentlyHeld = { 'dit': false, 'dah': false };
        this.pending = false;
        this.breakTimer = null;
        this.lastHeldTime = -1;
        this.setSpeed(25, 25, 25); // default
    }

    #enqueue(element) {
        if (this.queue.length < this.queueBufferLen || !this.pending) {
            this.queue.push(element);
            this.lastQueued = element;
            this.elementCallback(element);
            if (!this.pending) this.#keyerUpdate();
        }
    }

    #clearBreakTimers() {
        if (this.breakTimer != null) clearTimeout(this.breakTimer);
    }

    #setTimer(fn, time) {
        this.#clearBreakTimers();
        this.breakTimer = setTimeout(fn, time);
    }

    #setBreakTimers(elementTime) {
        this.#setTimer(() => {
            this.elementCallback('char');
            this.#setTimer(() => {
                this.elementCallback('word');
            }, (this.elementTiming.word - this.elementTiming.char) * 1000); // after remaining word space (word breaks _do_ follow farnsworth/wordsworth)
        }, (elementTime + this.elementTiming.element * 3 /* keyer letter completes regardless of farnsworth this.elementTiming.char */) * 1000); // after element and char space
    }

    #opposite(element) { return element == 'dit' ? 'dah' : 'dit'; }

    #keyerUpdate() {
        var squeezing = this.currentlyHeld.dit && this.currentlyHeld.dah;
        this.pending = this.queue.length > 0;
        if (this.pending) {
            this.squeezedLastElement = squeezing;
            var element = this.queue.shift();
            this.toneCallback(element);
            var elementTime = this.elementTiming[element];
            setTimeout(() => this.#keyerUpdate(), (elementTime + this.elementTiming.element) * 1000); // after element + element space
            this.#setBreakTimers(elementTime);
        } else {
            if (squeezing) {
                switch (this.keyerMode) {
                    case 'U': // ultimatic
                        this.#enqueue(this.lastPressed);
                        break;
                    case 'A': // iambic A
                    case 'B': // iambic B
                        this.#enqueue(this.#opposite(this.lastQueued));
                        break;
                }
            }
            else if (this.currentlyHeld['dit']) { this.#enqueue('dit'); }
            else if (this.currentlyHeld['dah']) { this.#enqueue('dah'); }
            else if (this.keyerMode == 'B' && this.squeezedLastElement) {
                this.#enqueue(this.#opposite(this.lastQueued));
                this.squeezedLastElement = false;
            }
        }
    }

    set mode(mode) { this.keyerMode = mode; }
    get mode() { return this.keyerMode; }

    set bufferLen(len) { this.queueBufferLen = len; }
    get bufferLen() { return this.queueBufferLen; }

    setSpeed(wpm, farnsworth, wordsworth) { this.elementTiming = Timing(wpm, farnsworth, wordsworth); }

    #straightKey() {
        var pressed = this.currentlyHeld.dit || this.currentlyHeld.dah;
        this.toneCallback(pressed ? 'tone' : 'silent');
        if (pressed) {
            this.lastHeldTime = Date.now();
            this.#clearBreakTimers();
        } else if (this.lastHeldTime >= 0) {
            var elapsed = Date.now() - this.lastHeldTime;
            this.lastHeldTime = -1;
            this.toneCallback(elapsed >= this.elementTiming.dah * 2/3 * 1000 ? 'dah' : 'dit'); // 2/3rds dah becomes dah
            this.#setBreakTimers(0);
        }
    }

    key(element, pressed) {
        this.currentlyHeld[element] = pressed;
        if (this.keyerMode == 'S') {
            this.#straightKey();
        } else if (pressed) {
            this.lastPressed = element;
            this.#enqueue(element);
        }
    }

    get held() {
        return this.currentlyHeld;
    }
}
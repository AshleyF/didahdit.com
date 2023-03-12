class Debouncer {
    constructor(keyCallback) {
        this.keyCallback = keyCallback;
        this.debounceTimeoutMs = 10;
        this.lastPressed = false;
        this.currentlyPressed = false;
        this.timer = null;
    }

    set timeout(ms) { this.debounceTimeoutMs = ms; }
    get timeout() { return this.debounceTimeoutMs; }

    key(pressed) {
        if (this.currentlyPressed != pressed) { // ignore if state hasn't changed
            this.currentlyPressed = pressed;
            var now = Date.now();
            if (now - this.lastKeyTime > this.debounceTimeoutMs) {
                this.lastPressed = pressed;
                this.lastKeyTime = now;
                this.keyCallback(pressed);
            }
            if (this.timer == null) {
                this.timer = window.setTimeout(() => {
                    if (this.currentlyPressed != this.lastPressed) {
                        this.lastKeyTime = Date.now();
                        this.keyCallback(this.currentlyPressed);
                    }
                    this.timer = null;
                }, this.debounceTimeoutMs - (now - this.lastKeyTime));
            }
        }
    }
}
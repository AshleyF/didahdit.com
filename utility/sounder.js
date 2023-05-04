class Sounder {
    constructor() {
        this.ramp = 0.01;
        this.audioStarted = false;
        this.t = 0;
        this.wpm; // setSpeed() to init
        this.elementTiming = null; // setSpeed() to init
        this.setSpeed(25); // default
        //this.lastStartTime = 0;
    }

    start() {
        //if (this.audioStarted && (Date.now() - this.lastStartTime) > 10000) {
        //    this.stop(); // restart audio after timeout to avoid "stalls"
        //}
        //this.lastStartTime = Date.now();
        if (!this.audioStarted || this.audioCtx.state != 'running') {
            this.audioCtx = new (window.AudioContext || window.audioContext || window.webkitAudioContext)();
            this.gainVolume = this.audioCtx.createGain();
            this.gainVolume.gain.value = 1;
            this.gainVolume.connect(this.audioCtx.destination);
            this.gainMain = this.audioCtx.createGain();
            this.gainMain.gain.value = 0;
            this.gainMain.connect(this.gainVolume);
            this.toneOscillator = this.audioCtx.createOscillator();
            this.toneOscillator.frequency.value = 600;
            this.toneOscillator.connect(this.gainMain);
            this.toneOscillator.start(0);
            this.dingGain = this.audioCtx.createGain();
            this.dingGain.gain.value = 0;
            this.dingGain.connect(this.audioCtx.destination);
            this.dingOscillator = this.audioCtx.createOscillator();
            this.dingOscillator.frequency.value = 1500;
            this.dingOscillator.connect(this.dingGain);
            this.dingOscillator.start(0);
            this.dongGain = this.audioCtx.createGain();
            this.dongGain.gain.value = 0;
            this.dongGain.connect(this.audioCtx.destination);
            this.dongOscillator = this.audioCtx.createOscillator();
            this.dongOscillator.frequency.value = 1000;
            this.dongOscillator.connect(this.dongGain);
            this.dongOscillator.start(0);
            this.t = this.audioCtx.currentTime;
            this.audioStarted = true;
        }
    }

    get isStarted() { return this.audioStarted; }

    stop() {
        //if (this.audioStarted) {
        //    this.t = 0;
        //    this.lastStartTime = 0;
        //    this.audioCtx.close();
        //    this.audioStarted = false;
        //}
    }

    tone(on) {
        this.start();
        this.gainMain.gain.setTargetAtTime(on ? 1 : 0, this.time, this.ramp);
    }

    play(len) {
        this.tone(true);
        this.t += len;
        this.gainMain.gain.setTargetAtTime(0, this.t, this.ramp);
    }

    send(element) {
        switch (element) {
            case 'dit':
                this.play(this.elementTiming.dit);
                this.silence(this.elementTiming.element);
                break;
            case 'dah':
                this.play(this.elementTiming.dah);
                this.silence(this.elementTiming.element);
                break;
            case 'char':
                this.silence(this.elementTiming.char - this.elementTiming.element);
                break;
            case 'word':
                this.silence(this.elementTiming.word - this.elementTiming.element);
                break;
        }
    }

    silence(len) {
        this.start();
        this.t += len;
    }

    #bell(bellGain) {
        this.start();
        var bellTime = this.time;
        bellGain.gain.linearRampToValueAtTime(0.2, bellTime + 0.005);
        bellGain.gain.exponentialRampToValueAtTime(0.000001, bellTime + 2.4);
    }

    ding() { this.#bell(this.dingGain); }

    dong() { this.#bell(this.dongGain); }


    get isSounding() {
        this.start();
        return this.gainMain.gain.value > 0.5;
    }

    set pitch(frequency) {
        this.start();
        this.toneOscillator.frequency.value = frequency;
    }

    get pitch() { return this.toneOscillator.frequency.value; }

    set volume(vol) {
        this.start();
        this.gainVolume.gain.value = vol;
    }

    get volume() { return this.gainVolume.gain.value; }

    get time() {
        this.start();
        var c = this.audioCtx.currentTime;
        this.t = c > this.t ? c : this.t;
        return this.t;
    }

    setSpeed(wpm, farnsworth, wordsworth) {
        this.elementTiming = Timing(wpm, farnsworth, wordsworth);
    }
}
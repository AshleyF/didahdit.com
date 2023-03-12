const _morse = "  ETIANMSURWDKGOHVFÜL PJBXCYZQ  54 3   2& +    16=/   ( 7   8 90     $      ?_    \"  .    @   '  -        ;! )     ,    :";

class Encoder {
    constructor(callback) {
        this.callback = callback;
        this.prosign = false;
    }

    character(c) {
        if (c == ' ') { // word break
            this.callback('word');
        } else {
            if (c == '<') {
                this.prosign = true;
                return;
            }
            if (c == '>') {
                this.prosign = false;
                return;
            }
            var code = [];
            var j = _morse.indexOf(c.toUpperCase());
            while (j > 1) {
                if (j % 2 == 0) {
                    code.unshift('dit');
                    j /= 2;
                } else {
                    code.unshift('dah');
                    j = (j - 1) / 2;
                }
            }
            for (var i in code) {
                this.callback(code[i]);
            }
            if (!this.prosign) {
                this.callback('char');
            }
        }
    }

    phrase(text) {
        for (var i in text) {
            this.character(text[i]);
        }
    }
}

class Decoder {
    constructor(callback) {
        this.callback = callback;
        // Called with { complete: false, ... } updates as 'dit'/'dah' elements stream in.
        //   Regular characters are of the form { kind: 'char', value: 'A', ... }
        //     Including: ' ' (space) which is different to an explicit { kind: 'control', value: 'space' }
        //   Prosigns are of the form { kind: 'prosign', value: 'BT', ... }
        //   Control characters of the form { kind: 'control', value: 'backspace', ... }
        //     Including: 'space', 'return', 'backspace', 'shift', 'error'
        //   Invalid characters of the form { kind: 'invalid', value: null, ... }
        // Called again with { complete: true, ... } once 'char' break element is seen.
        // Called with { complete: true, kind: 'char', value: ' ' } for 'word' breaks.
        this.current;
        this.p;
        this.reset();
    }

    reset() {
        this.p = 1;
        this.current = null;
    }

    #partial(kind, value) {
        this.current = { complete: false, kind: kind, value: value }
        this.callback(this.current);
    }

    #prosign(sign) { this.#partial('prosign', sign); }

    #update() {
        switch (this.p) {
            case 19: // ..-- space on Google Morse keyboard
                this.#partial('control', 'space');
                break;
            case 21: // .-.- return on Google Morse keyboard (or AA prosign)
                this.#partial('control', 'return');
                break;
            case 31: // backspace
                this.#partial('control', 'backspace');
                break;
            case 34:
                this.#prosign('VE');
                break;
            case 37:
                this.#prosign('INT');
                break;
            case 42:
                this.#prosign('AR');
                break;
            case 49:
                this.#prosign('BT');
                break;
            case 53:
                this.#prosign('CT');
                break;
            case 54:
                this.#prosign('KN'); // overrides '(' char
                break;
            case 66: // ....-. shift on Google Morse keyboard
                this.#partial('control', 'shift');
                break;
            case 69:
                this.#prosign('SK');
                break;
            case 103:
                this.#prosign('NJ');
                break;
            case 197:
                this.#prosign('BK');
                break;
            case 568:
                this.#prosign('SOS');
                break;
            case 256:
                this.#partial('control', 'error');
                break;
            default:
                if (this.p >= 121) { // invalid
                    this.#partial('invalid', null);
                } else {
                    var char = _morse[this.p];
                    if (char == ' ') { // invalid?
                        this.#partial('invalid', null);
                    } else {
                        this.#partial('char', char);
                    }
                }
                break;
        }
    }

    element(element) {
        switch (element) {
            case 'dit':
                this.p *= 2;
                this.#update();
                break;
            case 'dah':
                this.p = this.p * 2 + 1;
                this.#update();
                break;
            case 'char':
                if (this.current != null) {
                    this.current.complete = true;
                    this.callback(this.current);
                }
                this.reset();
                return;
            case 'word':
                this.callback({ complete: true, kind: 'char', value: ' ' });
                this.reset();
                return;
        }
    }
}
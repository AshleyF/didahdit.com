function Decoder(callback) {
    var _callback = callback;
    // Called with { complete: false, ... } updates as 'dit'/'dah' elements stream in.
    //   Regular characters are of the form { kind: 'character', value: 'A', ... }
    //     Including: ' ' (space) which is different to an explicit { kind: 'control', value: 'space' }
    //   Prosigns are of the form { kind: 'prosign', value: 'BT', ... }
    //   Control characters of the form { kind: 'control', value: 'backspace', ... }
    //     Including: 'space', 'return', 'backspace', 'shift', 'error'
    //   Invalid characters of the form { kind: 'invalid', value: null, ... }
    // Called again with { complete: true, ... } once 'letter' break element is seen.
    // Called with { complete: true, kind: 'character', value: ' ' } for 'word' breaks.

    var _current;
    var _p;

    function reset() {
        _p = 1;
        _current = null;
    }

    reset();

    function _partial() {
        function _partial(kind, value) {
            _current = { complete: false, kind: kind, value: value }
            _callback(_current);
        }
        function _prosign(sign) { _partial('prosign', sign); }
        switch (_p) {
            case 19: // ..-- space on Google Morse keyboard
                _partial('control', 'space');
                break;
            case 21: // .-.- return on Google Morse keyboard (or AA prosign)
                _partial('control', 'return');
                break;
            case 31: // backspace
                _partial('control', 'backspace');
                break;
            case 34:
                _prosign('VE');
                break;
            case 37:
                _prosign('INT');
                break;
            case 42:
                _prosign('AR');
                break;
            case 49:
                _prosign('BT');
                break;
            case 53:
                _prosign('CT');
                break;
            case 54:
                _prosign('KN'); // overrides '(' char
                break;
            case 66: // ....-. shift on Google Morse keyboard
                _partial('control', 'shift');
                break;
            case 69:
                _prosign('SK');
                break;
            case 103:
                _prosign('NJ');
                break;
            case 197:
                _prosign('BK');
                break;
            case 568:
                _prosign('SOS');
                break;
            case 256:
                _partial('control', 'error');
                break;
            default:
                if (_p >= 121) { // invalid
                    _partial('invalid', null);
                } else {
                    var char = "  ETIANMSURWDKGOHVFÜL PJBXCYZQ  54 3   2& +    16=/   ( 7   8 90     $      ?_    \"  .    @   '  -        ;! )     ,    :"[_p];
                    if (char == ' ') { // invalid?
                        _partial('invalid', null);
                    } else {
                        _partial('character', char);
                    }
                }
                break;
        }
    }

    function element(element) {
        switch (element) {
            case 'dit':
                _p *= 2;
                _partial();
                break;
            case 'dah':
                _p = _p * 2 + 1;
                _partial();
                break;
            case 'letter':
                if (_current != null) {
                    _current.complete = true;
                    _callback(_current);
                }
                reset();
                return;
            case 'word':
                _callback({ complete: true, kind: 'character', value: ' ' });
                reset();
                return;
        }
    }

    return {
        element,
    };
}
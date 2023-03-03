function Keyer(elementCallback, toneCallback) {
    var _elementCallback = elementCallback; // 'dit'/'dah' (when queued) 'letter'/'word' (just-in-time)
    var _toneCallback = toneCallback || function() {}; // 'dit'/'dah' (just-in-time or after decoded) 'tone'/'silent' (straight)
    var _mode = 'B'; // 'U'/'A'/'B'/'S'
    var _bufferLen = 1;
    var _elementTiming = null; // setSpeed() to init
    var _queue = []; // 'dit'/'dah'
    var _lastPressed = null; // 'dit'/'dah'
    var _lastQueued = null; // 'dit'/'dah'
    var _squeezedLastElement = false;
    var _currentlyHeld = { 'dit': false, 'dah': false };
    var _pending = false;
    var _breakTimer = null;
    var _lastHeldTime = -1;

    setSpeed(25); // default

    function _enqueue(element) {
        if (_queue.length < _bufferLen || !_pending) {
            _queue.push(element);
            _lastQueued = element;
            _elementCallback(element);
            if (!_pending) _keyerUpdate();
        }
    }

    function _clearBreakTimers() {
        if (_breakTimer != null) clearTimeout(_breakTimer);
    }

    function _setBreakTimers(elementTime) {
        function _setTimer(fn, time) {
            _clearBreakTimers();
            _breakTimer = setTimeout(fn, time);
        }
        _setTimer(function () {
            _elementCallback('letter');
            _setTimer(function () {
                _elementCallback('word');
            }, _elementTiming.word - _elementTiming.letter);
        }, _elementTiming.letter + elementTime);
    }

    function _keyerUpdate() {
        function _opposite(element) { return element == 'dit' ? 'dah' : 'dit'; }
        var squeezing = _currentlyHeld.dit && _currentlyHeld.dah;
        _pending = _queue.length > 0;
        if (_pending) {
            _squeezedLastElement = squeezing;
            var element = _queue.shift();
            _toneCallback(element);
            var elementTime = _elementTiming[element];
            setTimeout(_keyerUpdate, elementTime);
            _setBreakTimers(elementTime);
        } else {
            if (squeezing) {
                switch (_mode) {
                    case 'U': // ultimatic
                        _enqueue(_lastPressed);
                        break;
                    case 'A': // iambic A
                    case 'B': // iambic B
                        _enqueue(_opposite(_lastQueued));
                        break;
                }
            }
            else if (_currentlyHeld['dit']) { _enqueue('dit'); }
            else if (_currentlyHeld['dah']) { _enqueue('dah'); }
            else if (_mode == 'B' && _squeezedLastElement) {
                _enqueue(_opposite(_lastQueued));
                _squeezedLastElement = false;
            }
        }
    }

    function setMode(mode) { _mode = mode; }

    function setBufferLen(len) { _bufferLen = len; }

    function setSpeed(wpm) {
        var ditlen = 1.2 / wpm * 1000;
        _elementTiming = {
            'dit': 2 * ditlen,
            'dah': 4 * ditlen,
            'letter': 2 * ditlen, // TODO: farnsworth? Also, 2 dits (rather than 3) to allow faster than proper sending
            'word': 5 * ditlen }; // TODO: wordsworth? Also, 5 (rather than 7) dits to allow faster than proper sending
    }

    function _straightKey() {
        var pressed = _currentlyHeld['dit'] || _currentlyHeld['dah'];
        _toneCallback(pressed ? 'tone' : 'silent');
        if (pressed) {
            _lastHeldTime = Date.now();
            _clearBreakTimers();
        } else if (_lastHeldTime >= 0) {
            var elapsed = Date.now() - _lastHeldTime;
            _lastHeldTime = -1;
            _toneCallback(elapsed >= _elementTiming.dit * 2 ? 'dah' : 'dit');
            _setBreakTimers(0);
        }
    }

    function key(element, pressed) {
        _currentlyHeld[element] = pressed;
        if (_mode == 'S') {
            _straightKey();
        } else if (pressed) {
            _lastPressed = element;
            _enqueue(element);
        }
    }

    return {
        key,
        setMode,
        setBufferLen,
        setSpeed,
    };
}
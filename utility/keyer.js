function Keyer(elementCallback, jitElementCallback) {
    var _elementCallback = elementCallback;
    var _jitElementCallback = jitElementCallback || function() {};
    var _mode = 'B'; // 'U'/'A'/'B'
    var _bufferLen = 1;
    var _elementTiming = null; // setSpeed() to init
    var _queue = []; // 'dit'/'dah'
    var _lastPressed = null; // 'dit'/'dah'
    var _lastQueued = null; // 'dit'/'dah'
    var _squeezedLastElement = false;
    var _currentlyHeld = { 'dit': false, 'dah': false };
    var _pending = false;
    var _breakTimer = null;

    setSpeed(25); // default

    function _opposite(element) {
        return element == 'dit' ? 'dah' : 'dit';
    }

    function _enqueue(element) {
        if (_queue.length < _bufferLen || !_pending) {
            _queue.push(element);
            _lastQueued = element;
            _elementCallback(element);
            if (!_pending) _keyerUpdate();
        }
    }

    function _setBreakTimer(callback, time) {
        if (_breakTimer != null) clearTimeout(_breakTimer);
        _breakTimer = setTimeout(callback, time);
    }

    function _keyerUpdate() {
        var squeezing = _currentlyHeld.dit && _currentlyHeld.dah;
        _pending = _queue.length > 0;
        if (_pending) {
            _squeezedLastElement = squeezing;
            var element = _queue.shift();
            _jitElementCallback(element);
            var elementTime = _elementTiming[element];
            setTimeout(_keyerUpdate, elementTime);
            _setBreakTimer(function () {
                _elementCallback('letter');
                _setBreakTimer(function () {
                    _elementCallback('word');
                }, _elementTiming.word - _elementTiming.letter);
            }, _elementTiming.letter + elementTime);
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
            'letter': 3 * ditlen, // TODO: farnsworth
            'word': 7 * ditlen }; // TODO: wordsworth
    }

    function key(element, pressed) {
        _currentlyHeld[element] = pressed;
        if (pressed) {
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
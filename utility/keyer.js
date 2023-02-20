function Keyer(elementQueuedCallback, elementCallback) {
    var _elementQueuedCallback = elementQueuedCallback;
    var _elementCallback = elementCallback;

    var _mode = 'B'; // 'U'/'A'/'B'
    var _elementSeconds = null; // setSpeed() to init
    var _queue = []; // 'dit'/'dah'
    var _lastPressed = null; // 'dit'/'dah'
    var _lastQueued = null; // 'dit'/'dah'
    var _squeezedLastElement = false;
    var _currentlyHeld = { 'dit': false, 'dah': false };
    var _pending = false;

    setSpeed(25); // default

    function _opposite(element) {
        return element == 'dit' ? 'dah' : 'dit';
    }

    function _enqueue(element) {
        _queue.push(element);
        _lastQueued = element;
        _elementQueuedCallback(element);
        if (!_pending) _keyerUpdate();
    }

    function _keyerUpdate() {
        var squeezing = _currentlyHeld.dit && _currentlyHeld.dah;
        _pending = _queue.length > 0;
        if (_pending) {
            _squeezedLastElement = squeezing;
            var element = _queue.shift();
            _elementCallback(element);
            setTimeout(_keyerUpdate, _elementSeconds[element]);
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

    function setSpeed(wpm) {
        var ditlen = 1.2 / wpm * 1000;
        _elementSeconds = { 'dit': 2 * ditlen, 'dah': 4 * ditlen };
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
        setSpeed,
    };
}
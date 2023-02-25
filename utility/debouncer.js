function Debouncer(keyCallback) {
    var _keyCallback = keyCallback;
    var _debounceTimeoutMs = 10;
    var _lastKeyTime = 0;
    var _lastPressed = false;
    var _currentlyPressed = false;
    var _timeout = null;

    function setTimeout(ms) { _debounceTimeoutMs = ms; }
    function getTimeout() { return _debounceTimeoutMs; }

    function key(pressed) {
        _currentlyPressed = pressed;
        var now = Date.now();
        if (now - _lastKeyTime > _debounceTimeoutMs) {
            _lastPressed = pressed;
            _lastKeyTime = now;
            _keyCallback(pressed);
        }
        if (_timeout == null) {
            _timeout = window.setTimeout(function() {
                if (_currentlyPressed != _lastPressed) {
                    _lastKeyTime = Date.now();
                    _keyCallback(_currentlyPressed);
                }
                _timeout = null;
            }, _debounceTimeoutMs - (now - _lastKeyTime));
        }
    }

    return {
        key,
        setTimeout,
        getTimeout,
    };
}
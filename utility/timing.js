function Timing(wpm, farnsworth, wordsworth) {
    _wpm = wpm;
    var ditlen = 1.2 / wpm;
    var fditlen = (60 / Math.min(farnsworth, wpm) - 31 * ditlen) / 19;
    var wditlen = (60 / Math.min(wordsworth, Math.min(farnsworth, wpm)) - 31 * ditlen) / 19;
    return {
        'dit': ditlen, // length of dit
        'dah': 3 * ditlen, // length of dah
        'element': ditlen, // space between elements
        'char': 3 * fditlen, // space between characters
        'word': 7 * wditlen }; // space between words
}
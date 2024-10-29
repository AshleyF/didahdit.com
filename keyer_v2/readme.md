# v2 Arduino Morse Decoder

This is an even simpler Arduino build (see the [old version](../keyer_v1)) made to run on the Seeed SAMD board. It has no screen, dimmer, or buzzer. It merely connects to a key/paddle, to a radio, and to a computer that drives and configures everything via the web page.

![Hardware](keyer.jpg)

The [web interface](default.htm) connects over serial (works in Chrome) and allows for adjusting settings and switching modes and such. It visualizes the paddle state and code generated along with decoded characters, etc.

Another [version here](https://github.com/AshleyF/didahdit.com/commit/4781834e1abf64f993f0b31d63c0c54f0a439cf3#diff-a56a90ee2ad94159f28ea99556e6c402a691b454d623213e50e8ebf78e0c81d6R3) was a quick hack to create a visualization for [this YouTube video](https://youtu.be/Hn4j2nfdKNE).

# ScaryDuino
Animated Halloween Pumpkin Eyes - Arduino Style

![Assembled Pumpkin](pumpkpin.png)

## Parts (aka BOM)

  * Arduino Nano (or compatible)
  * 2x MAX7219 8x8 LED Modules
  * Wires
  * Pumpkin (or sth else you would like to look at you and wink ;)

## Assembly

In my case the LED modules still needed some light soldering for the header
pins. After that was done the whole setup comes together with F/F jumper wires.
I chose to use digital pins 10-12 on the Arduino, connecting them like this
to the first LED Array (and the second one is simply daisy chained with all
pins connected to their counterparts VCC-VCC, CLK-CLK, ...):

|Arduino |LED Array|
|--------|---------|
|  5V    |   VCC   |
|  GND   |   GND   |
| Pin 10 |   CLK   |
| Pin 11 |   CS    |
| Pin 12 |   DIN   |

See the original project for a nice [connection diagram](https://mjanyst.weebly.com/arduino-pumpkin-eyes.html).

## Code

The code is using the [LEDControl](https://www.arduino.cc/reference/en/libraries/ledcontrol/)
library to talk to the MAX7219's.

I went for a relatively simple program with delays here, because there wasn’t any input
to record or similar that would have made this problematic. A fancier version here might
use timers instead, but it felt unnecessary (I guess some would already argue that the
animation classes are overblown ;).

The high-level pseudocode for the animations is

```
  Center eyes
  Wait
  Blink left
  Blink right
  Wait
  Loop:
    Look in random direction
    Wait
    1 in 5 chance of Blinking both eyes (wait 500ms before and after)
    Randomly play one of these animations some of the time:
      Cross eyes (move pupils to middle by 2 pixels, wait, move back)
      Horizontal spin (move pupils horizontally with wraparound)
      Big eye (draw a larger pupil on one of the eyes)
      Lazy eye (lower pupil on one eye, wait, move back)
      Crazy blink (blink left eye, blink right eye)
      Glow eyes (flash intensity of displays, slowly lower)
      Slow machine (spin eyes vertically with wraparound, stop one eye slower than the other, flash intensity)
    Wait
```

### LED Matrix Layout

For some reason the modules I received were mirrored both vertically and horizontally.
That is, LED `(0,0)` was _bottom right_ corner instead of the expected
top left (maybe I messed up the assembly). To adapt for this, `Eye::show` has
code that can `INVERT` the canvas when emitting it to the leds. You can use
global `const bool INVERT=true` to control this (not sure if there might be
cases where only one axis is inverted, I didn't implement support to mirror
these individually).

## Acknoledgements

This is my version of [Michal T Janyst's project](https://mjanyst.weebly.com/arduino-pumpkin-eyes.html). 
I loved the idea and wanted to build my own for some Halloween fun. While I did look at the original
project for inspiration (e.g., for the hardware and which animations to use), I wrote the code from
scratch. Maybe someone finds this version more useful/interesting (but do check out the original, too).

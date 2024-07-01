# mozzi_experiments

Experiments with Mozzi, mostly on SAMD21- and RP2040-based boards

## Sketches


* [eighties_dystopia](eighties_dystopia/eighties_dystopia.ino) - A swirling ominous wub that evolves over time

* [eighties_dystopia_rp2040](eighties_dystopia_rp2040/eighties_dystopia_rp2040.ino) - Same thing, but on a Raspberry Pi Pico w/ PWM audio

* [eighties_dystopia_rp2040_i2s](eighties_dystopia_rp2040_i2s/eighties_dystopia_rp2040_i2s.ino) - Same thing, but on a Raspberry Pi Pico w/ I2S DAC

* [eighties_arp](eighties_arp/eighties_arp.ino) - An arpeggio explorer for non-musicians and test bed for my "Arpy" arpeggio library

* [eighties_drums](eighties_drums/eighties_drums.ino) - simple 4-channel drum machine w/ sequencer

* [derpnote2](derpnote2/derpnote2.ino) - Tries to recreate THX "Deep Note" sound

* and others in the repo


## Demos

* "derpnote2" - [demo on youtube](https://www.youtube.com/watch?v=7fX8cBwbOmU)

* "eighties_drums" - [demo on youtube](https://www.youtube.com/watch?v=Jtr5wm48R7A)

* "eighties_dystopia" - See [this Twitter thread](https://twitter.com/todbot/status/1475970495400923137) or
  [this blog post on Adafruit](https://blog.adafruit.com/2021/12/29/make-your-own-80s-dystopian-music-soundtrack-qtpy-arduino-todbot/)
  or this video below

   https://github.com/todbot/mozzi_experiments/assets/274093/b39463fb-55a2-458a-9ce9-b2939a42408d

* "eighties_arp" - click for [youtube video](https://www.youtube.com/watch?v=Ql72YoCJ8-8)

  [![youtube link](eighties_arp/eighties_arp_youtube.jpg) ](https://www.youtube.com/watch?v=Ql72YoCJ8-8)

  <a href="eighties_arp/eighties_arp_wiring.jpg"><img src="eighties_arp/eighties_arp_wiring.jpg" alt="wiring" width="700"/></a>


## How to install Mozzi

Mozzi 2 is now in the Arduino Library Manager! And it's much easier to configure. 
To install Mozzi, find it in the Arduino Library Manager and install it and its
dependencies. 

## Configuring Mozzi

You can now configure Mozzi in your sketch intead of editing Mozzi library files.

The [hardware configuration section](https://sensorium.github.io/Mozzi/doc/html/hardware.html)
of the Mozzi docs is very helpful.

The pattern is that you put this at the top of your sketch:

```c++
#include "MozziConfigValues.h"  // for named option values
// your system-specific configuration defines, including MOZZI_CONTROL_RATE
#include "Mozzi.h"
```

The configs I've used so far. Some of these configs are the defaults but I like 
to be explicit. 

- SAMD21 boards using DAC on A0 as output:

    ```c++
    #include "MozziConfigValues.h"  // for named option values
    #define MOZZI_ANALOG_READ MOZZI_ANALOG_READ_NONE
    #define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()
    #include "Mozzi.h"
    ```

- RP2040 boards (including Raspberry Pi Pico) using PWM audio out:

    ```c++
    #include "MozziConfigValues.h"  // for named option values
    #define MOZZI_OUTPUT_MODE MOZZI_OUTPUT_PWM
    #define MOZZI_ANALOG_READ MOZZI_ANALOG_READ_NONE
    #define MOZZI_AUDIO_PIN_1 0  // GPIO pin number, can be any pin
    #define MOZZI_AUDIO_RATE 32768
    #define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()
    #include "Mozzi.h"
    ```

- RP2040 boards using I2S DAC audio out:

    ```c++
    #include "MozziConfigValues.h"  // for named option values
    #define MOZZI_AUDIO_MODE MOZZI_OUTPUT_I2S_DAC
    #define MOZZI_AUDIO_CHANNELS MOZZI_STEREO 
    #define MOZZI_AUDIO_BITS 16
    #define MOZZI_AUDIO_RATE 32768
    #define MOZZI_I2S_PIN_BCK 20
    #define MOZZI_I2S_PIN_WS (MOZZI_I2S_PIN_BCK+1) // HAS TO BE NEXT TO pBCLK, i.e. default is 21
    #define MOZZI_I2S_PIN_DATA 22
    #define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()
    #include "Mozzi.h"
    ```

## Using Mozzi

- Mozzi is very particular about what is in `loop()`. Do not put anything else in there.
  Instead put it in the `void updateControl()` function. See the sketches for examples.



## Any questions?

Open up an issue on this repo or contact me on [@todbot@mastodon.social](https://mastodon.social/@todbot/)!

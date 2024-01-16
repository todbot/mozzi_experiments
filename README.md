# mozzi_experiments

Experiments with Mozzi, mostly on SAMD21- and RP2040-based boards

## Sketches


* [eighties_dystopia](eighties_dystopia/eighties_dystopia.ino) - A swirling ominous wub that evolves over time

* [eighties_dystopia_rp2040](eighties_dystopia_rp2040/eighties_dystopia_rp2040.ino) - Same thing, but on a Raspberry Pi Pico

* [eighties_arp](eighties_arp/eighties_arp.ino) - An arpeggio explorer for non-musicians and test bed for my "Arpy" arpeggio library

* [eighties_drums](eighties_drums/eighties_drums.ino) - simple 4-channel drum machine w/ sequencer

* [derpnote2](derpnote2/derpnote2.ino) - Tries to recreate THX "Deep Note" sound

* and others in the repo


## Demos

* "eighties_dystopia" - See [this Twitter thread](https://twitter.com/todbot/status/1475970495400923137) or
  [this blog post on Adafruit](https://blog.adafruit.com/2021/12/29/make-your-own-80s-dystopian-music-soundtrack-qtpy-arduino-todbot/)
  or this video below

https://github.com/todbot/mozzi_experiments/assets/274093/b39463fb-55a2-458a-9ce9-b2939a42408d



* "eighties_arp" - click for [youtube video](https://www.youtube.com/watch?v=Ql72YoCJ8-8)

  [![youtube link](eighties_arp/eighties_arp_youtube.jpg) ](https://www.youtube.com/watch?v=Ql72YoCJ8-8)

  <a href="eighties_arp/eighties_arp_wiring.jpg"><img src="eighties_arp/eighties_arp_wiring.jpg" alt="wiring" width="700"/></a>

* "eighties_drums" - [demo on youtube](https://www.youtube.com/watch?v=Jtr5wm48R7A)

* "derpnote2" - [demo video on twitter](https://twitter.com/todbot/status/1483512885779173377)


## How to install Mozzi

Mozzi is not in the Arduino Library Manager, but installing it is pretty easy anyway.
The installation process is the same way you hand-install any Arduino library.

- Go to https://github.com/sensorium/Mozzi and click on the green "Code" button,
then click "Download ZIP".
- You will get a "Mozzi-master.zip" file. Rename it to "Mozzi.zip"
- In the Arduino IDE, go to "Sketch" -> "Include Library" -> "Install .ZIP library...", then pick your "Mozzi.zip" file.
- Once it completes, you will see Mozzi in the list of available libraries and the Mozzi examples
in "File" -> "Examples" -> "Mozzi".  Many of the examples will work with Scout but not use the keyboard.


## Using Mozzi

- Mozzi is very particular about what is in `loop()`. Do not put anything else in there.
  Instead put it in the `void updateControl()` function. See the sketches for examples.


## Any questions?

Open up an issue on this repo or contact me on [twitter/@todbot](https://twitter.com/todbot)!

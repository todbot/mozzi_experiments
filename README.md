# mozzi_experiments

Experiments with Mozzi, mostly on SAMD21 chips


## Demos




## Sketches

- [eighties_dystopia](eighties_dystopia/eighties_dystopia.ino) - A swirling ominous wub that evolves over time



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
- Mozzi output is quieter than standard Scout (which outputs full-width square waves).
  Use an external amp for best results.

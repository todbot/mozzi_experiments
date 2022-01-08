/**
 * eighties_dystopia_rp2040_happybass.ino --
 *  - You can dance if you want to, in the dystopia...
 *  - No user input, just wallow in the sound
 *  
 *  Circuit:
 *  - Copy the PWM cleanup RC filter from schematic figure 3.4.1 "PWM Audio" in
 *     https://datasheets.raspberrypi.com/rp2040/hardware-design-with-rp2040.pdf
 *     also see: https://www.youtube.com/watch?v=rwPTpMuvSXg
 *  - Wire GP0 to input of this circuit, output to TRRS Tip & Ring1
 *  - On a Trinkey QT2040, you can use (abuse) the StemmaQT port!
 *  
 *  Compiling:
 *  - Use the RP2040 Arduino core at: https://github.com/earlephilhower/arduino-pico
 *  - Use the Mozzi fork at https://github.com/pschatzmann/Mozzi
 *  - For slightly better audio quality, add the following after "#define PWM_RATE" in "Mozzi/AudioConfigRP2040.h"
 *      #define AUDIO_BITS 10
 *      #define AUDIO_BIAS ((uint16_t) 512)
 *      #define PWM_RATE (60000*2)
 *  
 *  Code:
 *  - Knob on A0 controls lowpass filter cutoff frequency
 *  - Knob on A1 controls lowpass filter resonance
 *  - Five detuned oscillators are randomly detuned very second or so
 *  - Every 17.5 seconds, a new note is randomly chosen from the allowed note list
 *  
 * 28 Dec 2021 - @todbot
 * 
 */
 // Mozzi is very naughty about a few things
#pragma GCC diagnostic ignored "-Wno-expansion-to-defined"
 
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <ADSR.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

#define NUM_VOICES 5

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];

Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lpf;
uint8_t resonance = 170; // range 0-255, 255 is most resonant

//uint8_t notes[] = {33, 37, 40, 42}; // possible notes to play MIDI A1, C#, E, F
uint8_t notes[] = {34, 37, 39, 42}; // possible notes to play MIDI A#, C#, D#, F#
uint16_t note_duration = 250;
uint8_t note_id = 0;
int octave = 0;
byte beat_tick = 0;
uint32_t lastMillis = 0;

void setup() {
  // RP2040 defaults to GP0, from https://github.com/pschatzmann/Mozzi/
  // Mozzi.setPin(0,2); // this sets RP2040 GP2
  // Mozzi.setPin(0,16); // this sets RP2040 GP16  // MacroPad
   Mozzi.setPin(0,16); // this sets RP2040 GP16  // Trinkey QT2040 GP16 
  // pinMode(14,OUTPUT);
  // digitalWrite(14, HIGH);
  Serial.begin(115200);
  startMozzi();
  kFilterMod.setFreq(0.08f);
  lpf.setCutoffFreqAndResonance(20, resonance);
  envelope.setADLevels(255, 10);
  envelope.setTimes(20, 300, 200, 300 );

  for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
  setNotes();
}

void loop() {
  audioHook();
}

void setNotes() {
  beat_tick = (beat_tick+1) % 8;  
  octave = 0;
  if( beat_tick % 2 == 1 ) {  // every odd beat, up an oactive
    octave = 1;
  }
  if( beat_tick == 0 ) { 
    note_id = rand(4); //(note_id+1) % 3;
  }
  byte note = notes[note_id] + 12;
  note = note + 12*octave;
  float f = mtof(note);
  for(int i=1;i<NUM_VOICES-1;i++) {
    aOscs[i].setFreq( f + (float)rand(100)/200); // orig 1.001, 1.002, 1.004
  }
  aOscs[NUM_VOICES-1].setFreq( (f/2) + (float)rand(100)/1000);
  envelope.noteOn();
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  //byte cutoff_freq = 67 + kFilterMod.next()/2;
  byte cutoff_freq = analogRead(A0) / 16; 
  byte resonance = analogRead(A1) / 16; 
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  envelope.update();

//  if(rand(CONTROL_RATE) == 0) { // about once every second
//    Serial.println("!");
//    Serial.print(cutoff_freq); Serial.print(":"); Serial.print(resonance);
//    kFilterMod.setFreq((float)rand(255)/4096);  // choose a new modulation frequency
//  }
  
  if( millis() - lastMillis > note_duration )  {
    lastMillis = millis();
    setNotes(); // wiggle the tuning a little
    Serial.println((byte)notes[note_id]);
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = 0;
  for( int i=0; i<NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
  asig = lpf.next(asig);
  return MonoOutput::fromAlmostNBit(19, envelope.next() * asig);                       
//  return MonoOutput::fromAlmostNBit(11, asig);
}

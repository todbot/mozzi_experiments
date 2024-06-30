/**
 * eighties_dystopia_rp2040.ino --
 *  - A swirling ominous wub that evolves over time
 *  - Tested on Raspberry Pi Pico, PWM audio output on pin GP0
 *  - No user input, just wallow in the sound
 *  
 *  Circuit:
 *  - Copy the PWM cleanup RC filter from schematic figure 3.4.1 "PWM Audio" in
 *     https://datasheets.raspberrypi.com/rp2040/hardware-design-with-rp2040.pdf
 *     also see: https://www.youtube.com/watch?v=rwPTpMuvSXg
 *  - Wire GP0 to input of this circuit, output to TRRS Tip & Ring1
 *  - Become engulfed with dismay in a Cameron-/Carpeter-inspired 1980s dystopia
 *  
 *  Compiling:
 *  - Install Mozzi 2 from Arduino Library Manager
ada *  
 *  Code:
 *  - Knob on A0 controls lowpass filter cutoff frequency
 *  - Knob on A1 controls lowpass filter resonance
 *  - Five detuned oscillators are randomly detuned very second or so
 *  - Every 12.5 seconds, a new note is randomly chosen from the allowed note list
 *  
 * 28 Dec 2021 - @todbot
 * 30 Jun 2024 - @todbot - update to Mozzi 2
 * 
 */
 // Mozzi is very naughty about a few things
//#pragma GCC diagnostic ignored "-Wno-expansion-to-defined"

// set this if you have pots wired up to A0 and A1
//#define USE_KNOBS
 
#include "MozziConfigValues.h"  // for named option values
#define MOZZI_OUTPUT_MODE MOZZI_OUTPUT_PWM
#define MOZZI_ANALOG_READ MOZZI_ANALOG_READ_NONE
#define MOZZI_AUDIO_PIN_1 0  // GPIO pin number
#define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <ResonantFilter.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

#define NUM_VOICES 5

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];

Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

LowPassFilter lpf;
uint8_t resonance = 170; // range 0-255, 255 is most resonant

uint8_t notes[] = {33, 34, 31}; // possible notes to play MIDI A1, A1#, G1
uint16_t note_duration = 13500;
uint8_t note_id = 0;
uint32_t lastMillis = 0;

void setup() {
   
  Serial.begin(115200);
  
  startMozzi();
  kFilterMod.setFreq(0.08f);
  lpf.setCutoffFreqAndResonance(20, resonance);
  for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
  setNotes();
}

void loop() {
  audioHook();
}

void setNotes() {
  float f = mtof(notes[note_id]);
  for(int i=1;i<NUM_VOICES-1;i++) {
    aOscs[i].setFreq( f + (float)rand(100)/100); // orig 1.001, 1.002, 1.004
  }
  aOscs[NUM_VOICES-1].setFreq( (f/2) + (float)rand(100)/1000);
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  #ifdef HAVE_KNOBS
  byte cutoff_freq = analogRead(A0) / 16; 
  resonance = analogRead(A1) / 16; 
  #else
  byte cutoff_freq = 80 + kFilterMod.next()/2;
  #endif
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  
  if(rand(CONTROL_RATE) == 0) { // about once every second
    Serial.println("!");
    Serial.print(cutoff_freq); Serial.print(":"); Serial.print(resonance);
    kFilterMod.setFreq((float)rand(255)/4096);  // choose a new modulation frequency
    setNotes(); // wiggle the tuning a little
  }
  
  if( millis() - lastMillis > note_duration )  {
    lastMillis = millis();
    note_id = rand(3); // (note_id+1) % 3;
    Serial.print("note: "); Serial.println((byte)notes[note_id]);
    if( Serial.available() ) { 
      for(int i=1;i<NUM_VOICES-1;i++) {
        aOscs[i].setFreq( 0 ); // orig 1.001, 1.002, 1.004
      }
    }
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput updateAudio() {
  int16_t asig = (long) 0;
  for( int i=0; i<NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
  asig = lpf.next(asig);
  return MonoOutput::fromAlmostNBit(12, asig);
}

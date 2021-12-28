/**
 * eighties_distopia.ino -- 
 *  - A swirling ominous wub that evolves over time
 *  - Tested on QT Py M0 (audio output pin A0) or Trinket M0 (pin 1)
 *  - No user input, just wallow in the sound
 *  
 *  Circuit:
 *  - Wire a 1k resistor from audio out to Tip & Ring1 of a standard 
 *      TRRS 1/8" (3.5mm) headphone plug
 *  - Plug into portable speaker with good bass response
 *  - Become engulfed with dismay in a Cameron-/Carpeter-inspired 1980s dystopia
 *  
 *  Code:
 *  - Five detuned oscillators are randomly detuned very second or so
 *  - A low-pass filter is slowly modulated over the filters
 *  - The filter modulation rate also changes randomly about every second
 *  - Every 17.5 seconds, a new note is randomly chosen from the allowed note list
 *  
 * 27 Dec 2021 - @todbot
 * 
 */
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc1(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc2(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc3(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc4(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc5(SAW_ANALOGUE512_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

LowPassFilter lpf;
uint8_t resonance = 170; // range 0-255, 255 is most resonant

uint8_t notes[] = {33, 34, 31}; // possible notes to play MIDI A1, A1#, G1
uint16_t note_duration = 17500;
uint8_t note_id = 0;
uint32_t lastMillis = 0;

void setup() {
  Serial.begin(115200); 
  startMozzi();
  kFilterMod.setFreq(0.08f);
  lpf.setCutoffFreqAndResonance(20, resonance);
  setNotes();
}

void loop() {
  audioHook();
}

void setNotes() {
  float f = mtof(notes[note_id]);
  aOsc2.setFreq( f + (float)rand(100)/100); // orig 1.001, 1.002, 1.004
  aOsc3.setFreq( f + (float)rand(100)/100);
  aOsc4.setFreq( f + (float)rand(100)/100);
  aOsc5.setFreq( (f/2) + (float)rand(100)/1000);  
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  byte cutoff_freq = 67 + kFilterMod.next()/2;
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  
  if(rand(CONTROL_RATE) == 0) { // about once every second
    Serial.println("!");
    kFilterMod.setFreq((float)rand(255)/4096);  // choose a new modulation frequency
    setNotes(); // wiggle the tuning a little
  }
  
  if( millis() - lastMillis > note_duration )  {
    lastMillis = millis();
    note_id = rand(3); // (note_id+1) % 3;
    Serial.println((byte)notes[note_id]);
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = lpf.next(aOsc1.next() + 
                       aOsc2.next() + 
                       aOsc3.next() +
                       aOsc4.next() +
                       aOsc5.next()
                       );
  return MonoOutput::fromAlmostNBit(11,asig);
}

/**
 * eighties_chase.ino --
 *  - You're being chased by a mad killer and/or robot and/or both
 *  - You're hiding behind a burned out car, flames cast flickering shadows across your face
 *  - You only have two shotgun shells left
 *  - Now's your chance. Run!
 *  
 *  - Tested on QT Py M0 / Seeed XAIO (audio output pin A0) or Trinket M0 (pin 1)
 *  - Use pot hooked up to pin A1 to control filter cutoff
 *  
 *  Circuit:
 *  - Wire a 1k resistor from audio out to Tip & Ring1 of a standard 
 *      TRRS 1/8" (3.5mm) headphone plug
 *  - Plug into portable speaker with good bass response
 *  - Become engulfed with dismay in a Cameron-/Carpeter-inspired 1980s dystopia
 *  
 *  Code:
 *  - Five detuned oscillators are randomly detuned every second or so
 *  - A low-pass filter is slowly modulated over the filters
 *  - Every "note_duration" milliseconds, a new note is played
 *  - After 8 notes, the next note in "notes" is chosen
 *  
 * 4 Jan 2022 - @todbot
 * 30 Jun 2024 - @todbot - updated for Mozzi 2
 *
 */


#include "MozziConfigValues.h"  // for named option values
#define MOZZI_ANALOG_READ MOZZI_ANALOG_READ_NONE
#define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <ResonantFilter.h>
#include <ADSR.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc1(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc2(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc3(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc4(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc5(SAW_ANALOGUE512_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lpf;

uint8_t resonance = 185; // range 0-255, 255 is most resonant

uint8_t notes[] = {43, 43, 44, 41, }; // possible notes to play MIDI G2, G2#, F2
uint16_t note_duration = 231; // 120 bpm = 250 msec 1/8th note, 231 msec = 130 bpm
uint8_t note_id = 0;
uint32_t lastMillis = 0;

const int pot1Pin = A1; 

void setup() { 
  Serial.begin(115200);
  pinMode(pot1Pin, INPUT);

  startMozzi();
  kFilterMod.setFreq(0.08f);
  envelope.setADLevels(255, 10);
  envelope.setTimes(20, 200, 200, 200 );
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
  aOsc5.setFreq( (f/2) + (float)rand(100)/1000);  // sub osc
}

uint8_t beat_tick = 0;
// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  // byte cutoff_freq = 67 + kFilterMod.next()/2;
  byte cutoff_freq = analogRead(pot1Pin) / 8 ; // we only want 0-127 really
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  // envelope.setADLevels(255, 1 + cutoff_freq); // range 1-128
  envelope.setDecayLevel(1+cutoff_freq);  // 

  envelope.update();
  
  if( millis() - lastMillis > note_duration )  {
    lastMillis = millis();
    beat_tick++;
    if( beat_tick == 8 ) { // every 8th note 
      beat_tick = 0;
      note_id = (note_id+1) % 4;
      Serial.println((byte)notes[note_id]);
      setNotes();      
    }
    envelope.noteOn();
    //envelope.noteOff(); // let the envelope do the work
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput updateAudio() {
  long asig = lpf.next(aOsc1.next() + 
                       aOsc2.next() + 
                       aOsc3.next() +
                       aOsc4.next() +
                       aOsc5.next()
                       );
  return MonoOutput::fromAlmostNBit(19, envelope.next() * asig); // 19 = 13 signal bits + 8 envelope bits 
}

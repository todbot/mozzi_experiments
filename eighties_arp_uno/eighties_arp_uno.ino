/**
 * eighties_arp_uno.ino -- playing with arpeggios on Arduino Uno 
 *  - arp arp arp
 *  - two pots, two buttons, no keys!
 *  - no musical knowledge needed, just plug it, start twisting knobs
 *  - for demo see: https://www.youtube.com/watch?v=Ql72YoCJ8-8
 *  
 *  Circuit:
 *  - Potentiometer knobs on A1 & A2
 *  - Buttons on D3, D4, D5 (D5 optional)
 *  - Audio out on pin 9
 *  - Wire a 1k resistor from Audio Out to Tip & Ring1 of a 
 *     standard TRRS 1/8" (3.5mm) headphone plug
 *     (or better: https://sensorium.github.io/Mozzi/learn/output/#low-pass-audio-filter)
 *  - Plug into portable speaker
 *  
 *  Compiling:
 *  - Bounce2 library - https://github.com/thomasfredericks/Bounce2/
 *  - Mozzi library - https://github.com/sensorium/Mozzi
 *  - Mozzi must you hand-install, others from Library Manager
 *  
 *  Code:
 *  - Knob on A1 controls root note
 *  - Knob on A2 controls bpm
 *  - Button on D3 controls which arp to play
 *  - Button on D4 controls octave count
 *  - Button on D5 changes sounds
 *  
 * 21 Jan 2022 - @todbot
 * 
 */

#define CONTROL_RATE 64 // mozzi rate for updateControl()

#include <Bounce2.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/square_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <ADSR.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

#include "Arpy.h"

const int audioPin =  9; // 9 // audio out assumed by Mozzi
const int knobAPin = A1; // A1
const int knobBPin = A2; // A2
const int butAPin  = 3;  // D6 
const int butBPin  = 4;  // D7 
const int butCPin  = 5;  // D8 "

Oscil <SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc0(SAW_ANALOGUE512_DATA);
Oscil <SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc1(SAW_ANALOGUE512_DATA);

ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lpf;

uint8_t cutoff_freq = 180;
uint8_t resonance = 100; // range 0-255, 255 is most resonant

int bpm = 130;
uint8_t arp_octaves = 1;
uint8_t root_note = 60;
uint8_t patch_num = 0;

Arpy arp = Arpy();

Bounce butA = Bounce();
Bounce butB = Bounce();
Bounce butC = Bounce();

void setup() { 
  Serial.begin(115200);
  Serial.println("\neighties_arp_uno hello!");
  
  pinMode( LED_BUILTIN, OUTPUT);
  pinMode( knobAPin, INPUT);
  pinMode( knobBPin, INPUT);
  butA.attach( butAPin, INPUT_PULLUP);
  butB.attach( butBPin, INPUT_PULLUP);
  butC.attach( butCPin, INPUT_PULLUP);

  arp.setNoteOnHandler(noteOn);
  arp.setNoteOffHandler(noteOff);
  arp.setRootNote( root_note );
  arp.setBPM( bpm );
  arp.setGateTime( 0.75 ); // percentage of bpm
  arp.on();

  setPatch(0);
  startMozzi();

}

void loop() {
  audioHook();
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  
  butA.update();
  butB.update();
  butC.update();
  int knobA = mozziAnalogRead(knobAPin); // value is 0-1023
  int knobB = mozziAnalogRead(knobBPin); // value is 0-1023

  root_note = map( knobA, 0,1023, 24, 72);
  float bpm = map( knobB, 0,1023, 100, 5000 );

  arp.setRootNote( root_note );
  arp.setBPM( bpm );
  
  arp.update();

  if( butA.fell() ) {
    arp.nextArpId();
    Serial.print("--- picking next arp"); Serial.println(arp.getArpId());
  }
  
  if( butB.fell() ) {
    arp_octaves = arp_octaves + 1; if( arp_octaves==4) { arp_octaves=1; }
    arp.setTransposeSteps( arp_octaves );
    Serial.print("--- arp steps:"); Serial.println(arp_octaves);
  }
  
  if( butC.fell() ) {
    patch_num = (patch_num+1) % 2;
    setPatch(patch_num);
    Serial.print("---- patch:"); Serial.println(patch_num);
  }

  envelope.update();
  
}

void noteOn(uint8_t note) {
  Serial.print(" noteON:"); Serial.println((byte)note);
  digitalWrite(LED_BUILTIN, HIGH);
  float f = mtof(note);
  aOsc0.setFreq( f ); 
  aOsc1.setFreq( f + (random(100)/1000) ); 
  envelope.noteOn();
}

void noteOff(uint8_t note) {
  Serial.print("noteOFF:"); Serial.println((byte)note);
  digitalWrite(LED_BUILTIN, LOW);
  envelope.noteOff();
}

void setPatch(uint8_t patchnum) {
  if( patchnum == 0 ) {
    Serial.println("PATCH: saw waves");
    aOsc0.setTable(SAW_ANALOGUE512_DATA);
    aOsc1.setTable(SAW_ANALOGUE512_DATA);
    envelope.setADLevels(255, 255);
    envelope.setTimes(30, 200, 200, 200 );
    cutoff_freq = 120;
    resonance = 100;
    lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  }
  else if( patchnum == 1 ) { 
    Serial.println("PATCH: soft square waves");
    aOsc0.setTable(SQUARE_ANALOGUE512_DATA);
    aOsc1.setTable(SQUARE_ANALOGUE512_DATA);
    envelope.setADLevels(240, 10);
    envelope.setTimes(350, 200, 200, 200);
    cutoff_freq = 127;
    resonance = 220;
    lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = (long) 0;
  asig = aOsc0.next() + aOsc1.next();
  asig = lpf.next(asig);

  return MonoOutput::fromNBit(16, envelope.next() * asig);
}

/**
 * eighties_drums.ino --
 *  
 *  - Tested on QT Py M0 / Seeed XIAO (audio output pin A0) or Trinket M0 (pin 1)
 *  
 *  Circuit:
 *  - Wire a 1k resistor from audio out to Tip & Ring1 of a standard 
 *      TRRS 1/8" (3.5mm) headphone plug
 *  - Plug into portable speaker with good bass response
 *  - Become engulfed with dismay in a Cameron-/Carpeter-inspired 1980s dystopia
 *  
 *  Code:
 *  
 * 18 Jan 2022 - @todbot
 *  
 */
// #define CONTROL_RATE 128

//#include <Adafruit_TinyUSB.h>
#include <MozziGuts.h>
#include <Sample.h> // Sample template
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <ADSR.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

#include "Seqy.h"

#include "d_kit.h"

// pot is connected on pins 3v3, MO, MI.
// MI is fake gnd. MO is wiper
const int dacPin   =  0; // A0 // audio out assumed by Mozzi
const int knobAPin = 10; // D10 = MO 
const int knobBPin =  8; // D8 = SCK
const int GndAPin  =  9; // D9 = MI

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <BD_NUM_CELLS, AUDIO_RATE> aBD(BD_DATA) ;
Sample <SD_NUM_CELLS, AUDIO_RATE> aSD(SD_DATA);
Sample <CH_NUM_CELLS, AUDIO_RATE> aCH(CH_DATA);
Sample <OH_NUM_CELLS, AUDIO_RATE> aOH(OH_DATA);

#define NUM_VOICES 3

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lpf;

Seqy seq = Seqy();

float drum_pitch = 1.0;

uint8_t cutoff_freq = 180;
uint8_t resonance = 100; // range 0-255, 255 is most resonant

void setup() { 
  Serial.begin(115200);
  while(!Serial) delay(1);
  Serial.println("eighties_drums setup");
  
  //pinMode(knobAPin, INPUT);
  //pinMode(pot1GndPin, OUTPUT);
  //digitalWrite(pot1GndPin, LOW);

  seq.setTriggerHandler( triggerDrums );
  seq.setBPM(120);
  
  envelope.setADLevels(255, 10);
  envelope.setTimes(20, 180, 180, 180 );
  lpf.setCutoffFreqAndResonance(20, resonance);
  //setNotes();
  
  set_sample_pitches();

  startMozzi();
 
}

void loop() {
  audioHook();
}

void set_sample_pitches() {
  // set samples to play at the speed it was recorded, pitch-shifted potentially
  aBD.setFreq(((float) D_SAMPLERATE / (float) BD_NUM_CELLS) * drum_pitch);
  aSD.setFreq(((float) D_SAMPLERATE / (float) SD_NUM_CELLS) * drum_pitch);
  aCH.setFreq(((float) D_SAMPLERATE / (float) CH_NUM_CELLS) * drum_pitch);
  aOH.setFreq(((float) D_SAMPLERATE / (float) OH_NUM_CELLS) * drum_pitch);
}

void setNotes() {
//  float f = mtof(notes[note_id]);
//  aOsc2.setFreq( f ); // + (float)rand(100)/100); // orig 1.001, 1.002, 1.004
}

void triggerDrums(bool bd, bool sd, bool ch, bool oh ) {
  Serial.printf("drum: %d %d %d %d\n", bd, sd, ch, oh);
  if( bd ) { aBD.start(); }
  if( sd ) { aSD.start(); }
  if( ch ) { aCH.start(); }
  if( oh ) { aOH.start(); }
}

uint32_t test_millis = 0;

// mozzi function, called every CONTROL_RATE
void updateControl() {
  
  seq.update();

  if( millis() - test_millis > 1000 ) { 
    test_millis = millis();
    seq.setSeqId( random(0, seq.getSeqCount()) );
    Serial.printf("sequence change: %d\n", seq.getSeqId() );
  }
  
//  envelope.update();

}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = 0;
//  long asig = lpf.next(aOsc1.next() +
//                       aOsc2.next()
//                       );
//  asig = (envelope.next() * asig) / 256 / 5;

  asig += aBD.next() + aSD.next() + aOH.next() + aCH.next();
  
  return MonoOutput::fromAlmostNBit(9,(long)asig);
  //return MonoOutput::fromAlmostNBit(11, asig); // 16 = 8 signal bits + 8 envelope bits
}

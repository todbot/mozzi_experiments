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
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()
#include <Bounce2.h>

#include "Seqy.h"

#include "d_kit.h"

const int dacPin   =  0; // A0 // audio out assumed by Mozzi
const int knobAPin =  1; // A1
const int modePin  =  6; // "TX"
const int butBDPin =  7; // "RX"
const int butSDPin =  8; // "SCK"
const int butCHPin =  9; // "MI"
const int butOHPin = 10; // "MO"

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <BD_NUM_CELLS, AUDIO_RATE> aBD(BD_DATA) ;
Sample <SD_NUM_CELLS, AUDIO_RATE> aSD(SD_DATA);
Sample <CH_NUM_CELLS, AUDIO_RATE> aCH(CH_DATA);
Sample <OH_NUM_CELLS, AUDIO_RATE> aOH(OH_DATA);

Bounce butMode = Bounce();
Bounce butBD = Bounce();
Bounce butSD = Bounce();
Bounce butCH = Bounce();
Bounce butOH = Bounce();

Seqy seq = Seqy();

float drum_pitch = 1.0;
bool bd_on = true;
bool sd_on = true;
bool ch_on = true;
bool oh_on = true;
bool direct_play = false;
int knobAval = 0;

void setup() { 
  Serial.begin(115200);
  //while(!Serial) delay(1);
  Serial.println("eighties_drums setup");
  
  pinMode(knobAPin, INPUT);
  butMode.attach( modePin, INPUT_PULLUP);
  butBD.attach( butBDPin, INPUT_PULLUP);
  butSD.attach( butSDPin, INPUT_PULLUP);
  butCH.attach( butCHPin, INPUT_PULLUP);
  butOH.attach( butOHPin, INPUT_PULLUP);
  
  seq.setBeatHandler( handleBeat );
  seq.setTriggerHandler( triggerDrums );
  seq.setBPM(120);
  
  setDrumPitches();

  startMozzi();
 
}

void loop() {
  audioHook();
}

void setDrumPitches() {
  // set samples to play at the speed it was recorded, pitch-shifted potentially
  aBD.setFreq(((float) D_SAMPLERATE / (float) BD_NUM_CELLS) * drum_pitch);
  aSD.setFreq(((float) D_SAMPLERATE / (float) SD_NUM_CELLS) * drum_pitch);
  aCH.setFreq(((float) D_SAMPLERATE / (float) CH_NUM_CELLS) * drum_pitch);
  aOH.setFreq(((float) D_SAMPLERATE / (float) OH_NUM_CELLS) * drum_pitch);
}

// called by Seqy every beat, to trigger drum noises
void triggerDrums(bool bd, bool sd, bool ch, bool oh ) {
  Serial.printf("drum: %d %d %d %d\n", bd, sd, ch, oh);
  if( direct_play ) { return; } // no seq triggers when playing by hand  
  if( bd && bd_on ) { aBD.start(); }
  if( sd && sd_on ) { aSD.start(); }
  if( ch && ch_on ) { aCH.start(); }
  if( oh && oh_on ) { aOH.start(); }
}

// called by Seqy at top of every beat
void handleBeat( uint8_t beatnum) {
  if( beatnum == 0 || beatnum == 8) { 
    seq.setSeqId( random(0, seq.getSeqCount()) );
  }
  Serial.printf("play: %d beat: %2d seq: %2d ", direct_play, beatnum, seq.getSeqId() );
}


// mozzi function, called every CONTROL_RATE
void updateControl() {

  butMode.update();
  butBD.update();
  butSD.update();
  butCH.update();
  butOH.update();

  
  knobAval = analogRead(knobAPin);
  if( butMode.read() == LOW ) { // pressed
    drum_pitch = (knobAval/1023.0)*2 + 0.25; // "map(knobAval, 0,1023, 0.5, 1.5 )"
    setDrumPitches();
  }
  else {
    float bpm = (knobAval/1023.0) * 100 + 50; // "float map(knobAval, 0,1023, 50,100)"
    seq.setBPM(bpm);
    direct_play = ( knobAval < 20 ); // let human play drums by themselves?
  }
  
  if( direct_play ) { 
    if( butBD.fell() ) { aBD.start(); }
    if( butSD.fell() ) { aSD.start(); }
    if( butCH.fell() ) { aCH.start(); }
    if( butOH.fell() ) { aOH.start(); }  
  }
  else {
    // toggle mutes when button pressed
    if( butBD.fell() ) { bd_on = !bd_on; }
    if( butSD.fell() ) { sd_on = !sd_on; }
    if( butCH.fell() ) { ch_on = !ch_on; }
    if( butOH.fell() ) { oh_on = !oh_on; }
  }

  seq.update();
  
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = 0;

  // hihats are too loud, so lower their volume
  asig += aBD.next() + aSD.next() + (aOH.next()/2) + (aCH.next()/2);
  
  return MonoOutput::fromAlmostNBit(9,(long)asig);
}

/**
 * eighties_arp0.ino -- testing arps, see "eighties_arp" for more thought-out example
 *  - arp arp arp
 *  - two pots, two buttons, not keys!
 *  
 *  Circuit:
 *  
 *  Compiling:
 *  
 *  Code:
 *  - Knob on A1 controls root note
 *  - Knob on A2 controls bpm
 *  - Button on D7 ("RX") controls which arp to play
 *  - BUtotn on D6 ("TX") controls...
 *  
 * 12 Jan 2022 - @todbot
 * 
 */
// Mozzi is very naughty about a few things
//#pragma GCC diagnostic ignored "-Wno-expansion-to-defined"
#define CONTROL_RATE 128

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Bounce2.h>

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/square_analogue512_int8.h> // oscillator waveform
#include <tables/triangle_analogue512_int8.h> // oscillator waveform
#include <tables/sin512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <StateVariable.h> // bandpass & highpass filter
#include <ADSR.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

const int dacPin  =  0;  // A0 // audio out assumed by Mozzi
const int knobAPin = 1;  // A1
const int knobBPin = 2;  // A2
const int butBPin = 6;   // D6 "TX"
const int butAPin = 7;   // D7 "RX"

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIusb); // USB MIDI

#define NUM_VOICES 4

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
LowPassFilter lpf;
StateVariable <LOWPASS> svf; // can be LOWPASS, BANDPASS, HIGHPASS or NOTCH

uint8_t cutoff_freq = 180;
uint8_t resonance = 100; // range 0-255, 255 is most resonant

int bpm = 130; 

const uint8_t arp_len = 4; // length of the arps, all are 4
int8_t arps[][arp_len] = { 
  {0, 4, 7, 12},    // major
  {0, 3, 7, 10},    // minor 7th 
  {0, 7, 12,0},     // 7 Power 5th
  {0, 12, 0, -12},  // octaves
  {0, 12, 24, -12}, // octaves 2
  {0, -12, -12, 0}, // octaves 3 (bass)
  {0, 0, 0, 0},     // root
};
uint8_t arp_count = 7;

uint8_t arp_id = 0;  // which arp we using
uint8_t arp_pos = 0; // where in current arp are we

uint8_t root_note = 40;
uint16_t per_beat_millis = 1000 * 60 / bpm;

uint8_t note_played = 0;  // the note we have played, so we can unplay it later
uint16_t note_duration = 0.5 * per_beat_millis; // 75% gate

uint32_t last_beat_millis = 0;

uint8_t patch_num = 0;

Bounce butA = Bounce();
Bounce butB = Bounce();

void setup() {
  // RP2040 defaults to GP0, from https://github.com/pschatzmann/Mozzi/
  // Mozzi.setPin(0,2); // this sets RP2040 GP2
  // Mozzi.setPin(0,16); // this sets RP2040 GP16  // MacroPad
  // Mozzi.setPin(0,16); // this sets RP2040 GP16  // Trinkey QT2040 GP16 
 
  Serial.begin(115200);
  MIDIusb.begin(MIDI_CHANNEL_OMNI);
  MIDIusb.turnThruOff();    // turn off echo

  pinMode( knobAPin, INPUT);
  pinMode( knobBPin, INPUT);
  butA.attach( butAPin, INPUT_PULLUP);
  butB.attach( butBPin, INPUT_PULLUP);
    
  //delay(3000);
  startMozzi();
  setPatch(0);
}

void loop() {
  audioHook();
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  readMIDI();
  
  butA.update();
  butB.update();
  int knobA = analogRead(knobAPin);
  int knobB = analogRead(knobBPin);

  bpm = map( knobB, 0,1023, 100, 1600 );
  per_beat_millis =   1000 * 60 / bpm;
  note_duration = 0.5 * per_beat_millis;
  
  uint8_t root_note_maybe = map( knobA, 0,1023, 24, 84);
  
  if( butA.fell() ) { // pressed
    Serial.println("next arp");
    arp_id = (arp_id + 1) % arp_count;
  }
  if( butB.fell() ) {
    patch_num = (patch_num+1) % 2;
    setPatch(patch_num);
  }
  
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  //lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  envelope.update();
  
  // Serial.printf("knob A:%d b:%d : \t %d %d\n", knobA,knobB, bpm, per_beat_millis);
  if( millis() - last_beat_millis > per_beat_millis )  {
    last_beat_millis = millis();
    arp_pos = (arp_pos+1) % arp_len;
    // only make musical changes at start of a new measure
    if( arp_pos == 0 ) {
      root_note = root_note_maybe;
    }
    note_played = root_note + arps[arp_id][arp_pos];
    // note_off_millis = millis() + note_duration;
    noteOn( note_played );
  }
  
  if( millis() - last_beat_millis > note_duration ) { 
    if( note_played !=0 ) { 
      noteOff( note_played );
      note_played = 0;
    }
  }

}

void noteOn(uint8_t note) {
  Serial.print(" noteON:"); Serial.println((byte)note);
  float f = mtof(note);
  for(int i=1;i<NUM_VOICES-1;i++) {
    aOscs[i].setFreq( f + (float)rand(150)/100); // detune slightly
    //aOscs[i].setPhase(rand(100)); // helps with clipping on summing if they're not all in phase
  }
  envelope.noteOn();
  MIDIusb.sendNoteOn(note, 127, 1); // velocity 127 on chan 1
}

void noteOff(uint8_t note) {
  Serial.print("noteOFF:"); Serial.println((byte)note);
  envelope.noteOff();
  MIDIusb.sendNoteOn(note, 0, 1); // velocity 0 on chan 1
}

void readMIDI() {
  if (MIDIusb.read()) {
    byte mtype = MIDIusb.getType();
    byte data1 = MIDIusb.getData1();
    byte data2 = MIDIusb.getData2();
    byte chan  = MIDIusb.getChannel();
    if( mtype == midi::NoteOn ) {  
      Serial.printf("MIDIusb: c:%d t:%2x data:%2x %2x\n", chan, mtype, data1,data2);
      // we're not using MIDI in yet
    }
  }
}
void setPatch(uint8_t patchnum) {
  if( patchnum == 0 ) {
    for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SAW_ANALOGUE512_DATA);
    }
    kFilterMod.setFreq(0.08f);
    envelope.setADLevels(255, 10);
    envelope.setTimes(20, 500, 500, 200 );
    cutoff_freq = 80;
    resonance = 220;
    lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
    svf.setCentreFreq(cutoff_freq * 32);
    svf.setResonance(resonance);
  }
  else if( patchnum == 1 ) { 
    for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SQUARE_ANALOGUE512_DATA);
    }
    kFilterMod.setFreq(0.08f);
    envelope.setADLevels(255, 10);
    envelope.setTimes(350, 500, 500, 500);
    cutoff_freq = 180;
    resonance = 100;
    lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
    svf.setCentreFreq(cutoff_freq * 32);
    svf.setResonance(resonance);
  }
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = (long) 0;
  for( int i=0; i<NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
//  asig = lpf.next(asig);
  asig = svf.next(asig);
  return MonoOutput::fromNBit(19, envelope.next() * asig); // 16 = 8 signal bits + 8 envelope bits                       
//  return MonoOutput::fromAlmostNBit(12, asig);
}

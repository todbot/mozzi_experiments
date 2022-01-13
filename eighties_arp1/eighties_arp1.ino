/**
 * eighties_dystopia_rp2040.ino --
 *  - A swirling ominous wub that evolves over time
 *  - Tested on Raspberry Pi Pico, audio output on pin GP0
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
//#pragma GCC diagnostic ignored "-Wno-expansion-to-defined"

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Bounce2.h>

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
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
uint8_t cutoff_freq = 100;
uint8_t resonance = 180; // range 0-255, 255 is most resonant

int bpm = 130; 
//uint8_t notes[] = {33, 34, 31}; // possible notes to play MIDI A1, A1#, G1

int8_t arp1[] = {0, 4, 7, 12};   // major
int8_t arp2[] = {0, 3, 7, 10};   // minor 7th
int8_t arp3[] = {0, 12, 0, -12}; // octaves
int8_t arp4[] = {0, 12, 24, -12}; // octaves 2

int8_t* arps[] = { arp1, arp2, arp3, arp4 };
uint8_t arp_count = 4;

uint8_t arp_len = 4; // length of the arps, all are 4
uint8_t arp_id = 0;  // which arp we using
uint8_t arp_pos = 0; // where in current arp are we

uint8_t note_base = 40;
uint16_t per_beat_millis = 1000 * 60 / bpm;

//uint16_t note_duration = 200;
uint32_t last_beat_millis = 0;

Bounce butA = Bounce();
Bounce butB = Bounce();

void setup() {
  // RP2040 defaults to GP0, from https://github.com/pschatzmann/Mozzi/
  // Mozzi.setPin(0,2); // this sets RP2040 GP2
  // Mozzi.setPin(0,16); // this sets RP2040 GP16  // MacroPad
  // Mozzi.setPin(0,16); // this sets RP2040 GP16  // Trinkey QT2040 GP16 
  
  MIDIusb.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(115200);

  MIDIusb.turnThruOff();    // turn off echo

  pinMode( knobAPin, INPUT);
  pinMode( knobBPin, INPUT);
  butA.attach( butAPin, INPUT_PULLUP);
  butB.attach( butBPin, INPUT_PULLUP);
    
  delay(3000);
  startMozzi();
  kFilterMod.setFreq(0.08f);
  envelope.setADLevels(255, 10);
  envelope.setTimes(20, 150, 150, 150 );
  lpf.setCutoffFreqAndResonance(20, resonance);
  for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
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
  
  uint8_t note_base_maybe = map( knobA, 0,1023, 24, 72);
  
  if( butA.fell() ) { // pressed
    Serial.println("next arp");
    arp_id = (arp_id + 1) % arp_count;
  }
  if( butB.fell() ) {
    //Serial.println("clk div");
    //clkdiv_id = (clk_id + 1) % arp_count;
  }
  
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  //byte cutoff_freq = 67 + kFilterMod.next()/2;
  //  byte cutoff_freq = analogRead(A0) / 16; 
  //  byte resonance = analogRead(A1) / 16; 
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);
  envelope.update();
  
  Serial.printf("knob A:%d b:%d : \t %d %d\n", knobA,knobB, bpm, per_beat_millis);
  if( millis() - last_beat_millis > per_beat_millis )  {
    last_beat_millis = millis();
    arp_pos = (arp_pos+1) % arp_len;
    if( arp_pos == 0 ) { // start of a new measure
      note_base = note_base_maybe;    
    }
    noteOn( note_base + arps[arp_id][arp_pos] );
  }

}

void noteOn(byte note) {
  Serial.print("noteOn:"); Serial.println((byte)note);
  float f = mtof(note);
  for(int i=1;i<NUM_VOICES-1;i++) {
    aOscs[i].setFreq( f + (float)rand(100)/100); // detune slightly
  }
  //aOscs[NUM_VOICES-1].setFreq( (f/2) + (float)rand(100)/1000);
  envelope.noteOn();
}

void readMIDI() {
  if (MIDIusb.read()) {
    byte mtype = MIDIusb.getType();
    byte data1 = MIDIusb.getData1();
    byte data2 = MIDIusb.getData2();
    byte chan  = MIDIusb.getChannel();
    if( mtype == midi::NoteOn ) {  
      Serial.printf("c:%d t:%2x data:%2x %2x\n", chan, mtype, data1,data2);
    }
  }  
}


// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  long asig = (long) 0;
  for( int i=0; i<NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
  asig = lpf.next(asig);
  return MonoOutput::fromAlmostNBit(18, envelope.next() * asig); // 16 = 8 signal bits + 8 envelope bits                       
//  return MonoOutput::fromAlmostNBit(12, asig);
}


//  Serial.printf("butt:%d %d knob:%d %d\n", 
//    digitalRead(butAPin), digitalRead(butBPin), analogRead(knobAPin), analogRead(knobBPin));

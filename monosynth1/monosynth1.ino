/**
 * monosynth1 wubwubwub synth using LowPassFilter
 * based MozziScout "mozziscout_monosynth1"
 *
 * Responds to Serial (DIN) MIDI In
 *
 *  @todbot 3 Jan 2021
 **/

#define CONTROL_RATE 512  // Mozzi's controller update rate

#include <MozziGuts.h>
#include <Oscil.h>
//#include <tables/triangle_warm8192_int8.h>
#include <tables/triangle_analogue512_int8.h>
#include <tables/square_analogue512_int8.h>
#include <tables/saw_analogue512_int8.h>
#include <tables/cos2048_int8.h> // for filter modulation
#include <LowPassFilter.h>
#include <ADSR.h>
#include <Portamento.h>
#include <mozzi_midi.h> // for mtof()
#include <MIDI.h>


// SETTINGS
float mod_amount = 0.5;
uint8_t resonance = 187; // range 0-255, 255 is most resonant
uint8_t cutoff = 59;     // range 0-255, corresponds to 0-8192 Hz
int portamento_time = 50;  // milliseconds
int env_release_time = 1000; // milliseconds

enum KnownCCs {
  Modulation=0,
  Resonance,
  FilterCutoff,
  CC_COUNT
};

uint8_t midi_ccs[] = {
  1, // modulation
  71, // resonance
  74, // filter cutoff
};
uint8_t mod_vals[ CC_COUNT ];


// This is a piece of "magic" code that allows us to change the default behaviour of the MIDI library
//struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
//  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
//  static const long BaudRate = 31250;        // Doesn't build without this...
//};
//MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDI, MySettings); // for USB-based SAMD
MIDI_CREATE_DEFAULT_INSTANCE();

//
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc1(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc2(SAW_ANALOGUE512_DATA);

Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA); // filter mod

ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
Portamento <CONTROL_RATE> portamento;
LowPassFilter lpf;

byte startup_mode = 0;
bool retrig_lfo = true;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

//  MIDI.setHandleNoteOn(handleNoteOn);
//  MIDI.setHandleNoteOff(handleNoteOff);
//  MIDI.setHandleControlChange(handleControlChange);
  MIDI.begin(MIDI_CHANNEL_OMNI);   // Initiate MIDI communications, listen to all channels
  
  startMozzi(CONTROL_RATE);

  mod_vals[Modulation] = 64;  // 0.5
  mod_vals[Resonance] = 187; 
  mod_vals[FilterCutoff] = 59;

  lpf.setCutoffFreqAndResonance(mod_vals[FilterCutoff], resonance);
  kFilterMod.setFreq(4.0f);  // fast
  envelope.setADLevels(255, 255);
  envelope.setTimes(100, 200, 20000, env_release_time);
  portamento.setTime( portamento_time );

}

void loop() {
  audioHook();
}

void handleNoteOn(byte channel, byte note, byte velocity) {
//  Serial.println("midi_test handleNoteOn!");
  digitalWrite(LED_BUILTIN,HIGH);
  portamento.start(note);
//  aOsc1.setFreq( mtof(note) ); // old way of direct set, now use portamento
//  aOsc2.setFreq( (float)(mtof(note) * 1.01) );
  envelope.noteOn();
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  digitalWrite(LED_BUILTIN,LOW);
  envelope.noteOff();
}

void handleControlChange(byte channel, byte cc_num, byte cc_val) {
  for( int i=0; i<CC_COUNT; i++) { 
    if( midi_ccs[i] == cc_num ) { // we got one
      mod_vals[i] = cc_val;
//      Serial.printf("CC %d %d\n", cc_num, cc_val);
    }
  }
}

void handleMIDI() {
  while( MIDI.read() ) { 
    switch(MIDI.getType()) {
      case midi::ControlChange:
        handleControlChange(0, MIDI.getData1(), MIDI.getData2());
        break;
      case midi::NoteOn:
        handleNoteOn( 0, MIDI.getData1(),MIDI.getData2());
        break;
      case midi::NoteOff:
        handleNoteOff( 0, MIDI.getData1(),MIDI.getData2());
        break;
      default:
        break;
    }
  }
}

void updateControl() {
  scanKeys();
  handleMIDI();
  // MIDI.read();
  // map the lpf modulation into the filter range (0-255), corresponds with 0-8191Hz
  //uint8_t cutoff_freq = cutoff + (mod_amount * (kFilterMod.next()/2));
  lpf.setCutoffFreqAndResonance(mod_vals[FilterCutoff], resonance);
  envelope.update();
  Q16n16 pf = portamento.next();  // Q16n16 is a fixed-point fraction in 32-bits (16bits . 16bits)
  aOsc1.setFreq_Q16n16(pf);
  aOsc2.setFreq_Q16n16(pf*1.02); // hmm, feels like this shouldn't work

}

AudioOutput_t updateAudio() {
  long asig = lpf.next( aOsc1.next() + aOsc2.next() );
  return MonoOutput::fromAlmostNBit(18, envelope.next() * asig); // 16 = 8 signal bits + 8 envelope bits
}

void scanKeys() {
}

// allow some simple parameter changing based on keys
// pressed on startup
void handleModeChange(byte m) {
  startup_mode = m;
  Serial.print("mode change:"); Serial.println((byte)m);
  if ( startup_mode == 1 ) {      // lowest C held
    kFilterMod.setFreq(0.5f);     // slow
  }
  else if ( startup_mode == 2 ) { // lowest C# held
    kFilterMod.setFreq(0.25f);    // slower
    retrig_lfo = false;
  }
  else if ( startup_mode == 3 ) {  // lowest D held
    kFilterMod.setFreq(0.05f);      // offish, almost no mod
    cutoff = 78;
  }
  else if ( startup_mode == 6 ) {
    aOsc1.setTable(TRIANGLE_ANALOGUE512_DATA);
    aOsc2.setTable(TRIANGLE_ANALOGUE512_DATA);
    cutoff = 65;
  }
  else if ( startup_mode == 7 ) {
    aOsc1.setTable(SQUARE_ANALOGUE512_DATA);
    aOsc2.setTable(SQUARE_ANALOGUE512_DATA);
   //envelope.setADLevels(230, 230); // square's a bit too loud so it clips
   resonance = 130;
  }
  else {                          // no keys held
    kFilterMod.setFreq(4.0f);     // fast
  }
}

/**
 *  derpnote.ino -- sorta like a THX "Deep Note" sound
 *
 *  @todbot 12 Dec 2021
 **/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/triangle_analogue512_int8.h>
#include <tables/square_analogue512_int8.h>
#include <mozzi_rand.h> // for rand()
#include <mozzi_midi.h> // for mtof()
#include <ADSR.h>
#include <Portamento.h>

// SETTINGS
#define NUM_VOICES 16
int octave = 4;
int chord_notes[] = {0, 5, 12, 19,  -12, 7, 9, 12, 
                     0, 5, 7, 12,   -24, -7, 7, 12 }; // FIXME
byte note_offset = (octave * 12);

// oscillators
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];

// envelope to fade in & out nicer
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

// portamentos to slide the pitch around
Portamento <CONTROL_RATE> portamentos[NUM_VOICES];

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  envelope.setADLevels(255, 255);
  envelope.setTimes(300, 200, 20000, 1000);
  for ( int i = 0; i < NUM_VOICES; i++) {
    aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
  startMozzi(); // start with default control rate of 64
  
  initDerpNote();
  
  triggerDerpNote();
}

//
void loop() {
  audioHook(); // required here, don't put anything else in loop
}

// set all the oscs to random pitches away from mid point
void initDerpNote() {
  for ( int i = 0; i < NUM_VOICES; i++) {
    portamentos[i].setTime(1000u + i * 800u); // notes later in chord_notes take longer
    portamentos[i].start((byte)(note_offset + random(-24, 48))); 
  }
}

// start up the portamentos to slide towards destination pitches
void triggerDerpNote() {
  // uint8_t note = 48; // FIXME
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < NUM_VOICES; i++) {
    // "normal version that makes sense"
    //portamentos[i].start((byte)(note + chord_notes[i]));
    // "harder version that adds slight randomness to pitches"
    uint8_t note = chord_notes[ i ]; // chord_notes must be NUM_VOICES big
    portamentos[i].start(Q8n0_to_Q16n16(note_offset + note) + Q7n8_to_Q15n16(rand(150)));
  }
  envelope.noteOn();
}

// Mozzi function to update control-rate things like LFOs & envelopes
void updateControl() {
  
  envelope.update();
  
  // slide the oscs using the portamentos
  for ( int i = 0; i < NUM_VOICES; i++) {
    aOscs[i].setFreq_Q16n16(portamentos[i].next());
  }
}

// Mozzi function to update audio
AudioOutput_t updateAudio() {
  long asig = (long) 0;
  for ( int i = 0; i < NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
  // 19 = 8 bits signal * 8 bits envelope = 16 bits + exp(NUM_VOICES,1/2)
  return MonoOutput::fromNBit(20, envelope.next() * asig);
}

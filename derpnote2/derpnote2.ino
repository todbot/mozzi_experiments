/**
 *  derpnote.ino -- sorta like a THX "Deep Note" sound
 *
 *  @todbot 16 Dec 2021
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

// F#7  D7  A6  D6  A5  D5  A4  D4  A3  D3  D2
// 102  98  93  86  81  74  69  62  57  50  38
// F#3  D3  A4  D4  A3  D1  A2  D2  A1  D1  D2
//  54  50  69  62  57  26  45  38  33  26  38

int note_start = 38;
int notes_end[]  = {86, 93, 86, 93,  81, 74, 69, 57, 
                     33, 26, 38, 54,  57, 26, 45, 38, };
// oscillators
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];

// envelope to fade in & out nicer
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

// portamentos to slide the pitch around
Portamento <CONTROL_RATE> portamentos[NUM_VOICES];

enum DerpStates  {
  STATE_START,
  STATE_SLIDEA,
  STATE_SLIDEB,
  STATE_END,
};

uint8_t state = STATE_START;
uint16_t state_duration = 0;
uint32_t state_time_last = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  // while(!Serial); Serial.println("derpnote2 setup"); // for testing
  
  envelope.setADLevels(255, 255);
  envelope.setTimes(300, 200, 20000, 1000);
  for ( int i = 0; i < NUM_VOICES; i++) {
    aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
  startMozzi(); // start with default control rate of 64
}

//
void loop() {
  audioHook(); // required here, don't put anything else in loop
}

void handleDeepNoting() {
  
  if( millis() - state_time_last < state_duration ) {
     return; // not time yet
  }
  
  state_time_last = millis();

  if( state == STATE_START ) { 
    Serial.println("State: START");
    for ( int i = 0; i < NUM_VOICES; i++) {
      portamentos[i].start(Q8n0_to_Q16n16(note_start) + Q7n8_to_Q15n16(rand(1000)));
      portamentos[i].setTime(3000 + random(-500,500));
    }
    envelope.noteOn();
    state_duration = 3000;
    state = STATE_SLIDEA;
  }
  else if( state == STATE_SLIDEA ) {
    Serial.println("State: SLIDEA");
    for ( int i = 0; i < NUM_VOICES; i++) {
      portamentos[i].setTime(2000 + random(-500,1000));
      portamentos[i].start(Q8n0_to_Q16n16(note_start) + Q7n8_to_Q15n16(rand(3000)));
    }
    state_duration = 5000;
    state = STATE_SLIDEB;
  }
  else if( state == STATE_SLIDEB ) { 
    Serial.println("State: SLIDEB");
    for ( int i = 0; i < NUM_VOICES; i++) {
      portamentos[i].setTime(6000u + (NUM_VOICES-i) * 200u); // notes earlier in chord_notes take longer
      portamentos[i].start( Q8n0_to_Q16n16(notes_end[i]) + Q7n8_to_Q15n16(rand(100)));
    }
    state_duration = 8000;
    state = STATE_END;
  }
  else { 
    Serial.println("State: END");
    state_duration = 10000;
    state = STATE_END;
  }
}

// Mozzi function to update control-rate things like LFOs & envelopes
void updateControl() {
  
  handleDeepNoting();
    
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

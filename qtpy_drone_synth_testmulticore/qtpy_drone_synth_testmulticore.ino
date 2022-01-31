/**
 * qtpy_drone_synth_testmulticore.ino  - 
 *    Test out doing multicore stuff with Mozzi on QTPy RP2040
 *    We can do all the I2C on core1, leaving core0 for Mozzi synth
 * 26 Jan 2022 - @todbot 
 */
 
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

#include <Wire.h>
#include <Adafruit_seesaw.h>

#define NUM_KNOBS 4
#define NUM_VOICES NUM_KNOBS

Adafruit_seesaw ss( &Wire1 );

// seesaw on Attiny8x7, analog in can be 0-3, 6, 7, 18-20
uint8_t seesaw_knob_pins[ NUM_KNOBS ] = {0,1,2,3}; //, 6,7,18,19};
uint8_t seesaw_knob_i = 0;
float knob_smoothing = 0.5; // 1.0 = all new value
int knob_vals[ NUM_KNOBS ];

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOscs [NUM_VOICES];
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

LowPassFilter lpf;
uint8_t resonance = 120; // range 0-255, 255 is most resonant


// Running on core1
void setup1() {
  setupKnobs();
}
uint32_t knobUpdateMillis = 0;
// Runnong on core1
void loop1() {
  if( millis() - knobUpdateMillis > 10 ) { 
    knobUpdateMillis = millis();
    readKnobs();
  }
}

// 
void setup() {
  // RP2040 defaults to GP0, from https://github.com/pschatzmann/Mozzi/
  // Mozzi.setPin(0,16); // RP2040 GP16 / Trinkey QT2040 GP16 
  // Mozzi.setPin(0,20); // RP2040 GP20 / QT Py RP2040"RX"
  Mozzi.setPin(0,29);  // this sets RP2040 GP29 / QT Py RP2040 "A0"
  
  Serial.begin(115200);

  while (!Serial) delay(10);   // wait until serial port is opened

  startMozzi();
  
  kFilterMod.setFreq(0.08f);
  lpf.setCutoffFreqAndResonance(20, resonance);
  for( int i=0; i<NUM_VOICES; i++) { 
     aOscs[i].setTable(SAW_ANALOGUE512_DATA);
  }
  
  Serial.println("qtpy_drone_synth_testmulticore started");
}

//
void loop() {
  audioHook();
}

void setupKnobs() {
  if(!ss.begin()){
    Serial.println(F("seesaw not found!"));
    while(1) delay(10);
  }
}

//
  // i2c transactions take time, so only do one at a time
void readKnobs() {
  // get a reading
  int val = ss.analogRead( seesaw_knob_pins[seesaw_knob_i] );

  // smooth it based on last val
  int last_val = knob_vals[ seesaw_knob_i ];
  val = val + (knob_smoothing * (last_val - val));
 
  // save the (smoothed) reading 
  knob_vals[ seesaw_knob_i ] = val;
  
  // prepare for next knob to be read
  seesaw_knob_i = (seesaw_knob_i + 1) % NUM_KNOBS ;
  // debug
  //Serial.printf("    readKnobs:%d %d %d\n", seesaw_knob_i, knob_vals[0], knob_vals[1]);
}

float f_low = 40;
float f_high = 4000;
//
void setOscs() {
  for(int i=0;i<NUM_VOICES;i++) {
//    float f = f_low + (f_high * (knob_vals[i] / 1023));
    aOscs[i].setFreq( knob_vals[i] );
  }
}

uint32_t lastDebugMillis=0; // debug
// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127

  setOscs();

  // debug 
  if( millis() - lastDebugMillis > 100 ) {
    lastDebugMillis = millis();
    for( int i=0; i< NUM_KNOBS; i++) { 
      Serial.printf("%4d ", knob_vals[i]);
    }
    Serial.println();
  }
  
}

// mozzi function, called every AUDIO_RATE to output sample
AudioOutput_t updateAudio() {
  int16_t asig = (long) 0;
  for( int i=0; i<NUM_VOICES; i++) {
    asig += aOscs[i].next();
  }
  asig = lpf.next(asig);
  return MonoOutput::fromAlmostNBit(11, asig);
}

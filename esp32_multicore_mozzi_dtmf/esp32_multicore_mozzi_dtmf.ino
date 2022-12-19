/*********************************************************************
 * ESP32 Multicore Mozzi
 * 
 * 16 Dec 2022 - @todbot / Tod Kurt
 * 
 * Hardware used in this test:
 * - QTPy ESP32 or other ESP32 (not ESP32-S2 or ESP32-C3 (single-core), maybe ESP32-S3)
 * - 1.3" SH1106 I2C OLED on pins QTPy SDA & SCL 
 * - PCM5102 I2S audio DAC on pins MOSI=DIN, MISO=PCM5102 LCK, SCK=PCM5102 BCK 
 * - Potentiometer (10k) on pin A3
 * 
 * Mozzi config:
 * - Edit "Mozzi/AudioConfigESP32.h" to have:
 *     #define ESP32_AUDIO_OUT_MODE PT8211_DAC  // (any I2S not just PT8211 it seems)
 *   and:
 *     #define ESP32_I2S_BCK_PIN 14  // QTPy SCK
 *     #define ESP32_I2S_WS_PIN 12   // QTPY MISO
 *     #define ESP32_I2S_DATA_PIN 13 // QTPy MOSI
 * 
 * FreeRTOS task config
 * - The default settings for Arduino ESP32 in "Tools" menu are:
 *     - Arduino Runs on: "Core 1"
 *     - Events Runs on: "Core 1"
 *  - This means Mozzi runs on Core 1. So we will run our FreeRTOS UI tasks on Core 0
 *  
 * Notes for myself:
 * - https://techtutorialsx.com/2017/05/09/esp32-running-code-on-a-specific-core/
 * - https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
 * - https://techtutorialsx.com/2017/09/13/esp32-arduino-communication-between-tasks-using-freertos-queues/
 * 
 *********************************************************************/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator

// our audio oscillators
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aOsc1(SIN2048_DATA); 
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aOsc2(SIN2048_DATA); 

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

#define oled_i2c_addr 0x3C //or 0x3D for some displays
#define DW 128 // OLED display width, in pixels
#define DH 64 // OLED display height, in pixels
Adafruit_SH1106G display = Adafruit_SH1106G(DW, DH, &Wire, -1);

// forward declaration of our two tasks (not counding the "loop()" task)
void TaskUpdateDisplay( void* pvParameter );
void TaskReadControls (void* pvParameter );

int osc1freq = 0;
int osc2freq = 0;
int digit = 0;

void setup() {

  Serial.begin(115200);

  setupDisplay();
  
  // Set up two tasks to run independently, in addition to our normal "loop()" task
  // For this example we don't *need* two tasks, but it's kinda fun to make them
  // so let's have three tasks: read controls, update display, run Mozzi ("loop")
  xTaskCreatePinnedToCore(
    TaskUpdateDisplay
    ,  "TaskUpdateDisplay"   // A name just for humans
    ,  8192  // stack size, must be big enough or chip crashes, can be adjusted by reading the Stack Highwater
    ,  NULL
    ,  2     // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  0);   // which core to run on, "loop()" runs on core1, so lets run on core0

  xTaskCreatePinnedToCore(
    TaskReadControls
    ,  "ReadControls"  // name for humans 
    ,  8192 // stack size
    ,  NULL
    ,  1    // priority
    ,  NULL 
    ,  0);  // which core to run on, "loop()" runs on core1, so lets run on core0


  startMozzi(CONTROL_RATE); // start Mozzi with given control rate
  aOsc1.setFreq(osc1freq); // set initial frequency
  aOsc2.setFreq(osc2freq); // set initial frequency
  
}

void updateDTMF(int digit) {
  switch(digit) {
    case 1: osc1freq = 1209; osc2freq = 697; break;
    case 2: osc1freq = 1336; osc2freq = 697; break;
    case 3: osc1freq = 1447; osc2freq = 697; break;
    case 4: osc1freq = 1209; osc2freq = 770; break;
    case 5: osc1freq = 1336; osc2freq = 770; break;
    case 6: osc1freq = 1447; osc2freq = 770; break;
    case 7: osc1freq = 1209; osc2freq = 852; break;
    case 8: osc1freq = 1336; osc2freq = 852; break;
    case 9: osc1freq = 1447; osc2freq = 852; break;
    case 0: osc1freq = 1336; osc2freq = 941; break;
    case -1: osc1freq = 350; osc2freq = 440; break;
  }
 
}

// loop task runs on core1 on ESP32
void loop() {
  audioHook();  // required here by Mozzi
}

// 
void setupDisplay() {
  delay(100); // wait for the OLED to power up
  Serial.println("setting up OLED");
  display.begin(oled_i2c_addr, true); // Address 0x3C default
  display.setContrast(0); // dim display
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.printf("ESP32\n  Multicore Mozzi!\n");
  display.display();
  delay(500); // show off
}

// Mozzi function, called CONTROL_RATE times per second
void updateControl() {
  // put changing controls in here
   aOsc1.setFreq(osc1freq); // set the frequency 
   aOsc2.setFreq(osc2freq); // set the frequency 
}

// Mozzi function, called AUDIO_RATE times per second
AudioOutput_t updateAudio() {
  return MonoOutput::fromAlmostNBit(10, aOsc1.next() + aOsc2.next()); // return an int signal centred around 0
}

int sensorValueA3;
// a FreeRTOS task to read our UI
void TaskReadControls(void *pvParameters)  // This is a task.
{
  (void) pvParameters; // unused
  for (;;) {
    // read the input on analog pin A3:
    int newSensorValueA3 = analogRead(A3);
    digit = map(newSensorValueA3, 100,4095, 0,9);
    if( newSensorValueA3 == 0 ) { digit = -1; }
    updateDTMF(digit);
    // a little debugging
    Serial.printf("AnalogRead: core %d knobval=%d digit=%d\n",xPortGetCoreID(), newSensorValueA3, digit);
    delay(20);
  }
}

// a FreeRTOS task to update the display
void TaskUpdateDisplay( void* pvParameters )
{ 
  (void) pvParameters; // unused  
  for(;;) { 
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    if(digit==-1) {
      display.printf("DTMF: dial\n");
    } else {
      display.printf("DTMF: %d\n", digit);
    }
    display.printf("o1:%4d Hz\n", osc1freq);
    display.printf("o2:%4d Hz\n", osc2freq);
    display.display();
    delay(50);
  }
}

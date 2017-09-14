/* Digital Thermometer 
 * 
 * v0.1 bare-bones setting up the flow
 * v0.2 added thermistor and button code
 * v0.3 added shift-register and 4x7seg display code
 * v0.4 optimized a bit
 * 
 * CC-BY Roy Dybing October 2016
 * 
 * small weekend project inspired 
 * by this reddit thread:
 * https://www.reddit.com/r/arduino/comments/58g765/can_you_help_me_reading_this_internal_circuit/
 * 
 * BOM:
 * 1x arduino UNO
 * 1x med size breadboard
 * 1x 4x 7seg CA LED display 
 * 1x 74hc595 Shift Register
 * 1x Thermistor 10k MF52-103 3435 
 * 1x 10k resistor
 * 8x 1k resistor
 * 
 * link to wiring schematics:
 * v0.2: http://i.imgur.com/vKCjdBI.jpg
 * v0.3: http://i.imgur.com/k8kgzej.jpg
 */

// *************** Imports ***************

#include <math.h>;
#include <Bounce2.h>

// *************** Constants ***************

// setting up GPIO pins
#define thermistorPin A0
#define buttonPin      2
#define srData         7
#define srLatch        6
#define srClock        5
#define startCA        8

/* map of LED segments
 *  
 *     -------
 *    |   A   |
 *   F|       |B
 *    |---G---|
 *   E|       |C
 *    |   D   |
 *     -------  *P
 */
const byte ledBits[12]{
//ABCDEFGP
 B11111100,   // 0
 B01100000,   // 1
 B11011010,   // 2
 B11110010,   // 3
 B01100110,   // 4
 B10110110,   // 5
 B00111110,   // 6
 B11100000,   // 7
 B11111110,   // 8
 B11100110,   // 9
 B10011100,   // C
 B10001110    // F
 };

// actual measured value of 10k resistor
// used on the GND side of the 
// resistor -> thermistor voltage divider
const int measuredRes = 9960; // 9.96k on my multimeter

// *************** Datatypes ***************

typedef struct state_t{
  bool tempMode;
};

typedef struct timer_t{
  uint32_t oldTime;
  uint32_t newTime;
  uint32_t intervalMS;
};

typedef struct temp_t{
  int outTemp;
  int centTemp;
  byte tempArray[4];
};

// global pointers and references

Bounce button = Bounce();

// *************** Main setup & loop ***************

void setup(){   
  pinMode(buttonPin,  INPUT_PULLUP);
  pinMode(srData, OUTPUT);
  pinMode(srLatch, OUTPUT);
  pinMode(srClock, OUTPUT);
  for(int i = 0; i < 4; i++){
    pinMode(startCA + i, OUTPUT);
  } 
  button.attach(buttonPin);  
  int greenLEDfade = 0.0; 
}

void loop(){
  timer_t timer;
  state_t state;
  temp_t temp;

  intializeStructs(timer, state, temp);
  fillTempArray(temp, state);

  while(true){
    // check inputs
    if(getButton()){
      changeTempMode(state);
      convertTemp(temp, state);
      fillTempArray(temp, state);
    }
    if(checkTime(timer)){
        temp.centTemp = getTemperature(analogRead(thermistorPin));
        fillTempArray(temp, state);
    }
    // get output in centigrade or fahrenheit
    convertTemp(temp, state);
    // push it out to the 4x7Seg LED
    drawLED(temp);
  }       
}

// *************** Functions ***************

// intialize structs - run once
void intializeStructs(timer_t &t, state_t &s, temp_t &tp){
  t.oldTime = millis();
  t.intervalMS = 500;
  s.tempMode = true;
  tp.centTemp = getTemperature(analogRead(thermistorPin));
  tp.outTemp = tp.centTemp;
}

// get new temperature reading
int getTemperature(int thermRead){
  float res;
  float temp;
  // some math to get temperature from voltage in on thermistorPin...
  res = float(measuredRes) * ((1024.0 / float(thermRead)) - 1.0);
  temp = log(res);
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp -= 273.15;
  return round(temp);
}

// get if it is time to poll the temp sensor
bool checkTime(timer_t &t){
  t.newTime = millis();
  if (t.newTime > (t.oldTime + t.intervalMS)){
    t.oldTime = t.newTime;
    return true;
  } else {
    return false;
  }
}

// get if button have been pressed
bool getButton(){
  if(button.update() && button.read() == LOW){    
    return true;
  } else {
    return false;
  }
}

// change mode between Centigrade and Fahrenheit
void changeTempMode(state_t &s){
  s.tempMode = !s.tempMode;
}

// switch output between centigrade and fahrenheit
void convertTemp(temp_t &t, state_t &s){
  // centigrade if tempMode is true...
  if(s.tempMode){
    t.outTemp = t.centTemp;
  // ...and fahrenheit if false
  } else {
    t.outTemp = round((t.centTemp * 9) / 5) + 32;
  }
}

// output to the 4x7Seg LED display
void drawLED(temp_t &t){
  byte ledBit;
  // cycle through anode 0 through 3 
  for(int i = 0; i < 4; i++){
    int digit = t.tempArray[i];
    // flip all bits since using CA 7seg
    // delete '^ 0xff' if using CC 7seg
    ledBit = ledBits[digit] ^ 0xff;
    digitalWrite(startCA + i, HIGH);
    digitalWrite(srLatch, LOW);
    shiftOut(srData, srClock, LSBFIRST, ledBit);
    // small pause to prevent led-bleed
    delayMicroseconds(250);  
    digitalWrite(srLatch, HIGH);
    digitalWrite(startCA + i, LOW);
  }
}

// split the temperature into discrete digits 
// and add a 'C' or 'F' depending on mode
void fillTempArray(temp_t &t, state_t &s){
  for(int i = 0; i < 4; i++){
    if(i == 0){
      if(s.tempMode == 1){
        // display a 'C'
        t.tempArray[i] = 10;
      } else {
        // display a 'F'
        t.tempArray[i] = 11;
      }
    } else {
      t.tempArray[i] = getDigit(t.outTemp, i - 1);
    }
  }
}

// get a digit from a number at position pos
int getDigit (int number, int pos){
  return (pos == 0) ? number % 10 : getDigit (number/10, --pos);
}

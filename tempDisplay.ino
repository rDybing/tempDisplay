/* Digital Thermometer 
 * 
 * v0.1 bare-bones setting up the flow
 * v0.2 added thermistor and button code
 * v0.3 added shift-register and 4x7seg display code
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
 *     -------  * DP
 *
 *A, B, C, D, E, F, G, DP */
#define ledBitmap { \
 {1, 1, 1, 1, 1, 1, 0, 0},\   // 0
 {0, 1, 1, 0, 0, 0, 0, 0},\   // 1
 {1, 1, 0, 1, 1, 0, 1, 0},\   // 2
 {1, 1, 1, 1, 0, 0, 1, 0},\   // 3
 {0, 1, 1, 0, 0, 1, 1, 0},\   // 4
 {1, 0, 1, 1, 0, 1, 1, 0},\   // 5
 {0, 0, 1, 1, 1, 1, 1, 0},\   // 6
 {1, 1, 1, 0, 0, 0, 0, 0},\   // 7
 {1, 1, 1, 1, 1, 1, 1, 0},\   // 8
 {1, 1, 1, 0, 0, 1, 1, 0},\   // 9
 {1, 0, 0, 1, 1, 1, 0, 0},\   // C
 {1, 0, 0, 0, 1, 1, 1, 0}\    // F
 }

// assign led bitmap to a 2D array
const byte ledBits[12][8] = ledBitmap;

// actual measured value of 10k resistor
// used on the GND side of the 
// resistor -> thermistor voltage divider
const int measuredRes = 9960; // 9.96k on my multimeter

// *************** Datatypes ***************

typedef struct stateStruct{
  bool tempRefresh;
  bool tempMode;
} state;

typedef struct timeStruct{
  uint32_t oldTime;
  uint32_t newTime;
  uint32_t intervalMS;
} timer;

typedef struct tempStruct{
  int16_t outTemp;
  int16_t centTemp;
  int8_t tempArray[4];
} temp;

// global pointers and references
timer timeGet;
timer *timeSet = &timeGet;
state stateGet;
state *stateSet = &stateGet;
temp tempGet;
temp *tempSet = &tempGet;

Bounce button = Bounce();

// *************** Functions ***************

// intialize structs - run once
void intializeStructs(timer *t, state *s, temp *tp){
  t->oldTime = millis();
  t->intervalMS = 500;
  s->tempRefresh = false;
  s->tempMode = true;
  tp->centTemp = getTemperature(analogRead(thermistorPin));
  tp->outTemp = tempGet.centTemp;
  fillTempArray(tempSet);
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
bool checkTime(timer *t){
  t->newTime = millis();
  if (timeGet.newTime > (timeGet.oldTime + timeGet.intervalMS)){
    t->oldTime = millis();
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
void changeTempMode(state *s){
  s->tempMode = !stateGet.tempMode;
}

// switch output between centigrade and fahrenheit
void convertTemp(temp *t){
  // centigrade if tempMode is true...
  if(stateGet.tempMode){
    t->outTemp = tempGet.centTemp;
  // ...and fahrenheit if false
  } else {
    t->outTemp = round((tempGet.centTemp * 9) / 5) + 32;
  }
}

// output to the 4x7Seg LED display
void drawLED(){
  // cycle through anode 0 through 3 
  for(int i = 0; i < 4; i++){
    int digit = tempGet.tempArray[i];
    digitalWrite(startCA + i, HIGH);
    // bang out bits for each digit
    digitalWrite(srLatch, LOW);
    for (int j = 7; j >= 0; j--) {
      digitalWrite(srClock, LOW);
      digitalWrite(srData, !ledBits[digit][j]);
      digitalWrite(srClock, HIGH);
      // small pause to prevent led-bleed
      delayMicroseconds(250);
    }  
    digitalWrite(srLatch, HIGH);
    digitalWrite(startCA + i, LOW);
  }
}

// split the temperature into discrete digits 
// and add a 'C' or 'F' depending on mode
void fillTempArray(temp *t){
  for(int i = 0; i < 4; i++){
    if(i == 0){
      if(stateGet.tempMode == 1){
        // display a 'C'
        t->tempArray[i] = 10;
      } else {
        // display a 'F'
        t->tempArray[i] = 11;
      }
    } else {
      t->tempArray[i] = getDigit(tempGet.outTemp, i - 1);
    }
  }
}
// get a digit from a number at position pos
int getDigit (int number, int pos){
  return (pos == 0) ? number % 10 : getDigit (number/10, --pos);
}

// *************** Main setup & loop ***************

void setup() {   
  pinMode(buttonPin,  INPUT_PULLUP);
  pinMode(srData, OUTPUT);
  pinMode(srLatch, OUTPUT);
  pinMode(srClock, OUTPUT);
  for(int i = 0; i < 4; i++){
    pinMode(startCA + i, OUTPUT);
  } 
  button.attach(buttonPin);  
  intializeStructs(timeSet, stateSet, tempSet);
}

void loop() {
  
  // check inputs
  if(getButton()){
    changeTempMode(stateSet);
    convertTemp(tempSet);
    fillTempArray(tempSet);
  }
  if(checkTime(timeSet)){
      tempSet->centTemp = getTemperature(analogRead(thermistorPin));
      fillTempArray(tempSet);
  }
  // get output in centigrade or fahrenheit
  convertTemp(tempSet);
  // push it out to the 4x7Seg LED
  drawLED();       
}

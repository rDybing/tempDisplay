/* Digital Thermometer 
 * 
 * v0.1 bare-bones setting up the flow
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
 * 1x 4x 7seg LED display
 * 1x 74hc595 Shift Register
 * 1x Thermistor 10k MF52-103 3435 
 * 1x 10k resistor
 * 8x 1k resistor
 * 
 * link to wiring schematics:
 * (TBA)
 */

// *************** Imports ***************

#include <math.h>;

// *************** Constants ***************

// todo: Setting up IO adresses

// *************** Datatypes ***************

typedef struct stateStruct{
  bool tempRefresh;
  bool tempMode;
} state;

typedef struct timeStruct{
  unsigned long oldTime;
  unsigned long newTime;
  unsigned long intervalMS;
} timer;

typedef struct tempStruct{
  int outTemp;
  int centTemp;
} temp;

// global pointers and references
timer timeGet;
timer *timeSet = &timeGet;
state stateGet;
state *stateSet = &stateGet;
temp tempGet;
temp *tempSet = &tempGet;

// *************** Functions ***************

// intialize structs - run once
void intializeStructs(timer *t, state *s, temp *tp){
  t->oldTime = millis();
  t->intervalMS = 2000;
  s->tempRefresh = false;
  s->tempMode = false;
  tp->outTemp = 0;
  tp->centTemp = 0;
}

// get new temperature reading
void getTemperature(){
  //do stuff

  // debug
  Serial.println("getTemperature called");
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
  // do stuff
  return false;
}

// change between Centigrade and Fahrenheit
void changeTempMode(state *s){
  s->tempMode = !stateGet.tempMode;
}

// switch between centigrade and fahrenheit
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
  //do stuff
}

// *************** Main setup & loop ***************

void setup() {
  Serial.begin(9600); // debug only
  intializeStructs(timeSet, stateSet, tempSet);
}

void loop() {
  // check inputs
  if(checkTime(timeSet)){
    getTemperature();
  }
  if(getButton()){
    changeTempMode(stateSet);
  }
  // get output in centigrade or fahrenheit
  convertTemp(tempSet);
  // push it out to the 4x7Seg LED
  drawLED();      
}

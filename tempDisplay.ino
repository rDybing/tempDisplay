/* Digital Thermometer 
 * 
 * v0.1 bare-bones setting up the flow
 * v0.2 added thermistor and button code
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
 * v0.2: http://i.imgur.com/vKCjdBI.jpg
 */

// *************** Imports ***************

#include <math.h>;
#include <Bounce2.h>

// *************** Constants ***************

// todo: Setting up IO adresses
# define thermistorPin  A0
# define buttonPin      2

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
  t->intervalMS = 2000;
  s->tempRefresh = false;
  s->tempMode = true;
  tp->outTemp = 0;
  tp->centTemp = 0;
}

// get new temperature reading
int getTemperature(int thermRead){
  float res;
  float temp;
  // some math to get temperature from voltage in on thermistorPin...
  res = float(measuredRes) * ((1024.0 / float(thermRead)) - 1.0);
  temp = log(res);
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
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
  //do stuff
  Serial.println(tempGet.outTemp); 
}

// *************** Main setup & loop ***************

void setup() {
  Serial.begin(9600); // debug only  
  pinMode(buttonPin, INPUT_PULLUP);
  button.attach(buttonPin);  
  intializeStructs(timeSet, stateSet, tempSet);
}

void loop() {
  // check inputs
  if(checkTime(timeSet)){
      tempSet->centTemp = getTemperature(analogRead(thermistorPin));
  }
  if(getButton()){
    changeTempMode(stateSet);
  }
  // get output in centigrade or fahrenheit
  convertTemp(tempSet);
  // push it out to the 4x7Seg LED
  drawLED();      
}

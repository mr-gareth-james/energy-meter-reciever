/* Arduino code that recieves raw data from the base unit/energy sensor
- Recieves raw data
- Saves high and low averages to understand top speed and slow speed
- Animates the leds
*/


// EPROM
#include <EEPROM.h>

//RADIO Include needed Libraries at beginning
#include "nRF24L01.h" // NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include "RF24.h"
#include "SPI.h"
float ReceivedMessage[1] = {000}; // Used to store value received by the NRF24L01
RF24 radio(9,10); // NRF24L01 used SPI pins + Pin 9 and 10 on the UNO
const uint64_t pipe = 0xE6E6E6E6E6E6; // Needs to be the same for communicating between 2 NRF24L01 

// fast led
#include <FastLED.h>
#define NUM_LEDS 105
#define DATA_PIN 8
CRGB leds[NUM_LEDS]; // memory block for leds

// frame blending animation 
#define ANIM_FPS 255
// the number of steps you want between each frame - should be a power of 2 to keep things sane - e.g. 2,4,8,16,32,64
int STEPS = 2;
#define SCALE_PER_STEP (256 / STEPS) 
CRGB old_frame[NUM_LEDS];
CRGB next_frame[NUM_LEDS];
static uint8_t startIndex = 0;

float myTweenTimer = 0.00;

// data setup
float sensorMin = 100000;
float sensorMax = 100000;
float SensorMinNumbers[] = {100,100,100,100,100,100,100,100,100,100};
float SensorMaxNumbers[] = {0,0,0,0,0,0,0,0,0,0};
int energyPercentage = 0;
int oldEnergyPercentage=0;
int len = sizeof(SensorMaxNumbers)/sizeof(float);

// reset eeprom //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void reset_eeprom(){

  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }

  float SensorMinNumbers[] = {100,100,100,100,100,100,100,100,100,100};
  float SensorMaxNumbers[] = {0,0,0,0,0,0,0,0,0,0};
  EEPROM.put(0, SensorMinNumbers);
  EEPROM.put(50, SensorMaxNumbers);
}

// Setup ///////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void){
  // 
  //reset_eeprom();
  
  Serial.begin(115200);
  Serial.print("setting up");
  radio.begin(); // Start the NRF24L01
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.openReadingPipe(1,pipe); // Get NRF24L01 ready to receive
  radio.startListening(); // Listen to see if information received

  //LIGHTS
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  // EEPROM DATA LOAD
  EEPROM.get(0, SensorMinNumbers);  
  EEPROM.get(50, SensorMaxNumbers);

  // print out eeprom at start just to check!
  Serial.print("EEPROM.read(0): MIN");
  Serial.print(SensorMinNumbers[0]);
  Serial.print(",");
  Serial.println(SensorMinNumbers[9]);
  
  Serial.print("EEPROM.read(0): MAX");
  Serial.print(SensorMaxNumbers[0]);
  Serial.print(",");
  Serial.println(SensorMaxNumbers[9]);

  if (isnan(SensorMaxNumbers[9])){
    Serial.println("NAN! found");
    reset_eeprom();
  }else{
    Serial.println("NAN! not found");
  }
}

// Render next ///////////////////////////////////////////////////////////////////////////////////////////////////
void render_next_frame() {
  // draw/copy what you want the next frame to be into the next_frame

  startIndex = startIndex + 1; // loop through the leds
  if (startIndex>NUM_LEDS){startIndex=0;}
  
  for ( int i = 0; i < NUM_LEDS; i++) {
    next_frame[i] -= CHSV( 120, 150, 50); // this fades like the fadetoblack with hue
    if(i==startIndex){next_frame[i] += CHSV( 120, 150, 255); // this lights the next in the loop
    ;}
    // reverse loop
    int x = NUM_LEDS-startIndex;
    if(i==x){next_frame[i] += CHSV( 170, 255, 130); // this lights the next in the loop
    ;}
    
  }
}

// ease the speed number ///////////////////////////////////////////////////////////////////////////////////////////////////
float easeInOutCubic(float x){
    return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
}

/*
// showDisplay - light animation ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void showDisplay(int percentSpeed) {

  int speedDiff = percentSpeed-oldEnergyPercentage; // work out the difference in speed
  float tweenedSpeed = oldEnergyPercentage + (speedDiff*easeInOutCubic(myTweenTimer)); // tween the speed

  Serial.print("animating: "); 
  Serial.print(percentSpeed/100.00);
  Serial.print(" myTweenTimer: "); 
  Serial.print(myTweenTimer);
  Serial.print(" tweenmultiplier: "); 
  Serial.print(easeInOutCubic(myTweenTimer));
  Serial.print(" speedDiff: "); 
  Serial.print(speedDiff);
  Serial.print(" tweened percent: "); 
  Serial.println(tweenedSpeed);

  Serial.println(gHue);

  dot += 1; 
  if(dot > NUM_LEDS) {dot=0;}

  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( 0, 255, 128);

  leds[dot] += CHSV( 120, 150, 255); //CRGB::WhiteSmoke;
  FastLED.show();
  // clear this led for the next time around the loop
  fadeToBlackBy( leds, NUM_LEDS, 10);
}
*/
/* chase function
static void chase(int percentSpeed) {

  int resolution = 100 - percentSpeed; // invert the percentage to give us the resolution: as res 100 is slow/high res, res 1 is fast/low res, because the higher the resolution the slower the animation! // higher the res the more pixels in the count, and the slower the animation
  int pixelsCount = strip.numPixels()*resolution;
      
  int p = counter/resolution; // get this pixel
  float p_transform = (1.00/resolution) + ((counter/float(resolution))-p); // get the transform percentage for this pixel - this is used to fade a pixel, a pixel can be 10 levels of brightness, this is got from the float value, how we 'anti alias' the animation
  
  int i_10 = counter-(10*resolution); // length of the colour bar **** NEED to get this as a percentage at some point
  if (i_10 < 0){i_10 = i_10+pixelsCount;} // this is to offset the animatio so it seemlessly loops behind the pixel thats in the front
  
  int p_10 = i_10/resolution; // get the pixel ref
  float p_10_transform = 0.1 + ((i_10/float(resolution))-p_10); // get the transfprm percentage for this pixel

  // colours onto the strip //
  int tcol = transformColour(50,255,p_transform);
  strip.setPixelColor(p, strip.Color(tcol, tcol, 50)); // Draw new pixel
  
  int tcol_10 = transformColour(255,50,p_transform);
  strip.setPixelColor(p_10, strip.Color(tcol_10, tcol_10, 0)); // Erase pixel a few steps back
  
  strip.show(); // upate the leds

  // count up, and limit to the pixelsCount, the number of pixels
  counter++; if(counter>pixelsCount){counter=0;}
}
*/

// PROCESS DATA /////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int processData(float sensorData){
// Processes the raw data from the energy monitor base unit
// saves Max and Min values over time
// turns it into a percentage of current energy use based on averages //////////////////////////////////////////////////////////////////////////////////////////////
// this becomes the speed of the animation

// pre process data
  if (sensorData<0){sensorData=0.00;} // remove minus readings
  if (sensorData>1000){sensorData=SensorMaxNumbers[0];} // remove stupid high readings
 
// List of MIN amounts ////////////////////////////////////////////////////////////////////////////////////  
  if (sensorData < SensorMinNumbers[len-1] ){  // is this numner smaller than the biggest in the list?
    //Serial.println("- adding min number - ");  
    SensorMinNumbers[len-1] = sensorData; // add new number in replacing the biggest numner
  
    // sort the array
    for(int x = 0; x < len; x++)
      for(int y = 0; y < len-1; y++)
      if(SensorMinNumbers[y] > SensorMinNumbers[y+1]) {
         float holder = SensorMinNumbers[y+1];
         SensorMinNumbers[y+1] = SensorMinNumbers[y];
         SensorMinNumbers[y] = holder;
      }
  }
    
// List of MAX amounts ////////////////////////////////////////////////////////////////////////////////////
    if (sensorData > SensorMaxNumbers[0] ){  // add high numbner to the max list 
      //Serial.println("- adding max number - ");  
      SensorMaxNumbers[0] = sensorData; // add new number in replacing the biggest numner
      
      // sort the array
      for(int x = 0; x < len; x++)
        for(int y = 0; y < len-1; y++)
          if(SensorMaxNumbers[y] > SensorMaxNumbers[y+1]) {
            float holder = SensorMaxNumbers[y+1];
            SensorMaxNumbers[y+1] = SensorMaxNumbers[y];
            SensorMaxNumbers[y] = holder;
          }
    }

// now we find the average high + average low readings ////////////////////////////////////
  float tmin = 0;
  float tmax = 0;
  for(int x = 0; x < len; x++){
    tmin = tmin + SensorMinNumbers[x];
    tmax = tmax + SensorMaxNumbers[x];
  }
  float avlo = tmin/len;
  float avhi = tmax/len;

// now we work out the 'sensorData' current reading number as a percentage in-between the min and max
  float scope = (avhi - avlo); // scope is the gap between the lowest an the highest
  
  int CurrentReadingPercent = ((sensorData-avlo)/scope)*100; // make speed a percentage, like a progress bar between the low and high points
  if (CurrentReadingPercent>99) // limit the percentage from minusses and 101% ect
    CurrentReadingPercent = 99;

  if (CurrentReadingPercent<0)
    CurrentReadingPercent = 1;


  // debug print ing of values //
  
  Serial.println(""); Serial.println(""); 
  Serial.print("sensordata: ");   Serial.print(sensorData);
  Serial.print(" | scope : ");    Serial.print(scope);
  Serial.print(" | percent : ");  Serial.print(CurrentReadingPercent);
  Serial.print(" | avlo: ");      Serial.print(avlo); 
  Serial.print(" | avhi: ");      Serial.print(avhi);
  
  Serial.println(""); Serial.print("MIN LIST: ");
    for(int i=0;i<len;i++){
      Serial.print(SensorMinNumbers[i]); Serial.print(", ");
    }
  Serial.println(""); Serial.print("MAX LIST: ");
    for(int i=0;i<10;i++){
      Serial.print(SensorMaxNumbers[i]); Serial.print(", ");
    }
      
  //  save eeprom //
  EEPROM.put(0, SensorMinNumbers);
  EEPROM.put(50, SensorMaxNumbers);

  // return the percent, it will be turned in reoultion to adjust the speed
  return CurrentReadingPercent;
}

///LOOP //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void){ 
  if (myTweenTimer<1){
    myTweenTimer+=0.001;
  }else{
    myTweenTimer=1;
  }

  while (radio.available()){
    radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); // Read information from the NRF24L01
    int fromRadio = processData(ReceivedMessage[0]);// pass the data to be processed - high low average and turn it into a percentage for speed

    //flash display when data updates - red if higher, green if lower
    for ( int i = 0; i < NUM_LEDS; i++) {
      if(energyPercentage<fromRadio){
      next_frame[i] += CHSV( 255, 255, 50); // this fades like the fadetoblack with hue
      }else{
      next_frame[i] += CHSV( 120, 255, 50); // this fades like the fadetoblack with hue
      }
    }

    // save oldWheelSpeed
    oldEnergyPercentage = energyPercentage;
    // start timer
    myTweenTimer = 0;
    // set the 
    energyPercentage = fromRadio; 
    //flash display when data updates
  }

  // OR use serial to change wheelspeed precentage
  if ( Serial.available() > 0) {
    int fromSerial = Serial.parseInt(); //Read the data the user has input

    //flash display when data updates - red if higher, green if lower
    for ( int i = 0; i < NUM_LEDS; i++) {
      if(energyPercentage<fromSerial){
      next_frame[i] += CHSV( 255, 255, 50); // this fades like the fadetoblack with hue
      }else{
      next_frame[i] += CHSV( 120, 255, 50); // this fades like the fadetoblack with hue
      }
    }

    // save oldWheelSpeed
    oldEnergyPercentage = energyPercentage;
    // start timer
    myTweenTimer = 0;
    // set the 
    energyPercentage = fromSerial;
  }
  
  // animate! - this is the frameblending script
  int speedDiff = energyPercentage-oldEnergyPercentage; // work out the difference in speed from the data
  float tweenedSpeed = oldEnergyPercentage + (speedDiff*easeInOutCubic(myTweenTimer)); // tween the speed
  // set speed (steps id the resolution of the frame interpolation)
  STEPS = int(100-tweenedSpeed);

  static uint8_t cur_step;
  EVERY_N_MILLISECONDS(1000 / (ANIM_FPS * (STEPS)) ) {  //EVERY_N_MILLISECONDS(1000 / (ANIM_FPS * STEPS) ) { 
    if(cur_step == 0) {
      for(int i = 0; i < NUM_LEDS; i++) { old_frame[i] = next_frame[i]; }
      render_next_frame();
    }

    blend(old_frame, next_frame, leds, NUM_LEDS, cur_step * SCALE_PER_STEP);
    FastLED.show();
    cur_step++;

    if(cur_step == STEPS) { cur_step = 0; }
  } else { 
    // keep driving show for the temporal dithering
    FastLED.show();
  }
}
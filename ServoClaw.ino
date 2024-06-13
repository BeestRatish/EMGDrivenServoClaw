#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "EMGFilters.h"

#define TIMING_DEBUG 1

#define SensorInputPin A0 // input pin number

#include <Servo.h>

EMGFilters myFilter;
// discrete filters must works with fixed sample frequence
// our emg filter only support "SAMPLE_FREQ_500HZ" or "SAMPLE_FREQ_1000HZ"
// other sampleRate inputs will bypass all the EMG_FILTER
int sampleRate = SAMPLE_FREQ_1000HZ;
// For countries where power transmission is at 50 Hz
// For countries where power transmission is at 60 Hz, need to change to
// "NOTCH_FREQ_60HZ"
// our emg filter only support 50Hz and 60Hz input
// other inputs will bypass all the EMG_FILTER
int humFreq = NOTCH_FREQ_50HZ;

// Calibration:
// put on the sensors, and release your muscles;
// wait a few seconds, and select the max value as the throhold;
// any value under throhold will be set to zero
static int Throhold = 1300;

unsigned long timeStamp;
unsigned long timeBudget;


Servo clawServo;

int pos;
int bufIndex;
int buf[50];
int bufAve;

int openBool;


void setup() {
    /* add setup code here */
    myFilter.init(sampleRate, humFreq, true, true, true);

    // open serial
    Serial.begin(115200);

    // setup for time cost measure
    // using micros()
    timeBudget = 1e6 / sampleRate;
    // micros will overflow and auto return to zero every 70 minutes

    clawServo.attach(9);
    clawServo.write(125);
    pos = clawServo.read();
    bufIndex = 0;
    bufAve = 0;
    openBool = 1;
}

void loop() {
    /* add main program code here */
    // In order to make sure the ADC sample frequence on arduino,
    // the time cost should be measured each loop
    /*------------start here-------------------*/
    timeStamp = micros();

    int Value = analogRead(SensorInputPin);

    // filter processing
    int DataAfterFilter = myFilter.update(Value);
    

    
    int envlope = sq(DataAfterFilter);
    // any value under throhold will be set to zero
    envlope = (envlope > Throhold) ? envlope : 0;
    Serial.println(envlope);
    
//creates a buffer of 50 and reads values into it. when buffer it full, average is calculated
    if(bufIndex == 49){
      bufAve = bufferAverage(buf);
      bufIndex = 0;
    }
    
    else{
      buf[bufIndex] = envlope;
      bufIndex = bufIndex +1;
    }

 //if the buffer average == 0, no voltage spikes were detected and the claw doesn't move. 
 //if the average is greater than 0, the claw will either open or close
  if(bufAve>0){
    if(openBool){
      moveClaw();
      openBool = 0;
      }     
    }
   else if(bufAve == 0){
    openBool = 1;
   }

   

    /*------------end here---------------------*/
    // if less than timeBudget, then you still have (timeBudget - timeStamp) to
    // do your work
    delayMicroseconds(500);
    // if more than timeBudget, the sample rate need to reduce to
    // SAMPLE_FREQ_500HZ

    
}


void moveClaw() {
    //open claw
    pos = clawServo.read(); 
    if (pos == 125){
      clawServo.write(60);
   }
   //close claw
     else if (pos == 60){
      clawServo.write(125);
   }

    delay(100);                       
  
}



int bufferAverage (int * buf)  
{
  //calculates the average of the values in the buffer
  int sum = 0;  
  for (int i = 0 ; i < 50 ; i++) //500 samples per second
    sum += buf[i] ;
  return  sum/50 ;  
}

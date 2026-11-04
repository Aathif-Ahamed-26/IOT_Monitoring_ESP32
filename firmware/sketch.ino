
#include <Preferences.h>

Preferences preferences;


const int MQ2_PIN=34;
unsigned long prevMillis =0;
const unsigned long interval =10000;
const unsigned long warmUpTime=30000;
bool checkResponse = false;
const int minGasValue=100;
const int maxGasValue=4000;
unsigned long startMillis=0;
int lastGasValue=-1;
unsigned long sameValueStartMillis=0;
const unsigned long frozenTimeMillis=60000;


bool frozenResponse= false;

int validCount=0;
float totalGasValue=0;
float minGasReading=0;
float maxGasReading=0;
unsigned long reportTimerMillis=0;
const unsigned long reportMillis=30000; 

float calibrationGain=1.0;
float calibrationOffset=0.0;

void setup() {
  // put your setup code here, to run once:


  
  Serial.begin(115200);

  preferences.begin("calibration", false);
  calibrationGain=preferences.getFloat("gain",1.0);
  calibrationOffset=preferences.getFloat("offset",0.0);
Serial.print("Loaded Gain: ");
Serial.println(calibrationGain);

Serial.print("Loaded Offset: ");
Serial.println(calibrationOffset);


  startMillis=millis();
  pinMode(MQ2_PIN, INPUT);
  Serial.println("Hello, ESP32!");
  Serial.println("MQ-2 is warming up");
}

void loop() {
  // put your main code here, to run repeatedly:

    if(Serial.available()){
      char command = Serial.read();

      if(command == 'g'){
        float value = Serial.parseFloat();
        updateGain(value);
        Serial.println("gain updated and saved");
      }

      if(command =='o'){
        float value =Serial.parseFloat();
        updateOffset(value);
        Serial.println("Offset updated and saved");
      }
    }

if (Serial.available()) {
    char command = Serial.read();

    if (command == 'r') {
        ESP.restart();
    }
}




  
   if (checkResponse) {
  
    Serial.println("ERROR: No sensor response");
  } else {
  unsigned long currMillis=millis();
  if(currMillis-startMillis >= warmUpTime){
  if(currMillis-prevMillis >= interval){
    prevMillis=currMillis;
    bool isValid=true;

  int gasvalue=analogRead(MQ2_PIN);
  
float calibratedValue= (gasvalue * calibrationGain)+calibrationOffset;

  if(calibratedValue==0){
    isValid=false;
    Serial.println("ERROR: no sensor response ");
  }
  else if(calibratedValue<minGasValue || calibratedValue > maxGasValue){
  isValid=false;
  Serial.println("ERROR:sensor out of range");
  
  }
  else{
    if(calibratedValue != lastGasValue){
    lastGasValue = calibratedValue;
    sameValueStartMillis = currMillis;
    frozenResponse = false;

    Serial.print("MQ2 VALUE: ");
    Serial.println(calibratedValue);
    Serial.print("Raw Value: ");
Serial.println(gasvalue);

Serial.print("Calibrated Value: ");
Serial.println(calibratedValue);
  }
  else{
    if(currMillis - sameValueStartMillis >= frozenTimeMillis){
      isValid=false;
    if(!frozenResponse){
    Serial.println("ERROR: Sensor reading frozen");
    frozenResponse =true;
      }
      
    }
    else{
      Serial.print("MQ2 VALUE: ");
      Serial.println(calibratedValue);
      Serial.print("Raw Value: ");
Serial.println(gasvalue);

Serial.print("Calibrated Value: ");
Serial.println(calibratedValue);
    }

    
  
  }if(isValid){
    totalGasValue+=calibratedValue;
    validCount++;
    if(validCount==1 || calibratedValue<minGasReading){
      minGasReading=calibratedValue;}
    if(validCount==1 || calibratedValue>maxGasReading){
      maxGasReading=calibratedValue;}  

  }}}
  
  }
  if(currMillis-reportTimerMillis>=reportMillis){
    reportTimerMillis=currMillis;

    if(validCount>0){

      float avgGasValue =(float)totalGasValue/validCount;

      Serial.println("60'Sec Summary:");
      Serial.print("Average : ");
      Serial.println((float)avgGasValue);
      Serial.print("Minimum : ");
      Serial.println(minGasReading);
      Serial.print("Maximum : ");
      Serial.println(maxGasReading);
      Serial.print("ValidCount : ");
      Serial.println(validCount);

      avgGasValue=0;
      minGasReading=0;
      maxGasReading=0;
      validCount=0;
      totalGasValue=0;

    }else{
       Serial.println("60'Sec Summary:");
       Serial.println("No valid readings");
       Serial.println("Valid count: 0 ");

    }
  }
}
}

void updateGain(float newGain){
  calibrationGain=newGain;
  preferences.putFloat("gain",calibrationGain);
  
    Serial.print("Saved Gain: ");
    Serial.println(preferences.getFloat("gain", 1.0));

}

void updateOffset(float newOffset){
  calibrationOffset=newOffset;
  preferences.putFloat("offset",calibrationOffset);

  Serial.print("Saved Offset: ");
    Serial.println(preferences.getFloat("offset", 0.0));

}
  
  

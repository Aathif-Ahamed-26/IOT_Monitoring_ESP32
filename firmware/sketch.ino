const int MQ2_PIN=34;
unsigned long prevMillis =0;
const unsigned long interval =10000;//10000
const unsigned long warmUpTime=30000;//30000
bool checkResponse = false;
const int minGasValue=100;
const int maxGasValue=4000;
unsigned long startMillis=0;
int lastGasValue=-1;
unsigned long sameValueStartMillis=0;
const unsigned long frozenTimeMillis=60000;


bool frozenResponse= false;

int validCount=0;
int totalGasValue=0;
int minGasReading=0;
int maxGasReading=0;
unsigned long reportTimerMillis=0;
const unsigned long reportMillis=30000; 

void setup() {
  // put your setup code here, to run once:


  Serial.begin(115200);
  startMillis=millis();
  pinMode(MQ2_PIN, INPUT);
  Serial.println("Hello, ESP32!");
  Serial.println("MQ-2 is warming up");
}

void loop() {
  // put your main code here, to run repeatedly:


  
   if (checkResponse) {
  
    Serial.println("ERROR: No sensor response");
  } else {
  unsigned long currMillis=millis();
  if(currMillis-startMillis >= warmUpTime){
  if(currMillis-prevMillis >= interval){
    prevMillis=currMillis;
    bool isValid=true;
  int gasvalue=analogRead(MQ2_PIN);
  
  if(gasvalue==0){
    isValid=false;
    Serial.println("ERROR: no sensor response ");
  }
  else if(gasvalue<minGasValue || gasvalue > maxGasValue){
  isValid=false;
  Serial.println("ERROR:sensor out of range");
  
  }
  else{
    if(gasvalue != lastGasValue){
    lastGasValue = gasvalue;
    sameValueStartMillis = currMillis;
    frozenResponse = false;

    Serial.print("MQ2 VALUE: ");
    Serial.println(gasvalue);
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
      Serial.println(gasvalue);
    }

    
  
  }if(isValid){
    totalGasValue+=gasvalue;
    validCount++;
    if(validCount==1 || gasvalue<minGasReading){
      minGasReading=gasvalue;}
    if(validCount==1 || gasvalue<maxGasReading){
      maxGasReading=gasvalue;}  

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

    }else{
       Serial.println("60'Sec Summary:");
       Serial.println("No valid readings");
       Serial.println("Valid count: 0 ");

    }
  }
}
}
  
  

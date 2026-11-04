const int MQ2_PIN=34;
unsigned long prevMillis =0;
const unsigned long interval =10000;
const unsigned long warmUpTime=3000;
bool checkResponse = false;
const int minGasValue=100;
const int maxGasValue=4000;
unsigned long startMillis=0;
int lastGasValue=-1;
unsigned long sameValueStartMillis=0;
const unsigned long frozenTimeMillis=6000;
bool frozenResponse= false;
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
  if(currMillis - startMillis >= warmUpTime){
  if(currMillis - prevMillis >= interval){
    prevMillis=currMillis;
  
  int gasvalue=analogRead(MQ2_PIN);
  if(gasvalue==0){
    Serial.println("ERROR: no sensor response ");
  }
  else if(gasvalue<minGasValue || gasvalue > maxGasValue){
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
    if(!frozenResponse){
    Serial.println("ERROR: Sensor reading frozen");
    frozenResponse =true;
      }
    }
    else{
      Serial.print("MQ2 VALUE: ");
      Serial.println(gasvalue);
    }

    
  
  }
  }}}
  
  }
}
  
  // delay(10); // this speeds up the simulation


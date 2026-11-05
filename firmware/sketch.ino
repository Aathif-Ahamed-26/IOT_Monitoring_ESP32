#include<WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <PubSubClient.h>

Preferences preferences;
Preferences bufferPref;

const char* ssid="Wokwi-GUEST";
const char* password="";


const int MQ2_PIN=34;
unsigned long prevMillis =0;
const unsigned long interval =1000; //1000
const unsigned long warmUpTime=3000;
bool checkResponse = false;
const int minGasValue=100;
const int maxGasValue=4000;
unsigned long startMillis=0;
int lastGasValue=-1;
unsigned long sameValueStartMillis=0;
const unsigned long frozenTimeMillis=6000;


bool frozenResponse= false;

int validCount=0;
float totalGasValue=0;
float minGasReading=0;
float maxGasReading=0;
unsigned long reportTimerMillis=0;
const unsigned long reportMillis=6000; 

float calibrationGain=1.0;
float calibrationOffset=0.0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* mqttServer ="broker.hivemq.com";
const int mqttPort =1883;
const char* mqttTopic="Aathif/esp32/gas-monitor";


// const int bufferCapacity=10;
// int bufferCount=0;
// unsigned long recordTimeStamp=0;
// int bufferTail=0;
// int bufferHead=0;

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
    WiFi.begin(ssid,password);
    Serial.println("connecting to wifi ");

    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWifi connected");

  //   bufferPref.begin("buffer",false);
  //   bufferCount=bufferPref.getInt("count",0);


  // bufferCount= bufferPref.getInt("count",0);

  // bufferHead =bufferPref.getInt("head",0);
  // bufferTail= bufferPref.getInt("tail",0);
 mqttClient.setServer(mqttServer,mqttPort); 

 if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed");
    return;
}

Serial.println("LittleFS mounted successfully");
    sendSavedData();


  startMillis=millis();
  pinMode(MQ2_PIN, INPUT);
  Serial.println("Hello, ESP32!");
  Serial.println("MQ-2 is warming up");
  
}

void loop() {
  // put your main code here, to run repeatedly:

    mqttClient.loop();

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

if(WiFi.status() ==WL_CONNECTED){
  connectMqtt();
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
      unsigned long timeStamp = millis();

if (mqttClient.connected()) {
    publishSensorData(avgGasValue, minGasReading,
                      maxGasReading, validCount, timeStamp);
} else {
    saveDataOffline(avgGasValue, timeStamp);
}

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



// void savebufferCount(){
// bufferPref.putInt("count",bufferCount);
// bufferPref.putInt("head",bufferHead);
// bufferPref.putInt("tail",bufferTail);
// }
// void saveRecord(float value,unsigned long timeStamp){


//   if(bufferCount>=bufferCapacity){
//     Serial.println("Buffer full! cannot save record");
//     return;
//   }
// }
// struct SensorRecord {
//     float value;
//     unsigned long timestamp;
// };
  
  void saveDataOffline(float value, unsigned long timeStamp) {
    File file = LittleFS.open("/offline_data.csv", FILE_APPEND);

    if(!file){
        Serial.println("Could not open file");
        return;
    }

    file.print(timeStamp);
    file.print(",");
    file.println(value);

    file.close();

    Serial.println("Data saved offline");
    sendSavedData();
}

void sendSavedData() {
    File file = LittleFS.open("/offline_data.csv", FILE_READ);

    if (!file || file.size()==0) {
        if(file)file.close();
        return;
    }

    while(file.available()) {
        String line = file.readStringUntil('\n');
        Serial.print("Saved record: ");
        Serial.println(line);
    }

    file.close();
}

void connectMqtt(){
  if (!mqttClient.connected()) {
        Serial.println("Connecting to MQTT...");
  if(mqttClient.connect("esp32-gas-monitor-001")){
    Serial.println("MQTT connected");
  }else{
    Serial.print("MQTT connection Failed");
    Serial.println(mqttClient.state());

  }
Serial.print("WiFi status: ");
Serial.println(WiFi.status());
}
}

void publishSensorData(float avg, float minVal,
                       float maxVal, int count,
                       unsigned long timeStamp) {

    String payload = "{";
    payload += "\"timestamp\":" + String(timeStamp) + ",";
    payload += "\"average\":" + String(avg, 2) + ",";
    payload += "\"minimum\":" + String(minVal, 2) + ",";
    payload += "\"maximum\":" + String(maxVal, 2) + ",";
    payload += "\"valid_count\":" + String(count);
    payload += "}";

    mqttClient.publish(mqttTopic, payload.c_str());

    Serial.println("MQTT record published:");
    Serial.println(payload);
}

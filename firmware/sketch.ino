#include<WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

Preferences preferences;
Preferences bufferPref;

String ssid = "Wokwi-GUEST";
String password = "";


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

String mqttServer = "broker.hivemq.com";
const int mqttPort =1883;
const char* mqttTopic="Aathif/esp32/gas-monitor";

unsigned long lastWifiAttempt=0;
unsigned long lastMqttAttempt=0;

unsigned long wifiRetryInterval=5000;
unsigned long mqttRetryInterval=5000;




void setup() {
  // put your setup code here, to run once:


  
  Serial.begin(115200);

  preferences.begin("calibration", false);
  wifiRetryInterval = preferences.getULong("wifi_retry", 5000);
mqttRetryInterval = preferences.getULong("mqtt_retry", 5000);
  mqttServer = preferences.getString("mqtt_server", "broker.hivemq.com");
  ssid = preferences.getString("ssid", "Wokwi-GUEST");
password = preferences.getString("password", "");
Serial.print("Loaded WiFi SSID: ");
Serial.println(ssid);



  Serial.print("Loaded MQTT Server: ");
 Serial.println(mqttServer);
  calibrationGain=preferences.getFloat("gain",1.0);
  calibrationOffset=preferences.getFloat("offset",0.0);
Serial.print("Loaded Gain: ");
Serial.println(calibrationGain);

Serial.print("Loaded Offset: ");
Serial.println(calibrationOffset);
   WiFi.begin(ssid,password);
Serial.println("WiFi connection started. Connecting in background...");
 
 mqttClient.setServer(mqttServer.c_str(),mqttPort); 

 if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
}

Serial.println("LittleFS mounted successfully");
    


  startMillis=millis();
  pinMode(MQ2_PIN, INPUT);
  Serial.println("Hello, ESP32!");
  Serial.println("MQ-2 is warming up");
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,
    .idle_core_mask = 0,
    .trigger_panic = true
};

esp_task_wdt_reconfigure(&wdt_config);
esp_task_wdt_add(NULL);

  
}

void loop() {
  // put your main code here, to run repeatedly:
    esp_task_wdt_reset();
//     while (true) {
// }
    mqttClient.loop();

if (Serial.available() > 0) {

    String command = Serial.readStringUntil('\n');
    command.trim();

    Serial.print("Command received: ");
    Serial.println(command);

    if (command.startsWith("server")) {
    String newServer = command.substring(7);
    updateMqttServer(newServer);
}
if (command.startsWith("wifi ")) {

    int separator = command.indexOf(' ', 5);

    if (separator > 0) {
        String newSsid = command.substring(5, separator);
        String newPassword = command.substring(separator + 1);

        updateWifiCredentials(newSsid, newPassword);
    }
}
else if (command.startsWith("retry ")) {

    int separator = command.indexOf(' ', 6);

    if (separator > 0) {
        unsigned long newWifi = command.substring(6, separator).toInt();
        unsigned long newMqtt = command.substring(separator + 1).toInt();

        if (newWifi >= 1000 && newMqtt >= 1000) {
            updateRetryIntervals(newWifi, newMqtt);
        } else {
            Serial.println("Retry intervals must be at least 1000 ms");
        }
    } else {
        Serial.println("Usage: retry 5000 7000");
    }
}else if (command == "status") {

    Serial.println("\n--- DEVICE STATUS ---");

    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    Serial.print("MQTT: ");
    Serial.println(mqttClient.connected() ? "Connected" : "Disconnected");

    Serial.print("MQTT Server: ");
    Serial.println(mqttServer);

    Serial.print("WiFi Retry: ");
    Serial.print(wifiRetryInterval);
    Serial.println(" ms");

    Serial.print("MQTT Retry: ");
    Serial.print(mqttRetryInterval);
    Serial.println(" ms");

    Serial.print("Sensor Monitoring: ");
    Serial.println(checkResponse ? "Enabled" : "Disabled");

    Serial.println("---------------------");
}

  else if (command.startsWith("g")) {
        float value = command.substring(1).toFloat();
        updateGain(value);
        Serial.println("Gain updated and saved");
    }

    else if (command.startsWith("o")) {
        float value = command.substring(1).toFloat();
        updateOffset(value);
        Serial.println("Offset updated and saved");
    }

    else if (command == "r") {
        Serial.println("Restarting ESP32...");
        ESP.restart();
    }

    else if (command == "d") {
        WiFi.disconnect();
        Serial.println("WiFi disconnected");
    }

    else if (command == "w") {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Connecting to WiFi...");
    }
    else if (command == "sd") {
    checkResponse = true;
    Serial.println("Sensor disconnected!");
}

else if (command == "sr") {
    checkResponse = false;
    Serial.println("Sensor reconnected!");
}
}
if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiAttempt>=wifiRetryInterval) {
        lastWifiAttempt=millis();
        Serial.println("Reconnecting to WiFi...");
        WiFi.begin(ssid.c_str(),password.c_str());
    }
}
else {
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

  if(gasvalue==0){
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
    saveDataOffline(avgGasValue, minGasReading,
                maxGasReading, validCount, timeStamp);
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
  
  void saveDataOffline( float avg, float minVal,
                float maxVal, float count, unsigned long timeStamp) {
    File file = LittleFS.open("/offline_data.csv", FILE_APPEND);

    if(!file){
        Serial.println("Could not open file");
        return;
    }

    file.print(timeStamp);
    file.print(",");
    file.print(avg);
    file.print(",");
    file.print(minVal);
    file.print(",");
    file.print(maxVal);
    file.print(",");
    file.println(count);
    file.close();

    Serial.println("Data saved offline");
    
}
void sendSavedData() {

    File file = LittleFS.open("/offline_data.csv", FILE_READ);

    if (!file || file.size() == 0) {
        if (file) file.close();
        return;
    }

    while (file.available()) {

        String line = file.readStringUntil('\n');

        unsigned long timeStamp;
        float avg, minVal, maxVal;
        int count;

        sscanf(line.c_str(), "%lu,%f,%f,%f,%d",
               &timeStamp, &avg, &minVal, &maxVal, &count);

        if (!publishSensorData(avg, minVal, maxVal, count, timeStamp)) {
    file.close();
    Serial.println("Publish failed. Keeping offline records.");
    return;
}
    }

    file.close();
    LittleFS.remove("/offline_data.csv");
Serial.println("Offline records cleared");
}
void connectMqtt(){
  if (!mqttClient.connected()) {
        
        if (millis() - lastMqttAttempt >= mqttRetryInterval) {
            Serial.println("Connecting to MQTT...");
            lastMqttAttempt = millis();
  if(mqttClient.connect("esp32-gas-monitor-001")){
    Serial.println("MQTT connected");
    sendSavedData();
  }else{
    Serial.print("MQTT connection Failed");
    Serial.println(mqttClient.state());

  }
Serial.print("WiFi status: ");
Serial.println(WiFi.status());
}
}
}

bool publishSensorData(float avg, float minVal,
                       float maxVal, int count,
                       unsigned long timeStamp) {

    String payload = "{";
    payload += "\"timestamp\":" + String(timeStamp) + ",";
    payload += "\"average\":" + String(avg, 2) + ",";
    payload += "\"minimum\":" + String(minVal, 2) + ",";
    payload += "\"maximum\":" + String(maxVal, 2) + ",";
    payload += "\"valid_count\":" + String(count);
    payload += "}";

    bool success = mqttClient.publish(mqttTopic, payload.c_str());

if (success) {
    Serial.println("MQTT record published successfully:");
    Serial.println(payload);
} else {
    Serial.println("MQTT publish failed");
}
    Serial.println("MQTT record published:");
    Serial.println(payload);
    return success;
}
void updateMqttServer(String newServer) {
    mqttClient.disconnect();

    mqttServer = newServer;

    preferences.putString("mqtt_server", mqttServer);

    Serial.print("New MQTT Server saved: ");
    Serial.println(mqttServer);

}
void updateWifiCredentials(String newSsid, String newPassword) {

    ssid = newSsid;
    password = newPassword;

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    WiFi.disconnect();
  WiFi.begin(ssid.c_str(), password.c_str());

    Serial.println("WiFi credentials saved successfully");

}
void updateRetryIntervals(unsigned long newWifiInterval, unsigned long newMqttInterval) {

    wifiRetryInterval = newWifiInterval;
    mqttRetryInterval = newMqttInterval;

    preferences.putULong("wifi_retry", wifiRetryInterval);
    preferences.putULong("mqtt_retry", mqttRetryInterval);

    Serial.println("Retry intervals saved successfully");
}
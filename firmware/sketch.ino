#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

#include "TimeManager.h"
#include "SensorManager.h"
#include "HealthManager.h"

// Objects
TimeManager timeManager;
SensorManager sensorManager;
HealthManager healthManager;

Preferences preferences;
Preferences bufferPref;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// WiFi settings
String ssid = "Wokwi-GUEST";
String password = "";

// MQTT settings
String mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

const char* mqttTopic =
    "Aathif/devices/ESP32_001/gas-monitor";

const char* healthTopic =
    "Aathif/devices/ESP32_001/health";

// Sensor settings
const int MQ2_PIN = 34;

unsigned long prevMillis = 0;
const unsigned long interval = 1000;

const unsigned long warmUpTime = 3000;
unsigned long startMillis = 0;

bool checkResponse = false;

const int minGasValue = 100;
const int maxGasValue = 4000;

int lastGasValue = -1;
unsigned long sameValueStartMillis = 0;

const unsigned long frozenTimeMillis = 6000;
bool frozenResponse = false;

// Variables used to calculate sensor readings
int validCount = 0;

float totalGasValue = 0;
float minGasReading = 0;
float maxGasReading = 0;

unsigned long reportTimerMillis = 0;
const unsigned long reportMillis = 6000;

// Calibration values
float calibrationGain = 1.0;
float calibrationOffset = 0.0;

// WiFi and MQTT retry settings
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;

unsigned long wifiRetryInterval = 5000;
unsigned long mqttRetryInterval = 5000;

// Health report settings
unsigned long lastHealthReport = 0;
const unsigned long healthReportInterval = 60000;

String lastError = "None";

// Function declarations
void updateGain(float newGain);
void updateOffset(float newOffset);

void saveDataOffline(
    float avg,
    float minVal,
    float maxVal,
    float count,
    unsigned long timeStamp
);

void sendSavedData();
void connectMqtt();

bool publishSensorData(
    float avg,
    float minVal,
    float maxVal,
    int count,
    unsigned long timeStamp
);

void updateMqttServer(String newServer);

void updateWifiCredentials(
    String newSsid,
    String newPassword
);

void updateRetryIntervals(
    unsigned long newWifiInterval,
    unsigned long newMqttInterval
);

void publishHealthStatus();


void setup() {

    Serial.begin(115200);

    // Load saved settings
    bufferPref.begin("buffer", false);
    preferences.begin("calibration", false);

    wifiRetryInterval =
        preferences.getULong("wifi_retry", 5000);

    mqttRetryInterval =
        preferences.getULong("mqtt_retry", 5000);

    mqttServer =
        preferences.getString(
            "mqtt_server",
            "broker.hivemq.com"
        );

    ssid =
        preferences.getString(
            "ssid",
            "Wokwi-GUEST"
        );

    password =
        preferences.getString("password", "");

    calibrationGain =
        preferences.getFloat("gain", 1.0);

    calibrationOffset =
        preferences.getFloat("offset", 0.0);

    Serial.println("Device settings loaded");

    Serial.print("WiFi SSID: ");
    Serial.println(ssid);

    Serial.print("MQTT Server: ");
    Serial.println(mqttServer);

    Serial.print("Calibration Gain: ");
    Serial.println(calibrationGain);

    Serial.print("Calibration Offset: ");
    Serial.println(calibrationOffset);

    // Start WiFi connection
    WiFi.begin(ssid.c_str(), password.c_str());

    timeManager.syncTime();

    // Set MQTT server
    mqttClient.setServer(
        mqttServer.c_str(),
        mqttPort
    );

    // Start LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    Serial.println("LittleFS ready");

    // Start sensor
    sensorManager.begin();

    startMillis = millis();

    Serial.println("ESP32 started");
    Serial.println("MQ2 sensor warming up...");

    // Configure watchdog
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };

    esp_task_wdt_reconfigure(&wdt_config);
    esp_task_wdt_add(NULL);
}


void loop() {

    esp_task_wdt_reset();

    mqttClient.loop();

    // Send health information periodically
    if (millis() - lastHealthReport >= healthReportInterval) {

        lastHealthReport = millis();

        publishHealthStatus();
    }

    // Check for commands from Serial Monitor
    if (Serial.available() > 0) {

        String command = Serial.readStringUntil('\n');
        command.trim();

        Serial.print("Received command: ");
        Serial.println(command);

        if (command.startsWith("server")) {

            String newServer = command.substring(7);

            updateMqttServer(newServer);
        }

        else if (command.startsWith("wifi ")) {

            int separator = command.indexOf(' ', 5);

            if (separator > 0) {

                String newSsid =
                    command.substring(5, separator);

                String newPassword =
                    command.substring(separator + 1);

                updateWifiCredentials(
                    newSsid,
                    newPassword
                );
            }
        }

        else if (command.startsWith("retry ")) {

            int separator = command.indexOf(' ', 6);

            if (separator > 0) {

                unsigned long newWifi =
                    command.substring(6, separator).toInt();

                unsigned long newMqtt =
                    command.substring(separator + 1).toInt();

                if (newWifi >= 1000 && newMqtt >= 1000) {

                    updateRetryIntervals(
                        newWifi,
                        newMqtt
                    );
                }
                else {
                    Serial.println(
                        "Retry interval must be at least 1000 ms"
                    );
                }
            }
            else {
                Serial.println("Usage: retry 5000 5000");
            }
        }

        else if (command == "status") {

            Serial.println("\nDevice Status");

            Serial.print("WiFi: ");
            Serial.println(
                WiFi.status() == WL_CONNECTED
                ? "Connected"
                : "Disconnected"
            );

            Serial.print("MQTT: ");
            Serial.println(
                mqttClient.connected()
                ? "Connected"
                : "Disconnected"
            );

            Serial.print("MQTT Server: ");
            Serial.println(mqttServer);

            Serial.print("WiFi Retry: ");
            Serial.println(wifiRetryInterval);

            Serial.print("MQTT Retry: ");
            Serial.println(mqttRetryInterval);

            Serial.print("Sensor Monitoring: ");
            Serial.println(
                checkResponse ? "Disabled" : "Enabled"
            );
        }

        else if (command.startsWith("g")) {

            float value = command.substring(1).toFloat();

            updateGain(value);

            Serial.println("Gain updated");
        }

        else if (command.startsWith("o")) {

            float value = command.substring(1).toFloat();

            updateOffset(value);

            Serial.println("Offset updated");
        }

        else if (command == "r") {

            Serial.println("Restarting device...");
            ESP.restart();
        }

        else if (command == "d") {

            WiFi.disconnect();

            Serial.println("WiFi disconnected");
        }

        else if (command == "w") {

            WiFi.begin(
                ssid.c_str(),
                password.c_str()
            );

            Serial.println("Connecting to WiFi...");
        }

        else if (command == "sd") {

            checkResponse = true;

            Serial.println("Sensor disconnected");
        }

        else if (command == "sr") {

            checkResponse = false;

            Serial.println("Sensor reconnected");
        }
    }

    // Reconnect WiFi if the connection is lost
    if (WiFi.status() != WL_CONNECTED) {

        if (millis() - lastWifiAttempt >= wifiRetryInterval) {

            lastWifiAttempt = millis();

            Serial.println("Trying to reconnect to WiFi...");

            WiFi.begin(
                ssid.c_str(),
                password.c_str()
            );
        }
    }
    else {
        connectMqtt();
    }

    // Check sensor readings
    if (checkResponse) {

        Serial.println("ERROR: No sensor response");
    }
    else {

        unsigned long currMillis = millis();

        if (currMillis - startMillis >= warmUpTime) {

            if (currMillis - prevMillis >= interval) {

                prevMillis = currMillis;

                bool isValid = true;

                int gasvalue = sensorManager.readRawValue();

                float calibratedValue =
                    sensorManager.applyCalibration(
                        gasvalue,
                        calibrationGain,
                        calibrationOffset
                    );

                // Check if the sensor is responding
                if (gasvalue == 0) {

                    isValid = false;

                    Serial.println("ERROR: No sensor response");
                }

                // Check whether the reading is within range
                else if (
                    calibratedValue < minGasValue ||
                    calibratedValue > maxGasValue
                ) {

                    isValid = false;

                    Serial.println("ERROR: Sensor out of range");
                }

                else {

                    // Check if the sensor value is stuck
                    if (calibratedValue != lastGasValue) {

                        lastGasValue = calibratedValue;

                        sameValueStartMillis = currMillis;

                        frozenResponse = false;

                        Serial.print("MQ2 Value: ");
                        Serial.println(calibratedValue);

                        Serial.print("Raw Value: ");
                        Serial.println(gasvalue);

                        Serial.print("Calibrated Value: ");
                        Serial.println(calibratedValue);
                    }
                    else {

                        if (
                            currMillis - sameValueStartMillis >=
                            frozenTimeMillis
                        ) {

                            isValid = false;

                            if (!frozenResponse) {

                                Serial.println(
                                    "ERROR: Sensor reading frozen"
                                );

                                frozenResponse = true;
                            }
                        }
                        else {

                            Serial.print("MQ2 Value: ");
                            Serial.println(calibratedValue);

                            Serial.print("Raw Value: ");
                            Serial.println(gasvalue);

                            Serial.print("Calibrated Value: ");
                            Serial.println(calibratedValue);
                        }
                    }
                }

                // Add the reading if it is valid
                if (isValid) {

                    totalGasValue += calibratedValue;

                    validCount++;

                    if (
                        validCount == 1 ||
                        calibratedValue < minGasReading
                    ) {
                        minGasReading = calibratedValue;
                    }

                    if (
                        validCount == 1 ||
                        calibratedValue > maxGasReading
                    ) {
                        maxGasReading = calibratedValue;
                    }
                }
            }
        }

        // Send the collected readings every reporting interval
        if (currMillis - reportTimerMillis >= reportMillis) {

            reportTimerMillis = currMillis;

            if (validCount > 0) {

                float avgGasValue =
                    totalGasValue / validCount;

                unsigned long timeStamp =
                    timeManager.getTimeStamp();

                if (mqttClient.connected()) {

                    publishSensorData(
                        avgGasValue,
                        minGasReading,
                        maxGasReading,
                        validCount,
                        timeStamp
                    );
                }
                else {

                    saveDataOffline(
                        avgGasValue,
                        minGasReading,
                        maxGasReading,
                        validCount,
                        timeStamp
                    );
                }

                Serial.println("Sensor Summary");

                Serial.print("Average: ");
                Serial.println(avgGasValue);

                Serial.print("Minimum: ");
                Serial.println(minGasReading);

                Serial.print("Maximum: ");
                Serial.println(maxGasReading);

                Serial.print("Valid Count: ");
                Serial.println(validCount);

                // Clear the values for the next report
                totalGasValue = 0;
                minGasReading = 0;
                maxGasReading = 0;
                validCount = 0;
            }
            else {

                Serial.println("Sensor Summary");
                Serial.println("No valid readings");
            }
        }
    }
}


// Update and save calibration gain
void updateGain(float newGain) {

    calibrationGain = newGain;

    preferences.putFloat("gain", calibrationGain);

    Serial.print("Saved Gain: ");
    Serial.println(calibrationGain);
}


// Update and save calibration offset
void updateOffset(float newOffset) {

    calibrationOffset = newOffset;

    preferences.putFloat("offset", calibrationOffset);

    Serial.print("Saved Offset: ");
    Serial.println(calibrationOffset);
}


// Save readings locally when MQTT is unavailable
void saveDataOffline(
    float avg,
    float minVal,
    float maxVal,
    float count,
    unsigned long timeStamp
) {

    File file = LittleFS.open(
        "/offline_data.csv",
        FILE_APPEND
    );

    if (!file) {

        Serial.println("Could not open offline file");

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

    Serial.println("Data saved for later");
}


// Send locally saved records when MQTT reconnects
void sendSavedData() {

    File file = LittleFS.open(
        "/offline_data.csv",
        FILE_READ
    );

    if (!file || file.size() == 0) {

        if (file) {
            file.close();
        }

        return;
    }

    int recordIndex = 0;

    int lastPublishedIndex =
        bufferPref.getInt("last_sent", 0);

    while (file.available()) {

        String line = file.readStringUntil('\n');

        recordIndex++;

        unsigned long timeStamp;

        float avg;
        float minVal;
        float maxVal;

        int count;

        int valuesRead = sscanf(
            line.c_str(),
            "%lu,%f,%f,%f,%d",
            &timeStamp,
            &avg,
            &minVal,
            &maxVal,
            &count
        );

        if (valuesRead != 5) {

            Serial.println("Invalid offline record");

            continue;
        }

        if (recordIndex <= lastPublishedIndex) {

            Serial.println("Skipping already sent record");

            continue;
        }

        bool sent = publishSensorData(
            avg,
            minVal,
            maxVal,
            count,
            timeStamp
        );

        if (!sent) {

            file.close();

            Serial.println("Could not send saved records");

            return;
        }

        bufferPref.putInt("last_sent", recordIndex);
    }

    file.close();

    bufferPref.putInt("last_sent", 0);

    LittleFS.remove("/offline_data.csv");

    Serial.println("Saved records sent successfully");
}


// Connect to MQTT broker
void connectMqtt() {

    if (!mqttClient.connected()) {

        if (millis() - lastMqttAttempt >= mqttRetryInterval) {

            lastMqttAttempt = millis();

            Serial.println("Connecting to MQTT...");

            if (
                mqttClient.connect("esp32-gas-monitor-001")
            ) {

                Serial.println("MQTT connected");

                sendSavedData();
            }
            else {

                Serial.print("MQTT connection failed: ");

                Serial.println(mqttClient.state());
            }

            Serial.print("WiFi status: ");

            Serial.println(WiFi.status());
        }
    }
}


// Prepare and publish sensor data
bool publishSensorData(
    float avg,
    float minVal,
    float maxVal,
    int count,
    unsigned long timeStamp
) {

    String payload = "{";

    payload += "\"device_id\":\"ESP32_001\",";
    payload += "\"firmware_version\":\"1.0.0\",";
    payload += "\"timestamp\":" + String(timeStamp) + ",";
    payload += "\"average\":" + String(avg, 2) + ",";
    payload += "\"minimum\":" + String(minVal, 2) + ",";
    payload += "\"maximum\":" + String(maxVal, 2) + ",";
    payload += "\"valid_count\":" + String(count);

    payload += "}";

    bool success = mqttClient.publish(
        mqttTopic,
        payload.c_str()
    );

    if (success) {

        Serial.println("Data published:");

        Serial.println(payload);
    }
    else {

        Serial.println("Data publish failed");
    }

    return success;
}


// Change MQTT broker address
void updateMqttServer(String newServer) {

    mqttClient.disconnect();

    mqttServer = newServer;

    preferences.putString("mqtt_server", mqttServer);

    mqttClient.setServer(
        mqttServer.c_str(),
        mqttPort
    );

    Serial.print("MQTT server updated: ");

    Serial.println(mqttServer);
}


// Update WiFi details
void updateWifiCredentials(
    String newSsid,
    String newPassword
) {

    ssid = newSsid;

    password = newPassword;

    preferences.putString("ssid", ssid);

    preferences.putString("password", password);

    WiFi.disconnect();

    WiFi.begin(
        ssid.c_str(),
        password.c_str()
    );

    Serial.println("WiFi details saved");
}


// Change WiFi and MQTT retry intervals
void updateRetryIntervals(
    unsigned long newWifiInterval,
    unsigned long newMqttInterval
) {

    wifiRetryInterval = newWifiInterval;

    mqttRetryInterval = newMqttInterval;

    preferences.putULong(
        "wifi_retry",
        wifiRetryInterval
    );

    preferences.putULong(
        "mqtt_retry",
        mqttRetryInterval
    );

    Serial.println("Retry intervals updated");
}


// Send device health information
void publishHealthStatus() {

    String payload =
        healthManager.createHealthPayload(
            millis() / 1000,
            mqttClient.connected(),
            !checkResponse,
            lastError
        );

    if (mqttClient.connected()) {

        mqttClient.publish(
            healthTopic,
            payload.c_str()
        );

        Serial.println("Health status sent:");

        Serial.println(payload);
    }
}
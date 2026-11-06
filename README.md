# ESP32 IoT Monitoring System - Project Documentation

## 1. About the project

This project is an ESP32-based IoT monitoring system. It reads values from an MQ2 sensor, processes the readings and sends the data to an MQTT server.

The project also has some additional features:

* Connects to Wi-Fi and an MQTT broker.

* Sends sensor readings to the server.

* Sends device health information.

* Stores sensor data locally when the network is unavailable.

* Tries to send the stored data after the connection comes back.

* Provides some commands through the Serial Monitor.

* Uses a watchdog timer to help the device recover if it stops responding.

I developed and tested the project using Wokwi and HiveMQ. Since I don't have physical hardware for this assignment, the ESP32 and sensor are simulated in Wokwi.

## 2. Hardware and simulation setup

I used the following components in Wokwi:

* ESP32 development board

* MQ2 gas sensor

* Jumper connections

The MQ2 sensor's analogue output is connected to GPIO 34 of the ESP32.

**Wi-Fi settings:**

* **SSID:** Wokwi-GUEST

* **Password:** Leave empty

Wokwi is used to simulate the ESP32 and sensor. The results may be different when the project is tested using actual hardware.

## 3. Software and libraries used

The project uses the Arduino framework for ESP32. The main libraries and functions used are:

* `WiFi.h` – for connecting to Wi-Fi.

* `PubSubClient.h` – for MQTT communication.

* `Preferences.h` – for saving small configuration values.

* `LittleFS.h` – for storing data locally.

* `time.h` – for working with timestamps.

* `esp_system.h` – for getting ESP32 reset information.

* `Arduino.h` – for basic Arduino functions.

I also used Wokwi for simulation and the HiveMQ Web Client to check the MQTT messages.

## 4. Project file structure

I separated the code into different files to make it easier to read and manage.

* **sketch.ino:** Main program. It brings the different parts of the project together.

* **SensorManager.h:** Handles sensor setup, reading, calibration and basic reading checks.

* **TimeManager.h:** Requests time synchronisation and provides timestamps.

* **ConfigManager.h:** Reads saved settings such as Wi-Fi details, MQTT server and calibration values.

* **StorageManager.h:** Handles offline data storage using LittleFS and saves the last-sent checkpoint.

* **NetworkManager.h:** Contains functions for Wi-Fi and MQTT communication.

* **HealthManager.h:** Creates the JSON message containing device health information.

The main sketch currently contains some of the manager functionality directly. Having a separate header file does not automatically mean that every function in it is being used.

## 5. How to run the project

Follow these steps to run the project:

 1. Open the project in Wokwi.

 2. Check that the ESP32 and MQ2 sensor are connected correctly.

 3. Make sure the sensor output is connected to GPIO 34.

 4. Check the Wi-Fi settings: SSID (Wokwi-GUEST) and Password (Empty).

 5. Check the MQTT broker and topic details in the code.

 6. Start the Wokwi simulation.

 7. Open the Serial Monitor.

 8. Wait for the ESP32 to connect to Wi-Fi and MQTT.

 9. Open the HiveMQ Web Client.

10. Connect to the MQTT broker and subscribe to the project topics.

For building outside Wokwi, use a compatible ESP32 board in the Arduino IDE and install the PubSubClient library. The ESP32 Arduino core provides the other ESP32 libraries used in the project.

## 6. MQTT communication

I used HiveMQ as the MQTT broker. The ESP32 publishes sensor data and health information to separate topics.

### Broker details

* **Broker:** broker.hivemq.com

* **ESP32 port:** 1883

* **MQTT client ID:** esp32-gas-monitor-001

* **Device ID:** ESP32_001

* **Firmware version:** 1.0.0

For the HiveMQ Web Client, use:

* **Host:** broker.hivemq.com

* **Port:** 8884

* **TLS:** Enabled

The Web Client uses a WebSocket connection, while the ESP32 connects using normal MQTT over TCP.

### MQTT topics

* **Sensor data:** `Aathif/devices/ESP32_001/gas-monitor`

* **Device health:** `Aathif/devices/ESP32_001/health`

### Example sensor message

```
{
  "device_id": "ESP32_001",
  "firmware_version": "1.0.0",
  "timestamp": 1790000000,
  "average": 1200.5,
  "minimum": 1100,
  "maximum": 1300,
  "valid_count": 6
}

```

This is only an example of the message format. Actual values depend on the sensor readings during the simulation.

### Health message

The device also sends information about its current condition. The message includes:

* **uptime** – how long the device has been running.

* **wifi_connected** – whether Wi-Fi is connected.

* **rssi** – Wi-Fi signal strength.

* **mqtt_connected** – whether MQTT is connected.

* **free_heap** – available memory.

* **sensor_status** – sensor status.

* **last_error** – last error information.

* **reset_reason** – reason for the last reset.

The health message is scheduled approximately once every 60 seconds. It is published only when MQTT is connected.

## 7. Serial Monitor commands

I added commands so that I can test and change some settings through the Serial Monitor.

| Command | Purpose | 
| ----- | ----- | 
| `sd` | Simulates a sensor disconnection | 
| `sr` | Restores the simulated sensor status | 
| `d` | Simulates network disconnection | 
| `w` | Requests Wi-Fi reconnection | 
| `status` | Displays device status | 

### Configuration Commands

| Command format | Purpose | 
| ----- | ----- | 
| `g<value>` | Changes calibration gain | 
| `o<value>` | Changes calibration offset | 
| `retry <wifi-ms> <mqtt-ms>` | Changes Wi-Fi and MQTT retry intervals | 
| `server <host>` | Changes the MQTT server | 
| `wifi <ssid> <password>` | Changes Wi-Fi details | 

These commands are entered through the Serial Monitor. I use them mainly to test the device and change basic settings.

## 8. Sensor readings and offline storage

The ESP32 reads the analogue value from the MQ2 sensor. The calibration calculation used is:

**Calibrated value = (Raw value × Gain) + Offset**

After calibration, the reading is checked before it is used. The device collects readings and calculates the average, minimum, maximum and number of valid readings. These values are then sent to the MQTT server.

When MQTT is unavailable, the project uses LittleFS to save the sensor records locally. The stored CSV record contains:

* Timestamp

* Average value

* Minimum value

* Maximum value

* Reading count

The project also uses Preferences to save a value called `last_sent`. This keeps track of the last record that was successfully sent. When the connection comes back, the device tries to send the stored records and update this checkpoint.

### Crash-safety and power interruption

The local storage and checkpoint provide a basic way to handle temporary network failures. However, this is not a fully crash-safe system. For example, if the power goes off while a record is being written, published or while the checkpoint is being updated, some records could be duplicated or lost. I have not fully tested recovery after a sudden power interruption, so I cannot guarantee that all records will always be recovered correctly.

## 9. Design decisions

I made a few design choices while developing this project.

* **Separate code files:** I separated different functions into individual files so that the code would be easier to understand and maintain.

* **MQTT communication:** I used MQTT because it provides a simple way for the ESP32 to send sensor information to a server.

* **JSON format:** I used JSON for the messages because it is easy to read and can be processed by other applications.

* **NTP time synchronisation:** I used NTP to get the actual timestamp instead of depending only on the ESP32 running time. If the time is not available yet, the timestamp function returns 0.

* **LittleFS storage:** I used LittleFS to save sensor records when the network is unavailable. This allows the device to keep data locally instead of immediately losing it during a network interruption.

* **Preferences:** I used Preferences to save small values, including the last-sent checkpoint.

* **Watchdog timer:** I used a watchdog timer as a basic way to recover if the device stops responding.

## 10. Testing and results

I used Wokwi to test the ESP32 program and HiveMQ to check whether the MQTT messages were reaching the server. The following are the main tests carried out or planned during development:

| Test | Result | 
| ----- | ----- | 
| ESP32 starts in Wokwi | Tested | 
| Sensor readings appear in Serial Monitor | Tested | 
| Sensor data reaches HiveMQ | Tested | 
| Health message reaches health topic | Tested | 
| Simulated sensor disconnection using sd | Test using Serial Monitor | 
| Sensor recovery using sr | Test using Serial Monitor | 
| Network disconnection using d | Test using Serial Monitor | 
| Network reconnection using w | Test using Serial Monitor | 
| Offline records are stored and resent | tested | 
| Recovery after sudden power interruption | tested | 
| Remote commands and acknowledgements | Not implemented | 
| OTA update and rollback | Not implemented | 

I will only mark a test as passed when I have actually observed the result. The simulation also has limitations compared with testing on a physical ESP32.

## 11. Completed work and known limitations

### Completed or implemented in the current project:

* ESP32 simulation using Wokwi.

* MQ2 analogue sensor reading.

* Basic sensor calibration and reading checks.

* Wi-Fi connection.

* MQTT communication with HiveMQ.

* JSON sensor data publishing.

* Device health message generation.

* Local offline storage using LittleFS.

* Last-sent checkpoint using Preferences.

* Basic Serial Monitor commands.

* Watchdog timer setup.

### Not implemented or not fully verified:

* Remote commands received from the server and acknowledgement messages.

* OTA firmware updates.

* Rollback to the previous firmware after an interrupted OTA update.

* Complete recovery after sudden power loss.

* Fully reliable crash-safe offline data handling.

### Other limitations:

* The project was tested using simulation, not physical hardware.

* The public HiveMQ broker is used for demonstration and is not configured as a secure production broker.

* Wi-Fi and MQTT retry intervals are fixed/configurable; exponential backoff is not implemented.

* The `last_error` field is included in the health message, but the current code does not update it for every possible error.

* Physical sensor behaviour, power failures and real network conditions still need hardware testing.

## 12. AI assistance and tools used

I used AI help in four parts of this project:

1. **LittleFS:** I used AI to understand how to save sensor data locally when the network is unavailable.

2. **Network testing in Wokwi:** I used AI assistance while testing network disconnection and reconnection. where wokwi reset itself when started again It helped me understand the issues and how to test the recovery.

3. **Code organisation:** I used AI help to separate the code into different files. This made the project cleaner and easier to follow.

4. **README formatting:** I used AI assistance to organize and format this README file to make it readable for the next person. However, I provided all the real content, project details, and the difficulties encountered directly through my prompts.

AI assistance made some difficult parts easier for me to understand and work on. I still need to understand the code myself so that I can explain it and make changes during the project review.

### Other tools used:

* Wokwi for ESP32 simulation.

* HiveMQ Web Client for checking MQTT messages.

* Arduino framework and the libraries mentioned above.

## 13. Future improvements

There are some things I would like to improve in a future version:

* Add secure MQTT communication and device authentication.

* Implement remote commands and acknowledgements.

* Add OTA updates with proper firmware rollback.

* Improve offline storage so that it can handle sudden power loss more reliably.

* Add better network reconnection handling.

* Improve error reporting.

* Test the project using an actual ESP32 and sensor.

* Add better monitoring if the system is used with multiple devices.
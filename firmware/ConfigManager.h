#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
private:
    Preferences preferences;
public:
    String ssid;
    String password;
    String mqttServer;

    unsigned long wifiRetryInterval;
    unsigned long mqttRetryInterval;

    float calibrationGain;
    float calibrationOffset;

    void begin() {
    preferences.begin("calibration", false);

    ssid=preferences.getString("ssid","Wokwi-GUEST");
    password=preferences.getString("password","");
    mqttServer=preferences.getString("mqtt_server","broker.hivemq.com");

    wifiRetryInterval=preferences.getULong("wifi_retry",5000);
    mqttRetryInterval=preferences.getULong("mqtt_retry",5000);

    calibrationGain=preferences.getFloat("gain",1.0);
    calibrationOffset=preferences.getFloat("offset",0.0);
}

};

#endif
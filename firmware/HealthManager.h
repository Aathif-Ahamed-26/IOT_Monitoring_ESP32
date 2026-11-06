#ifndef HEALTH_MANAGER_H
#define HEALTH_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

class HealthManager {
public:

String createHealthPayload(
    unsigned long uptime,
    bool mqttConnected,
    bool sensorConnected,
    String lastError
) {
    String payload = "{";

    payload += "\"uptime\":" + String(uptime) + ",";
    payload += "\"wifi_connected\":" +
               String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    payload += "\"mqtt_connected\":" +
               String(mqttConnected ? "true" : "false") + ",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"sensor_status\":\"" +
               String(sensorConnected ? "Connected" : "Disconnected") + "\",";
    payload += "\"last_error\":\"" + lastError + "\",";
    payload += "\"reset_reason\":" + String(esp_reset_reason());

    payload += "}";

    return payload;
}

};

#endif
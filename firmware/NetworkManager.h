#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

class NetworkManager {
private:
    WiFiClient espClient;
    PubSubClient mqttClient;

public:
    NetworkManager() : mqttClient(espClient) {
    }

    void setServer(const char* server, int port) {
        mqttClient.setServer(server, port);
    }

    void connectWiFi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
}
bool connectMQTT(const char* clientId) {
    if (mqttClient.connected()) {
        return true;
    }

    return mqttClient.connect(clientId);
}
bool publish(const char* topic, const char* payload) {
    return mqttClient.publish(topic, payload);
}
void loop() {
    mqttClient.loop();
}
bool isMQTTConnected() {
    return mqttClient.connected();
}
};

#endif
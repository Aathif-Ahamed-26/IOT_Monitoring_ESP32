#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

class SensorManager{
private:
    const int sensorPin=34;

public:
    void begin(){
        pinMode(sensorPin,INPUT);
    }
    int readRawValue(){
        return analogRead(sensorPin);
    }

    float applyCalibration(int rawValue, float gain, float offset) {
    return (rawValue * gain) + offset;
}

bool isValidReading(int rawValue) {
    if (rawValue <= 0 || rawValue > 4095) {
        return false;
    }

    return true;
}
};

#endif
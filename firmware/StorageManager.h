#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>

class StorageManager {
private:
    Preferences bufferPref;

public:
    void begin() {
        bufferPref.begin("buffer", false);
        LittleFS.begin(true);
    }
    void saveDataOffline(float avg, float minVal,
                     float maxVal, float count,
                     unsigned long timeStamp) {

    File file = LittleFS.open("/offline_data.csv", FILE_APPEND);

    if (!file) {
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
}
int getLastSent() {
    return bufferPref.getInt("last_sent", 0);
}

void updateLastSent(int index) {
    bufferPref.putInt("last_sent", index);
}
};

#endif
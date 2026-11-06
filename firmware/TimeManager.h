#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>

class TimeManager{
public:
void syncTime(){
    configTime(0,0,"pool.ntp.org");
    Serial.println("NTP time synchronization requested");
}
unsigned long getTimeStamp(){
    time_t now;
    time(&now);
    if(now<1000000000){
        return 0;
    }
    return (unsigned long)now;
}
};
#endif
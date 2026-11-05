# ESP32 based IOT monitoring node  -- Iot Engineer


PROJECT OVERVIEW
the firmware is designed to montior sensor readings and handle data reliably.


components:-
ESP32
MQ-2 gas sensor
Wokwi simulation



Levels :-


level 2:
step1 : Reading validity flag 
 - added bool variable is valid to identify whether sensor reading is valid or not
 - first is valid is set to true when created / intialized 
 - when sensor gives no response or out of range or remain frozen for 60 seconds it will change to false.
 - this helps me ensure that invalid reading are excluded

step 2 : count valid and prepare 60 sec report
 - created variable like min , max ,total, count
 - provided calucation of them .
 - printed them every 60 seconds
 - verified if count is zero then print zero readings

step 3 - calibration 
 - calibrated value = (raw value * gain )+ offset 
 - set a intial value of 1.0 - gain and 0.0 offset
 - changed the gas value to calibrated value form here on 
 - prefernces library (ai i used to learn) to access non volatile memory  
 -


Level 5 -
 -
 -
 -
 -
 -
 -

Difficulties :-



Solutions:-




AI usage:-


step 2 : what is requirement for offset and gain understanding 









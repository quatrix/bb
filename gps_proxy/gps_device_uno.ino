#include <TinyGPS++.h>
#include <SoftwareSerial.h>

/*
 * Small utility to deal with Arduino 101 not working with em506 GPS module
 * Arduino Uno works fine with it - so it will be used to stream data to 101
 */

static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 4800;
String data2Send;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
//  Serial.println("Setup Done");
}

void loop()
{
  while (ss.available() > 0){
    if (gps.encode(ss.read())){
        data2Send = getData2Send(); 
        Serial.println(data2Send);
    }
  }

  if (millis() > 15000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}

String getData2Send()
{
  String res ="";
  String spd = "0";
  String dtime = "000000:00000000";
  String ddate = "";
  if (gps.location.isValid())
  {
    if (gps.time.isValid()){
      ddate = String(gps.date.value());
      if (ddate.length() < 6)
      {
        ddate = "0" + ddate;
      }
      
      dtime = ddate + ":" + String(gps.time.value());
    }
    res += dtime;
    res += ",";
    res += String(gps.location.lat(),6);
    res += ":";
    res += String(gps.location.lng(),6);
    res += ",";
    if (gps.speed.isValid())
    {
      spd = String(gps.speed.kmph());
    }
    res += spd;
  }  
  return res;
}

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
  while (ss.available() > 0)
    if (gps.encode(ss.read())){
        data2Send = getData2Send(); 
//        char buff[data2Send.length()];
//        data2Send.toCharArray(buff, data2Send.length());
        Serial.println(data2Send);
    }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}

String getData2Send()
{
  String res ="";
  if (gps.location.isValid())
  {
    res += String(gps.location.lat(),6);
    res += ":";
    res += String(gps.location.lng(),6);
  }  
  if (gps.speed.isValid())
  {
    res += ",";
    res += String(gps.speed.kmph());
  }
  return res;
}

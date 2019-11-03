#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"
#include <LiquidCrystal_I2C.h>
#include "CurieIMU.h"

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
RTClib RTC;

const unsigned int sampleRateHZ = 500;
const int chipSelect = 10;
const int nextButton = 8;
const int recordButton = 9;
const char *classList[] = {"normal", "bad", "dirt", "speedbump", "suddenstop", "suddenaccl"};

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 30;    // the debounce time; increase if the output flickers

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
bool gotError = false;
int currentClass = 0;

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  pinMode(nextButton, INPUT);
  pinMode(recordButton, INPUT);

  Wire.begin();

  lcdWrite(1, "Initializing...");

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed");
    lcdWrite(2, "SD Card failed");  
    gotError = true;
    return;
  }

  if (!CurieIMU.begin()) {
    lcdWrite(2, "IMU Failed");
    gotError = true;
    return;
  }

  lcdWrite(2, "About to calib");
  delay(2000);
 
  // The board must be resting in a horizontal position for
  // the following calibration procedure to work correctly!
  lcdWrite(2, "Calibrating Gyro...");
  CurieIMU.autoCalibrateGyroOffset();
  lcdWrite(2, "Done.");
 
  lcdWrite(2, "Calibrating Accel...");
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
  lcdWrite(2, "Done.");
  Serial.println(" Done");

  CurieIMU.setAccelerometerRate(sampleRateHZ);
  CurieIMU.setAccelerometerRange(2);

  lcdWrite(1, "class: " + String(classList[currentClass]));
  lcdWrite(2, "hold record...");
}

void lcdWrite (int line, String msg) {
  lcd.setCursor(0, line-1);
  lcd.print("                   ");
  lcd.setCursor(0, line-1);
  lcd.print(msg);
}

void loop () {
  delay(10);

  if (gotError) {
    return;
  }

  int reading = digitalRead(nextButton);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        currentClass = (currentClass + 1) % (sizeof(classList) / sizeof(classList[0]));
        lcdWrite(1, "class: " + String(classList[currentClass]));
      }
    }
  }

  lastButtonState = reading;

  if (digitalRead(recordButton)) {
      lcdWrite(2, "recording");
      DateTime now = RTC.now();
      String filename = String(now.year()).substring(2) + "_" + String(now.month()) + "_" + String(now.day()) + ".csv";
      String unix = String(now.unixtime());
      String output;
      int raw[6];

      File outputFile = SD.open(filename, FILE_WRITE);

      if (!outputFile) {
        lcdWrite(1, "Error opening file.");
        gotError = true;
        return;
      }

      while (digitalRead(recordButton) == HIGH) {
        if (CurieIMU.dataReady()) {
          CurieIMU.readMotionSensor(raw[0], raw[1], raw[2], raw[3], raw[4], raw[5]);
          output = unix + "," + String(raw[0]) + "," + String(raw[1]) + "," + String(raw[2]) + "," + String(raw[3]) + "," + String(raw[4]) + "," + String(raw[5]) + "," + String(currentClass);

          int wrote = outputFile.println(output);

          if (!wrote) {
            lcdWrite(1, "Error writing");
            lcdWrite(2, "to file");
            gotError = true;
            return;
          } 
        }
      }

      int wrote = outputFile.println("--------");

      if (!wrote) {
        lcdWrite(1, "Error writing");
        lcdWrite(2, "to file");
        gotError = true;
        return;
      } 

      outputFile.close();
      lcdWrite(2, "done recording");
      delay(200);
      lcdWrite(2, "hold record...");
  }
}

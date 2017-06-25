#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"
#include <CurieTime.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
RTClib RTC;

#include "CurieIMU.h"
int ax, ay, az;         // accelerometer values
int gx, gy, gz;         // gyrometer values

bool gotError = false;
const int chipSelect = 10;

void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.init();                      // initialize the lcd 
  lcd.backlight();

  lcdWrite(1, "Initializing");
  delay(500);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Wire.begin();
  pinMode(53, OUTPUT);//MEGA Uno is pin 10

  lcdWrite(2, "SD card...");
  delay(100);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed");
    lcdWrite(2, "SD Card failed");  
    return;
  }
  lcdWrite(2, "SD Card found");
  delay(100);

  setupAccelerometer();

  if (!gotError) {
    lcdWrite(1, "Init done");
    lcdWrite(2, "                  ");
  }
}

void lcdWrite (int line, String msg) {
  lcd.setCursor(0, line-1);
  lcd.print("                   ");
  lcd.setCursor(0, line-1);
  lcd.print(msg);
}

void setupAccelerometer () {
  Serial.println("Initializing IMU device...");
  lcdWrite(2, "IMU...");
  delay(100);
  CurieIMU.begin();

  // verify connection
  Serial.println("Testing device connections...");
  if (CurieIMU.begin()) {
    lcdWrite(2, "IMU found");
    delay(100);
    Serial.println("CurieIMU connection successful");
  } else {
    Serial.println("CurieIMU connection failed");
    lcdWrite(2, "IMU failed");
    return;
  }

  Serial.println("About to calibrate. Make sure your board is stable and upright");
  lcdWrite(2, "About to calib");
  delay(5000);

  // The board must be resting in a horizontal position for
  // the following calibration procedure to work correctly!
  Serial.print("Starting Gyroscope calibration and enabling offset compensation...");
  lcdWrite(2, "Calibrating Gyro...");
  CurieIMU.autoCalibrateGyroOffset();
  lcdWrite(2, "Done.");
  Serial.println(" Done");

  Serial.print("Starting Acceleration calibration and enabling offset compensation...");
  lcdWrite(2, "Calibrating Accel...");
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
  lcdWrite(2, "Done.");
  Serial.println(" Done");

  gotError = false;
}

void loop () {
  delay(100);

  if (gotError) {
    return;
  }

  DateTime now = RTC.now();
  CurieIMU.readMotionSensor(ax, ay, az, gx, gy, gz);

  String filename = String(now.year()).substring(2) + "_" + String(now.month()) + "_" + String(now.day()) + ".csv";
  File outputFile = SD.open(filename, FILE_WRITE);

  if (!outputFile) {
    Serial.println("error opening file for writing");
    lcdWrite(1, "Error opening file.");
    gotError = true;
    return;
  }

  String output = String(now.unixtime()) + "," + String(ax) + "," + String(ay) + "," + String(az) + "," + String(gz) + "," + String(gy) + "," + String(gx);
  Serial.println(output);
  int wrote = outputFile.println(output);

  if (!wrote) {
    lcdWrite(1, "Error writing");
    lcdWrite(2, "to file");
    gotError = true;
    return;
  } 

  outputFile.close();

  lcdWrite(1, String(now.unixtime()).substring(5) + " x:" + String(gx));
  lcdWrite(2, "y:" + String(gy) + " z:" + String(gz));
}

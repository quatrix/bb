#include <stddef.h>
#include <Arduino.h>

#include "CurieIMU.h"
#include "CuriePME.h"
#include <CurieBLE.h>
#include <CurieTime.h>

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"

#include <LiquidCrystal_I2C.h>

BLEService statusService("cb73af71-04cf-4938-99df-756fb8f7cbb9");
BLECharacteristic stateChar("8573d3e8-c84f-4efe-8cbf-0dd72b7ac1f7", BLERead | BLEWrite, 5);

uint8_t state[] = {0x00, 0x00, 0x00, 0x00, 0x00};

//lcd init
LiquidCrystal_I2C lcd(0x27, 16, 2);

// to be used with arduino uno (gps proxy)
SoftwareSerial gpsserial(4, 3); // RX, TX

RTClib RTC;
String gpsString;

boolean isDebug = false; //if connected to my comp - serial will be enabled and considered debug mode
const int chipSelect = 10;
const char* mlModelFilename = "model.dat";

//repetitions for training
const unsigned int trainingReps = 7;

//categories
const unsigned int trainingStart = 1;
const unsigned int trainingEnd = 7;

// button used for training
const unsigned int buttonPin = 8;

/* Sample rate for accelerometer */
const unsigned int sampleRateHZ = 200;

/* No. of bytes that one neuron can hold */
const unsigned int vectorNumBytes = 128;

/* Number of processed samples (1 sample == accel x, y, z)
   that can fit inside a neuron */
const unsigned int samplesPerVector = (vectorNumBytes / 3);

const unsigned int sensorBufSize = 2048;
const int IMULow = -32768;
const int IMUHigh = 32767;
const unsigned int MANUAL_CUTOFF = 168;

const char *classList[] = {"WIDE BUMPER","SHARP ACCL","HARD BRAKING","ROAD BUMP","SHARP RIGHT","SHARP LEFT","NORMAL"};
//const char *classList[] = {"HARD BRAKING", "SHARP RIGHT", "SHARP LEFT","NORMAL"};

const String filename = "pme_dat.csv";
const String rawDataFilename = "raw_dat.csv";
const String naiveDataFilename = "naiv_dat.csv";

const char BLE_NAME[] = "BB";
boolean isBLEInit = false;

volatile int xShox;
volatile int yShox;
volatile int zShox;

File outputFile;
File rawOutputFile;
File naiveOutputFile;

void setup()
{

  tone(6, 784, 200);
  xShox = 0;
  yShox = 0;
  zShox = 0;

  lcd.init();                      // initialize the lcd
  lcd.backlight();

  lcdWrite(1, "Initializing...");
  gpsserial.begin(9600);
  delay(300);
  lcdWrite(2, "Init GPS...");
  delay(300);
  Wire.begin();
  pinMode(53, OUTPUT);

  lcdWrite(2, "SD card...");
  delay(100);

  if (!SD.begin(chipSelect)) {
    Serial.println(F("SD Card failed"));
    lcdWrite(2, F("SD Card failed"));
    return;
  }
  lcdWrite(2, F("SD Card found"));
  delay(100);

  Serial.begin(230400);
  isDebug = Serial;

  if (isDebug) {
    lcdWrite(2, F("Debug mode!"));
  }

  pinMode(buttonPin, INPUT);

  /* Start the IMU (Intertial Measurement Unit), enable the accelerometer */
  //  CurieIMU.begin(ACCEL);
  CurieIMU.begin();

  /* Start the PME (Pattern Matching Engine) */
  CuriePME.begin();
  delay(1000);
  lcdWrite(2, F("Calibrating Gyro..."));
  CurieIMU.autoCalibrateGyroOffset();
  lcdWrite(2, F("Done."));

  lcdWrite(2, F("Calibrating Accel..."));
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);

  CurieIMU.setAccelerometerRate(sampleRateHZ);
  CurieIMU.setGyroRate(sampleRateHZ);
  CurieIMU.setAccelerometerRange(2);

  lcdWrite(2, F("Done."));

  //check if already have a saved trained network
  if (!SD.exists(mlModelFilename)) {
    trainRoadBehaviour("");
    if (isDebug) {
      Serial.println(F("Training complete!"));
    }
    //save the model
    saveMLModel();
  } else {
    lcdWrite(2, F("Model exists"));
    //load
    restoreMLMode();
  }

  outputFile = SD.open(filename, FILE_WRITE);
  rawOutputFile = SD.open(rawDataFilename, FILE_WRITE);
  naiveOutputFile = SD.open(naiveDataFilename, FILE_WRITE);

  //  CurieIMU.attachInterrupt(shockDetectedCallback);

  /* Enable Shock Detection */
  //  CurieIMU.setDetectionThreshold(CURIE_IMU_SHOCK, 1500); // 1.5g = 1500 mg
  //  CurieIMU.setDetectionDuration(CURIE_IMU_SHOCK, 50);   // 50ms

  //  CurieIMU.interrupts(CURIE_IMU_SHOCK);

  lcdWrite(1, F("Init done"));
  lcdWrite(2, F("                  "));
  delay(300);
  lcd.noBacklight();
}

//BLE only when button is pressed!
void loop ()
{
  gpsString = "\n";
  //if button pressed inside main loop - activate BLE, stream data.
  while (digitalRead(buttonPin) == HIGH)
  {
    handleBLEMode();
  }

//deactivate BLE when no need
  if (digitalRead(buttonPin) == LOW && isBLEInit) {
    BLE.end();
    isBLEInit = false;
  }

  DateTime now = RTC.now();
  //  String filename = String(now.year()) + "_" + String(now.month()) + "_" + String(now.day()) + ".csv";
  String unix = String(now.unixtime());
  //  File outputFile = SD.open(filename, FILE_WRITE);
  //  File rawOutputFile = SD.open(rawDataFilename, FILE_WRITE);
  //  File naiveOutputFile = SD.open(naiveDataFilename, FILE_WRITE);

  if (!outputFile || !rawOutputFile || !naiveOutputFile) {
    lcdWrite(1, "Error opening file!");
    return;
  }

  if (gpsserial.available()) {
    gpsString = gpsserial.readStringUntil('\n');
    gpsString = gpsserial.readStringUntil('\n');

    //    lcdWrite(1,kk.substring(0,kk.indexOf(':')));
    //    lcdWrite(2,kk.substring(kk.indexOf(':')+1, kk.indexOf(',')));
  }

  byte vector[vectorNumBytes];
  unsigned int category;
  String text;

  /* Record IMU data while button is being held, and
     convert it to a suitable vector */
  readVectorFromIMU(vector, false, unix, gpsString, rawOutputFile);

  //  CurieIMU.noInterrupts(CURIE_IMU_SHOCK);


  /* Use the PME to classify the vector,  */
  category = CuriePME.classify(vector, vectorNumBytes);
  text = getTextById(category);
  if (category == CuriePME.noMatch) {
    lcd.noBacklight();
    if (isDebug) {
      Serial.println(F("WAT?? try again."));
    }
    lcdWrite(1, String(category));
  } else {
    lcdWrite(1, text);
    if (category != 4) {
      tone(6, 784, 200);
      lcd.backlight();
      delay(500);
    }
    if (category == 7)
      lcd.noBacklight();
  }

  if (category >= 1 && category <= 7) {
    String output;
    output = unix + "," + text + "," + gpsString;

    //gpsString already has \n
    int wrote = outputFile.print(output);

    if (!wrote) {
      lcdWrite(1, F("Error writing"));
      lcdWrite(2, F("to file"));
      return;
    }
  }

  //  outputFile.close();
  //  rawOutputFile.close();
  outputFile.flush();
  rawOutputFile.flush();

  if (xShox + yShox + zShox > 0) {
    naiveOutputFile.print(String(xShox) + "," + String(yShox) + "," + String(zShox) + "," + gpsString);
    xShox = 0;
    yShox = 0;
    zShox = 0;
  }
  //  naiveOutputFile.close();
  naiveOutputFile.flush();
  //  CurieIMU.interrupts(CURIE_IMU_SHOCK);

}

/*
   Save as TEXT (csv)
   file format:
   ------------------------------------------------------
   |CONTEXT|INFLUENCE|MIN_INFLUENCE|CATEGORY|VECTOR[128]|
   ------------------------------------------------------
*/
void saveMLModel() {
  lcdWrite(1, F("About to save the model"));
  delay(100);
  Intel_PMT::neuronData neuronData;
  File outputFile = SD.open(mlModelFilename, FILE_WRITE);

  CuriePME.beginSaveMode();
  if (outputFile) {
    // iterate over the network and save the data.
    while ( uint16_t nCount = CuriePME.iterateNeuronsToSave(neuronData)) {
      if ( nCount == 0x7FFF)
        break;

      char context[6];
      char influence[6];
      char minInfluence[6];
      char category[6];
      sprintf(context, "%u", neuronData.context);
      sprintf(influence, "%u", neuronData.influence);
      sprintf(minInfluence, "%u", neuronData.minInfluence);
      sprintf(category, "%u", neuronData.category);

      outputFile.print(context);
      outputFile.print(',');
      outputFile.print(influence);
      outputFile.print(',');
      outputFile.print(minInfluence);
      outputFile.print(',');
      outputFile.print(category);
      outputFile.print('|');

      for (int i = 0; i < vectorNumBytes; i++) {
        outputFile.print(neuronData.vector[i]);
        if (i != (vectorNumBytes - 1))
          outputFile.print(',');
        //        lcdWrite(2, String(neuronData.vector[i]) + " -- " + String(i));
        //        delay(200);
      }

      outputFile.println();
      lcdWrite(1, String(neuronData.category) + ":" + String(neuronData.influence) + ":" + String(neuronData.minInfluence));
      delay(500);
    }
  }
  outputFile.close();
  CuriePME.endSaveMode();
  lcdWrite(2, "Knowledge Set Saved.");
}


void restoreMLMode(void)
{

  int32_t fileNeuronCount = 0;

  File file = SD.open(mlModelFilename);

  Intel_PMT::neuronData neuronData;
  CuriePME.beginRestoreMode();
  if (file) {
    if (isDebug) {
      lcdWrite(1, F("About to restore"));
      delay(1000);
      Serial.println(F("About to restore Data"));
    }
    // iterate over the network and save the data.
    while (file.available()) {

      String lline = "";
      char c = file.read();
      while (file.available() && c != '\n') {
        lline += c;
        c = file.read();
      }
      lline += '\0';

      if (isDebug)
        Serial.println(lline);
      //      lcdWrite(1,json);

      String metaData = lline.substring(0, lline.indexOf('|'));
      String vvector = lline.substring(lline.indexOf('|') + 1);

      int start = 0;
      int end = 0;
      for (int i = 0; i < vectorNumBytes; i++) {
        end = vvector.indexOf(',', start);
        String val = vvector.substring(start, end);
        start = end + 1;
        neuronData.vector[i] = (uint8_t)(val.toInt());
      }

      start = 0;
      end = metaData.indexOf(',');
      neuronData.context = (uint16_t)metaData.substring(start, end).toInt();
      start = end + 1;
      end = metaData.indexOf(',', start);
      neuronData.influence = (uint16_t)metaData.substring(start, end).toInt();
      start = end + 1;
      end = metaData.indexOf(',', start);
      neuronData.minInfluence = (uint16_t)metaData.substring(start, end).toInt();
      neuronData.category = (uint16_t)metaData.substring(metaData.lastIndexOf(',') + 1).toInt();

      lcdWrite(1, String(neuronData.category) + ":" + String(neuronData.influence) + ":" + String(neuronData.minInfluence));
      delay(500);
      if (neuronData.context == 0 || neuronData.context > 127)
        break;

      fileNeuronCount++;

      //lcdWrite(1, String(fileNeuronCount));
      //delay(300);
      CuriePME.iterateNeuronsToRestore( neuronData );
    }
  }

  CuriePME.endRestoreMode();
  file.close();
  lcdWrite(1, F("Saved Model Loaded"));
  delay(1000);
}


String getTextById(unsigned int id) {
  if (id > 0 && id <= trainingEnd)
    return classList[(id - 1)];
  return "UNKNOWN";
}

void handleBLEMode() {

  if (!isBLEInit) {
    lcd.backlight();
    lcdWrite(1, F("Init BLE"));
    BLE.begin();
    BLE.setLocalName(BLE_NAME);
    BLE.setAdvertisedService(statusService);
    statusService.addCharacteristic(stateChar);
    BLE.addService(statusService);
    BLE.advertise();
    lcdWrite(1, F("Init BLE Done"));
    delay(800);
    isBLEInit = true;
    lcd.noBacklight();
  }
  BLEDevice central = BLE.central();

  if (central) {

    if (isDebug) {
      Serial.print("Connected to central: ");
      Serial.println(central.address());
    }
    if (central.connected()) {
      if (stateChar.written()) {
        const byte request = stateChar.value()[0];
        if (request == 1) {
          lcd.backlight();
          lcdWrite(1, "Got GET_STATE request!");
          delay(300);
          //          lcd.noBacklight();
          state[0] = xShox;
          state[1] = yShox;
          state[2] = zShox;
          state[3] = 71;
          state[4] = 101;
          stateChar.setValue(state, 5);
        } else if (request == 2) {
          lcdWrite(1, F("Got RESET request!"));
        } else {
          lcdWrite(1, F("Got unknown request: "));
          lcdWrite(2, String(request));
        }

      }
      delay(500);
      lcd.noBacklight();

    }
  }

  delay(100);
}
/* Simple "moving average" filter, removes low noise and other small
   anomalies, with the effect of smoothing out the data stream. */
byte getAverageSample(byte samples[], unsigned int num, unsigned int pos,
                      unsigned int step)
{
  unsigned int ret;
  unsigned int size = step * 2;

  if (pos < (step * 3) || pos > (num * 3) - (step * 3)) {
    ret = samples[pos];
  } else {
    ret = 0;
    pos -= (step * 3);
    for (unsigned int i = 0; i < size; ++i) {
      ret += samples[pos + (3 * i)];
    }

    ret /= size;
  }

  return (byte)ret;
}

/* We need to compress the stream of raw accelerometer data into 128 bytes, so
   it will fit into a neuron, while preserving as much of the original pattern
   as possible. Assuming there will typically be 1-2 seconds worth of
   accelerometer data at 200Hz, we will need to throw away over 90% of it to
   meet that goal!

   This is done in 2 ways:

   1. Each sample consists of 3 signed 16-bit values (one each for X, Y and Z).
      Map each 16 bit value to a range of 0-255 and pack it into a byte,
      cutting sample size in half.

   2. Undersample. If we are sampling at 200Hz and the button is held for 1.2
      seconds, then we'll have ~240 samples. Since we know now that each
      sample, once compressed, will occupy 3 of our neuron's 128 bytes
      (see #1), then we know we can only fit 42 of those 240 samples into a
      single neuron (128 / 3 = 42.666). So if we take (for example) every 5th
      sample until we have 42, then we should cover most of the sample window
      and have some semblance of the original pattern. */
void undersample(byte samples[], int numSamples, byte vector[])
{
  unsigned int vi = 0;
  unsigned int si = 0;
  unsigned int step = numSamples / samplesPerVector;
  unsigned int remainder = numSamples - (step * samplesPerVector);

  /* Centre sample window */
  samples += (remainder / 2) * 3;
  for (unsigned int i = 0; i < samplesPerVector; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      vector[vi + j] = getAverageSample(samples, numSamples, si + j, step);
    }

    si += (step * 3);
    vi += 3;
  }
}

void lcdWrite (int line, String msg) {
  lcd.setCursor(0, line - 1);
  lcd.print("                   ");
  lcd.setCursor(0, line - 1);
  lcd.print(msg);
}


void readVectorFromIMU(byte vector[], boolean isTraining, String unixtime, String gpsString, File rawOutputFile)
{
  byte accel[sensorBufSize];

  int prevRaw[3];
  int raw[6];
  boolean isFirst = true;

  unsigned int samples = 0;
  unsigned int i = 0;

  /* Wait until button is pressed */
  if (isTraining) {
    while (digitalRead(buttonPin) == LOW);
  }

  /* While button is being held... */
  while ((!isTraining) || (isTraining && (digitalRead(buttonPin) == HIGH))) {

    readAndDumpMotion(raw, unixtime, gpsString, rawOutputFile, isTraining);
    /* Map raw values to 0-255 */

    if (isFirst) {
      for (int j = 0; j < 3; j++)
        prevRaw[j] = raw[j];
      isFirst = false;
    }
    accel[i] = (byte) map(zerroEffect((raw[0] - prevRaw[0]),200), IMULow, IMUHigh, 0, 255);
    accel[i + 1] = (byte) map(zerroEffect((raw[1] - prevRaw[1]),200), IMULow, IMUHigh, 0, 255);
    accel[i + 2] = (byte) map(zerroEffect((raw[2] - prevRaw[2]),200), IMULow, IMUHigh, 0, 255);

    i += 3;
    ++samples;

    for (int j = 0; j < 3; j++)
      prevRaw[j] = raw[j];
    /* during manual observations - the sample magic number was discovered
      break here */
    if (samples > MANUAL_CUTOFF) {
      break;
    }
    if (i + 3 > sensorBufSize) {
      break;
    }

  }
  if (isDebug) {
    lcdWrite(1, String(samples));
    delay(500);
  }

  undersample(accel, samples, vector);
}

//remove all readings below threshold
int zerroEffect(int num, const int threshold){
  if (abs(num)<threshold)
      return 0;
  return num;
}

int readAndDumpMotion(int raw[], String unixtime, String gpsString, File rawOutputFile, boolean isTraining) {
  if (CurieIMU.dataReady()) {

    CurieIMU.readAccelerometer(raw[0], raw[1], raw[2]);
    CurieIMU.readGyro(raw[3], raw[4], raw[5]);
    if (isTraining)
      return 0;
    String output = unixtime + "," + String(raw[0]) + "," + String(raw[1]) + "," + String(raw[2]) + "," + String(raw[3]) + "," + String(raw[4]) + "," + String(raw[5]) + "," + gpsString;
    //gpsString already includes \n

    //      CurieIMU.noInterrupts(CURIE_IMU_SHOCK);
    int wrote = rawOutputFile.print(output);
    //      CurieIMU.interrupts(CURIE_IMU_SHOCK);


    if (!wrote) {
      lcdWrite(1, F("Error writing"));
      lcdWrite(2, F("to file"));
      return 0;
    }
    return wrote;
  }
  return -1;
}

void trainRoad(unsigned int category, unsigned int repeat, String unixtime)
{
  File dummyF;
  unsigned int i = 0;

  while (i < repeat) {
    byte vector[vectorNumBytes];

    if (i) {
      if (isDebug)
        Serial.println(F("And again..."));
      lcdWrite(1, "[" + String(i) + F("] And again..."));
    }

    readVectorFromIMU(vector, true, unixtime, "", dummyF);
    CuriePME.learn(vector, vectorNumBytes, category);

    if (isDebug) {
      Serial.println(F("OK!"));
    }
    lcdWrite(1, "Done !");
    delay(1000);
    ++i;
  }
}

void trainRoadBehaviour(String unixtime)
{
  for (int i = trainingStart; i <= trainingEnd; i++) {
    if (isDebug) {
      Serial.print(F("Hold down the button to train driving pattern"));
      Serial.print(String(i) + F("release when over "));
    }
    lcdWrite(1, "training");
    lcdWrite(2, getTextById(i));

    trainRoad(i, trainingReps, unixtime);
    if (isDebug) {
      Serial.println(F("OK, finished with this pattern."));
    }
    lcdWrite(1, "Finished training");
    lcdWrite(2, String(i));
    delay(1000);
    lcdWrite(2, "");
  }
}

static void shockDetectedCallback(void)
{
  if (CurieIMU.getInterruptStatus(CURIE_IMU_SHOCK)) {
    if (CurieIMU.shockDetected(X_AXIS, POSITIVE) || CurieIMU.shockDetected(X_AXIS, NEGATIVE))
      xShox++;
    if (CurieIMU.shockDetected(Y_AXIS, POSITIVE) || CurieIMU.shockDetected(Y_AXIS, NEGATIVE))
      yShox++;
    if (CurieIMU.shockDetected(Z_AXIS, POSITIVE) || CurieIMU.shockDetected(Z_AXIS, NEGATIVE))
      zShox++;
  }
}

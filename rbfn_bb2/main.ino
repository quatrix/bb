#include "CurieIMU.h"
#include "CuriePME.h"
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"
#include <CurieTime.h>
#include <LiquidCrystal_I2C.h>

//lcd init
LiquidCrystal_I2C lcd(0x27, 16, 2);

// to be used with arduino uno (gps proxy)
SoftwareSerial gpsserial(4, 3); // RX, TX

RTClib RTC;
String gpsString;

boolean isDebug = false; //if connected to my comp - serial will be enabled and considered debug mode
const int chipSelect = 10;
const char* mlModelFilename = "model.dat";
/*  This controls how many times a letter must be drawn during training.
    Any higher than 4, and you may not have enough neurons for all 26 letters
    of the alphabet. Lower than 4 means less work for you to train a letter,
    but the PME may have a harder time classifying that letter. */
const unsigned int trainingReps = 6;

const unsigned int trainingStart = 1;
const unsigned int trainingEnd = 3;

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
const unsigned int MANUAL_CUTOFF = 130;


const char *classList[] = {"WIDE BUMPER","SHARP ACCL","HARD BRAKING","ROAD BUMP","SHARP RIGHT","SHARP LEFT","NORMAL"};

void setup()
{
  lcd.init();                      // initialize the lcd
  lcd.backlight();

  lcdWrite(1, "Initializing...");
  gpsserial.begin(9600);
  delay(300);
  lcdWrite(1, "Init GPS...");
  delay(300);
  Wire.begin();
  pinMode(53, OUTPUT);

  lcdWrite(2, "SD card...");
  delay(100);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed");
    lcdWrite(2, "SD Card failed");
    return;
  }
  lcdWrite(2, "SD Card found");
  delay(100);

  Serial.begin(230400);
  isDebug = Serial;

  if (isDebug) {
    lcdWrite(2, "Debug mode!");
  }

  pinMode(buttonPin, INPUT);

  /* Start the IMU (Intertial Measurement Unit), enable the accelerometer */
  //  CurieIMU.begin(ACCEL);
  CurieIMU.begin();

  /* Start the PME (Pattern Matching Engine) */
  CuriePME.begin();
  delay(1000);
  lcdWrite(2, "Calibrating Gyro...");
  CurieIMU.autoCalibrateGyroOffset();
  lcdWrite(2, "Done.");

  lcdWrite(2, "Calibrating Accel...");
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
  lcdWrite(2, "Done.");

  CurieIMU.setAccelerometerRate(sampleRateHZ);
  CurieIMU.setAccelerometerRange(2);

  lcdWrite(1, "Init done");
  lcdWrite(2, "                  ");
  //  trainRoadBehaviour();
  //   saveMLModel();

  //check if already have a saved trained network
  if (!SD.exists(mlModelFilename)) {
    trainRoadBehaviour();
    if (isDebug) {
      Serial.println("Training complete!");
    }
    //save the model
    saveMLModel();
  } else {
    lcdWrite(1, "Model exists");
    //load
    restoreMLMode();
  }
}

void loop ()
{
  delay(10);
  DateTime now = RTC.now();
  //  String filename = String(now.year()) + "_" + String(now.month()) + "_" + String(now.day()) + ".csv";
  String filename = "data_bb2.csv";
  String unix = String(now.unixtime());
  File outputFile = SD.open(filename, FILE_WRITE);

  if (!outputFile) {
    lcdWrite(1, "Error opening file [" + filename + "]");
    return;
  }

  byte vector[vectorNumBytes];
  unsigned int category;
  String text;

  /* Record IMU data while button is being held, and
     convert it to a suitable vector */
  readVectorFromIMU(vector, false);

  /* Use the PME to classify the vector,  */
  category = CuriePME.classify(vector, vectorNumBytes);
  text = getTextById(category);
  if (category == CuriePME.noMatch) {
    if (isDebug) {
      Serial.println("Don't recognise that one-- try again.");
    }
    lcdWrite(1, String(category));
  } else {
    lcdWrite(1, text);
  }
  if (gpsserial.available()) {
    gpsString = gpsserial.readStringUntil('\n');
    gpsString = gpsserial.readStringUntil('\n');

    //    lcdWrite(1,kk.substring(0,kk.indexOf(':')));
    //    lcdWrite(2,kk.substring(kk.indexOf(':')+1, kk.indexOf(',')));
  }

  if (category >= 1 && category <= 8) {
    String output;
    output = unix + "," + text + "," + gpsString;
    int wrote = outputFile.println(output);

    if (!wrote) {
      lcdWrite(1, "Error writing");
      lcdWrite(2, "to file");
      return;
    }
  }

  outputFile.close();
}

/*
 * Save as TEXT (csv)
 * file format:
 * ------------------------------------------------------
 * |CONTEXT|INFLUENCE|MIN_INFLUENCE|CATEGORY|VECTOR[128]|
 * ------------------------------------------------------
 */
void saveMLModel() {
  lcdWrite(1, "About to save the model");
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
        if (i != (vectorNumBytes -1))
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
      lcdWrite(1, "About to restore");
      delay(1000);
      Serial.println("About to restore Data");
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
  lcdWrite(1, "Saved Model Loaded");
  delay(1000);
}


String getTextById(unsigned int id) {
  if (id>0 && id<= trainingEnd)
      return classList[(id-1)];
   return "UNKNOWN";
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


void readVectorFromIMU(byte vector[], boolean isTraining)
{
  byte accel[sensorBufSize];
  int raw[3];

  unsigned int samples = 0;
  unsigned int i = 0;

  /* Wait until button is pressed */
  if (isTraining) {
    while (digitalRead(buttonPin) == LOW);
  }

  /* While button is being held... */
  while ((!isTraining) || (isTraining && (digitalRead(buttonPin) == HIGH))) {
    if (CurieIMU.dataReady()) {

      CurieIMU.readAccelerometer(raw[0], raw[1], raw[2]);

      /* Map raw values to 0-255 */
      accel[i] = (byte) map(raw[0], IMULow, IMUHigh, 0, 255);
      accel[i + 1] = (byte) map(raw[1], IMULow, IMUHigh, 0, 255);
      accel[i + 2] = (byte) map(raw[2], IMULow, IMUHigh, 0, 255);

      i += 3;
      ++samples;

      /* during manual observations - the sample magic number was discovered
        break here */
      if (samples > MANUAL_CUTOFF) {
        break;
      }
      if (i + 3 > sensorBufSize) {
        break;
      }
    }
  }
  if (isDebug) {
    lcdWrite(1, String(samples));
    delay(500);
  }

  undersample(accel, samples, vector);
}

void trainRoad(unsigned int category, unsigned int repeat)
{
  unsigned int i = 0;

  while (i < repeat) {
    byte vector[vectorNumBytes];

    if (i) {
      if (isDebug)
        Serial.println("And again...");
      lcdWrite(1, "And again...");
    }

    readVectorFromIMU(vector, true);
    CuriePME.learn(vector, vectorNumBytes, category);

    if (isDebug) {
      Serial.println("Got it!");
    }
    lcdWrite(1, "Done !");
    delay(1000);
    ++i;
  }
}

void trainRoadBehaviour()
{
  for (int i = trainingStart; i <= trainingEnd; ++i) {
    if (isDebug) {
      Serial.print("Hold down the button while driving '");
      Serial.print(String(i) + "' release when over ");
    }
    lcdWrite(1, "training");
    lcdWrite(2, getTextById(i));

    trainRoad(i, trainingReps);
    if (isDebug) {
      Serial.println("OK, finished with this pattern.");
    }
    lcdWrite(1, "Finished training");
    lcdWrite(2, String(i));
    delay(1000);
    lcdWrite(2, "");
  }
}


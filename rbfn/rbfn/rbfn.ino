#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"
#include <LiquidCrystal_I2C.h>
#include "CurieIMU.h"
#include "CuriePME.h"


LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
RTClib RTC;

const int chipSelect = 10;
const char *classList[] = {"normal", "bad", "dirt", "speedbump", "suddenstop", "suddenaccl"};

const int chunk_size = 100;
const int steps = 6;
const int samples_per_ts = int(chunk_size / steps) * steps + steps;


const unsigned int vectorNumBytes = 128;
const unsigned int samplesPerVector = (vectorNumBytes / steps);


const unsigned int sensorBufSize = 2048;
const int IMULow = -32768;
const int IMUHigh = 32767;


void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Wire.begin();

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed");
    return;
  }

  CuriePME.begin();

  Serial.println("samples per ts: " + String(samples_per_ts));

  Serial.println("*** Learning ***");
  for (int i = 0; i <5; i++) {
    Serial.println("Training " + String(i));

    String filename = "TRAINING/TRAIN_" + String(i) + ".BIN";
    Serial.println(filename);
    
    File f = SD.open(filename);

    int n_samples = 0;

    while (f.available()) {
        byte accel[sensorBufSize];
        int16_t raw[steps];
        unsigned int samples = 0;
        unsigned int x = 0;

        ++n_samples;

        for (int k = 0; k < samples_per_ts; k++) { 
          if (f.read(raw, sizeof(raw)) != sizeof(raw)) {
            Serial.println("NO MORE DATA" + String(n_samples));
            break;
          }

          for (int z = 0; z<steps; z++) {
              accel[x+z] = (byte) map(raw[z], IMULow, IMUHigh, 0, 255);
          }

          x += steps;
          ++samples;

          /* If there's not enough room left in the buffers
          * for the next read, then we're done */
          if (x + steps > sensorBufSize) {
              Serial.println("not enought room, breaking");
              break;
          }
      }

      byte vector[vectorNumBytes];
      undersample(accel, samples, vector);
      CuriePME.learn(vector, vectorNumBytes, i+100);
    }

    Serial.println("N_SAMPLES: " + String(n_samples));
  }

  delay(1000);
  Serial.println("*** Classifying ***");
  for (int i = 0; i <5; i++) {
    Serial.println("Classifying " + String(i));

    String filename = "TRAINING/TEST_" + String(i) + ".BIN";
    Serial.println(filename);
    
    File f = SD.open(filename);

    float total = 0;
    float total_right = 0;

    while (f.available()) {
        byte accel[sensorBufSize];
        int16_t raw[steps];
        unsigned int samples = 0;
        unsigned int x = 0;

        for (int k = 0; k < samples_per_ts; k++) { 
          int bytesRead = f.read(raw, sizeof(raw));

          if (bytesRead != sizeof(raw)) {
            Serial.println("BYE");
            break;
          }

          for (int z = 0; z<steps; z++) {
              accel[x+z] = (byte) map(raw[z], IMULow, IMUHigh, 0, 255);
          }

          x += steps;
          ++samples;

          /* If there's not enough room left in the buffers
          * for the next read, then we're done */
          if (x + steps > sensorBufSize) {
              Serial.println("not enought room, breaking");
              break;
          }
      }

      if (samples != samples_per_ts) {
        break;
      }

      byte vector[vectorNumBytes];

      undersample(accel, samples, vector);

      total++;
      int cls = CuriePME.classify(vector, vectorNumBytes);

      if (cls == i + 100) {
        total_right++;
      }
    }

    Serial.println(String(i+100) + " :: TOTAL: " + String(total) + " Total right: " + String(total_right) + " ACCURACY: " + String(total_right/total*100) + "%");
  }

  Serial.println("---- BYE ;) ----");
}

void loop () {
}

/* Simple "moving average" filter, removes low noise and other small
 * anomalies, with the effect of smoothing out the data stream. */
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
            ret += samples[pos - (3 * i)];
        }

        ret /= size;
    }

    return (byte)ret;
}

/* We need to compress the stream of raw accelerometer data into 128 bytes, so
 * it will fit into a neuron, while preserving as much of the original pattern
 * as possible. Assuming there will typically be 1-2 seconds worth of
 * accelerometer data at 200Hz, we will need to throw away over 90% of it to
 * meet that goal!
 *
 * This is done in 2 ways:
 *
 * 1. Each sample consists of 3 signed 16-bit values (one each for X, Y and Z).
 *    Map each 16 bit value to a range of 0-255 and pack it into a byte,
 *    cutting sample size in half.
 *
 * 2. Undersample. If we are sampling at 200Hz and the button is held for 1.2
 *    seconds, then we'll have ~240 samples. Since we know now that each
 *    sample, once compressed, will occupy 3 of our neuron's 128 bytes
 *    (see #1), then we know we can only fit 42 of those 240 samples into a
 *    single neuron (128 / 3 = 42.666). So if we take (for example) every 5th
 *    sample until we have 42, then we should cover most of the sample window
 *    and have some semblance of the original pattern. */
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

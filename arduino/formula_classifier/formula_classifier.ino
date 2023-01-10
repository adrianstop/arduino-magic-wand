/*
  IMU Classifier

  This example uses the on-board IMU to start reading acceleration and gyroscope
  data from on-board IMU, once enough samples are read, it then uses a
  TensorFlow Lite (Micro) model to try to classify the movement as a known gesture.

  Note: The direct use of C/C++ pointers, namespaces, and dynamic memory is generally
        discouraged in Arduino examples, and in the future the TensorFlowLite library
        might change to make the sketch simpler.

  The circuit:
  - Arduino Nano 33 BLE or Arduino Nano 33 BLE Sense board.

  Created by Don Coleman, Sandeep Mistry
  Modified by Dominic Pajak, Sandeep Mistry

  This example code is in the public domain.
*/

#include <Arduino_LSM9DS1.h>

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "model.h"

const float accelerationThreshold = 2.5; // threshold of significant in G's
const int numSamples = 119;
//const int numAvg = 5;

int samplesRead = numSamples;

// global variables used for TensorFlow Lite (Micro)
tflite::MicroErrorReporter tflErrorReporter;

// pull in all the TFLM ops, you can remove this line and
// only pull in the TFLM ops you need, if would like to reduce
// the compiled size of the sketch.
tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Create a static memory buffer for TFLM, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 10 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

// array to map gesture index to a name
const char* FORMULAS[] = {
  "alohomora",
  "arresto_momentum",
  "avadakedavra",
  "locomotor",
  "revelio"  
};

#define NUM_FORMULAS (sizeof(FORMULAS) / sizeof(FORMULAS[0]))

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // print out the samples rates of the IMUs
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.print("Magnetometer sample rate = ");
  Serial.print(IMU.magneticFieldSampleRate());
  Serial.println(" Hz");

  Serial.println();

  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  unsigned int i;
  float aX, aY, aZ, gX, gY, gZ, mX, mY, mZ;

  // wait for significant motion
  while (samplesRead == numSamples) {
    if (IMU.accelerationAvailable()) {
      // read the acceleration data
      IMU.readAcceleration(aX, aY, aZ);

      // sum up the absolutes
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // check if it's above the threshold
      if (aSum >= accelerationThreshold) {
        // reset the sample read count
        samplesRead = 0;
        break;
      }
    }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // check if new acceleration AND gyroscope data is available
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);
      if (IMU.magneticFieldAvailable()){
        IMU.readMagneticField(mX, mY, mZ);        
      }

      // normalize the IMU data between 0 to 1 and store in the model's
      // input tensor

      tflInputTensor->data.f[samplesRead * 9 + 0] = ((aX + 4.0) / 8.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 1] = ((aY + 4.0) / 8.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 2] = ((aZ + 4.0) / 8.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 3] = ((gX + 2000.0) / 4000.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 4] = ((gY + 2000.0) / 4000.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 5] = ((gZ + 2000.0) / 4000.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 6] = ((mX + 400.0) / 800.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 7] = ((mY + 400.0) / 800.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      tflInputTensor->data.f[samplesRead * 9 + 8] = ((mZ + 400.0) / 800.0); // / tflInputTensor->params.scale + tflInputTensor->params.zero_point;
      
      //Serial.print("samplesRead: ");
      //Serial.println(samplesRead);
      
      //Serial.print("((aX + 4.0) / 8.0): ");
      //Serial.println(((aX + 4.0) / 8.0));
      
      Serial.print("aX: ");
      Serial.println(tflInputTensor->data.f[samplesRead * 9 + 0]);

      samplesRead++;      
            

      if (samplesRead == numSamples) {
        // Run inferencing
        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk) {
          Serial.println("Invoke failed!");
          while (1);
          return;
        }
        // Loop through the output tensor values from the model
        for (i = 0; i < NUM_FORMULAS; i++) {
          Serial.print(FORMULAS[i]);
          Serial.print(": ");
          Serial.println(tflOutputTensor->data.f[i], 4);
        }
        Serial.println();
      }
    }
  }
}

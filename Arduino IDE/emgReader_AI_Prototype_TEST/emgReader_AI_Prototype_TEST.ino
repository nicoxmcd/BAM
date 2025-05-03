#include <BAM_Monitoring_AI_inferencing.h>
#include <Arduino.h>

#define EMG_PIN A1
// Sampling rate and Edge Impulse parameters
#define SAMPLE_RATE_HZ        100
#define INPUT_FEATURES        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define SAMPLE_INTERVAL_US    (1000000UL / SAMPLE_RATE_HZ)

// Buffer for live data
static float inputBuffer[INPUT_FEATURES];
size_t bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting live EMG inference...");
}

void loop() {
  static uint32_t nextMicros = micros();
  if (micros() >= nextMicros) {
    nextMicros += SAMPLE_INTERVAL_US;

    // 1) Read raw EMG value
    int raw = analogRead(EMG_PIN);
    // If your model expects normalized data, apply scaling here
    float sample = (float)raw;

    // 2) Store in buffer
    inputBuffer[bufferIndex++] = sample;

    // 3) Once we have a full window, run inference
    if (bufferIndex >= INPUT_FEATURES) {
      bufferIndex = 0;  // reset for next window

      // Prepare signal struct
      signal_t signal;
      if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) {
        Serial.println("ERR: signal_from_buffer");
        continue;
      }

      // Run classifier
      ei_impulse_result_t result;
      if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
        Serial.println("ERR: run_classifier");
        continue;
      }

      // 4) Print results
      Serial.print("Prediction: ");
      for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print(result.classification[i].label);
        Serial.print(" ");
        Serial.print(result.classification[i].value * 100, 1);
        Serial.print("%  ");
      }
      Serial.println();
    }
  }
}

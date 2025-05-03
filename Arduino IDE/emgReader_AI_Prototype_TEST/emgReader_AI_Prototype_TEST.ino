#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // your Edge Impulse inferencing header

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE  // keep this in sync

// rolling buffer
static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Starting EMG → Edge Impulse inference demo");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    // 1) read raw EMG
    int raw = analogRead(EMG_PIN);

    // 2) normalize the way you trained your model
    //    (if you trained on raw ADC [0…1023], skip this step)
    float norm = float(raw - 512) / 512.0f;

    // 3) store in rolling window
    window_buffer[window_index++] = norm;

    // print out raw for debugging
    Serial.print("Raw: "); Serial.print(raw);
    Serial.print("  Norm: "); Serial.println(norm, 4);

    // 4) when full, run inference
    if (window_index >= WINDOW_SIZE) {
      window_index = 0;

      // wrap into signal_t
      signal_t signal;
      if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
        Serial.println("ERR: signal_from_buffer failed");
      } else {
        // run Edge Impulse classifier
        ei_impulse_result_t result;
        if (run_classifier(&signal, &result) == EI_IMPULSE_OK) {
          // find highest score
          size_t best_ix = 0;
          for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (result.classification[i].value > result.classification[best_ix].value) {
              best_ix = i;
            }
          }
          // print prediction
          float confidence = result.classification[best_ix].value * 100.0f;
          Serial.print(">>> Prediction: ");
          Serial.print(result.classification[best_ix].label);
          Serial.print(" (");
          Serial.print(confidence, 1);
          Serial.println("%)");
        } else {
          Serial.println("ERR: run_classifier failed");
        }
      }
    }

    // schedule next sample
    nextMicros += interval;
  }
}

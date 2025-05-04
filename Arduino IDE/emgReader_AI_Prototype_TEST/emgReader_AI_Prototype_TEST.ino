#include <BAM_Monitoring_AI_inferencing.h>    // your EI inferencing header

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// 1) Rolling buffer for raw samples
static float   window_buffer[WINDOW_SIZE];
static size_t  window_index = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Starting live inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 2) Read one raw sample and store it
  int raw = analogRead(EMG_PIN);
  window_buffer[window_index++] = raw;

  // 3) If we have enough samples, run inference
  if (window_index >= WINDOW_SIZE) {
    window_index = 0;

    // 3a) Wrap into signal_t
    signal_t signal;
    if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
      Serial.println("ERR: signal_from_buffer failed");
      return;
    }

    // 3b) Run the classifier
    ei_impulse_result_t result;
    if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
      Serial.println("ERR: run_classifier failed");
      return;
    }

    // 4) Print out all three class confidences
    Serial.print("Prediction → ");
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      Serial.print(result.classification[i].label);
      Serial.print(":");
      Serial.print(result.classification[i].value * 100, 1);
      Serial.print("%  ");
    }
    Serial.println();
  }
}

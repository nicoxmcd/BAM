#include <BAM_Monitoring_AI_inferencing.h>

#define EMG_PIN A0          // Update if different
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

float features[WINDOW_SIZE];
uint16_t index = 0;

// --- These values MUST MATCH your model's training ranges ---
const uint16_t baselineMVC = 17;     // relaxed raw value
const uint16_t strainMVC = 850;      // highest contraction recorded

// Calculate once for speed
const float invRange = 1.0f / float(strainMVC - baselineMVC);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting EMG Inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    // 1. Read raw value
    int raw = analogRead(EMG_PIN);

    // 2. Normalize to 0.0–1.0 (MVC-based)
    float mvc = (raw <= baselineMVC) ? 0.0f :
                (raw >= strainMVC)  ? 1.0f :
                float(raw - baselineMVC) * invRange;

    features[index++] = mvc;

    // 3. Once full window, run inference
    if (index >= WINDOW_SIZE) {
      index = 0;

      signal_t signal;
      if (numpy::signal_from_buffer(features, WINDOW_SIZE, &signal) == 0) {
        ei_impulse_result_t result;
        if (run_classifier(&signal, &result) == EI_IMPULSE_OK) {
          // Print result
          Serial.print("RAW:");
          Serial.print(raw);
          Serial.print("  → ");
          for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            Serial.print(result.classification[i].label);
            Serial.print(":");
            Serial.print(result.classification[i].value * 100, 1);
            Serial.print("%  ");
          }
          Serial.println();
        } else {
          Serial.println("ERR: Classifier failed");
        }
      } else {
        Serial.println("ERR: signal_from_buffer failed");
      }
    }

    nextMicros += interval;
  }
}

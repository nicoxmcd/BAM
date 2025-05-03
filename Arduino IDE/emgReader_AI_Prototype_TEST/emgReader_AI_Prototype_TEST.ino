#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>

#define EMG_PIN           A1
#define SAMPLE_RATE_HZ    100
// Match this to the window size your model was trained on:
#define WINDOW_SIZE       128  

static float window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

bool model_ready = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize your inference engine
  if (!BAM_AI_initialize()) {
    Serial.println("Failed to initialize AI model!");
    while (1);
  }
  model_ready = true;
  Serial.println("Model initialized. Starting EMG acquisition...");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    // 1) Read EMG
    int rawValue = analogRead(EMG_PIN);
    // 2) Normalize / convert as needed by your model
    float normalized = (rawValue - 512) / 512.0f;
    // 3) Store into sliding window
    window_buffer[window_index++] = normalized;

    // Print raw for monitoring
    Serial.print("Raw:");
    Serial.print(rawValue);

    // 4) When window is full, run inference
    if (window_index >= WINDOW_SIZE && model_ready) {
      window_index = 0;  // reset for next window

      // Prepare a result struct (adjust type to your SDK)
      BAM_AI_Result result;
      bool ok = BAM_AI_run_inference(window_buffer, WINDOW_SIZE, &result);

      if (ok) {
        // Print the top prediction
        Serial.print("  Prediction: ");
        Serial.print(result.label);
        Serial.print(" (");
        Serial.print(result.confidence * 100, 1);
        Serial.println("%)");
      } else {
        Serial.println("  Inference error");
      }
    } else {
      Serial.println();
    }

    nextMicros += interval;
  }
}

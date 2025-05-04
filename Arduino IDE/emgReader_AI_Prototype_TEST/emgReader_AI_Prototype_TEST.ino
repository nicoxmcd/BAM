#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
// This comes from your .h — number of samples your model expects
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
// If you want to run only every N windows, set >1
#define SKIP_WINDOWS   1

static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index   = 0;
static uint16_t windows_skipped = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!BAM_AI_initialize()) {
    Serial.println("Failed to init AI!");
    while (1);
  }
  Serial.println("Starting live inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read raw ADC (0..4095 on nRF52840)
  int raw = analogRead(EMG_PIN);

  // 2) Scale to 0..1 float
  float sample = float(raw) / 4095.0f;

  // 3) Store
  window_buffer[window_index++] = sample;

  // 4) Once full, optionally skip some windows, then infer
  if (window_index >= WINDOW_SIZE) {
    window_index = 0;
    if (++windows_skipped >= SKIP_WINDOWS) {
      windows_skipped = 0;

      // Build signal object
      signal_t signal;
      if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
        Serial.println("ERR buffer→signal");
        return;
      }

      // Run inference
      ei_impulse_result_t res;
      if (run_classifier(&signal, &res) != EI_IMPULSE_OK) {
        Serial.println("ERR inference");
        return;
      }

      // 5) Print RAW window summary + prediction
      // Here we print the last raw value and the prediction:
      Serial.print("RAW:"); Serial.print(raw);
      Serial.print("  → Pred: ");
      Serial.print("Relaxed:");
      Serial.print(res.classification[0].value * 100, 1);
      Serial.print("%  Flexed:");
      Serial.print(res.classification[1].value * 100, 1);
      Serial.print("%  Strained:");
      Serial.print(res.classification[2].value * 100, 1);
      Serial.println("%");
    }
  }
}

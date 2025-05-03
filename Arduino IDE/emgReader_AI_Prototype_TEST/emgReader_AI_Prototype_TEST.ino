#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// Rolling sample buffer
static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

// Calibration points
uint32_t baseline  = 0;  // fully relaxed
uint32_t flexMVC   = 1;  // comfortable max contraction
uint32_t strainMVC = 2;  // over‑exertion peak
float    invRange  = 1.0f;

// Helper: average ADC over durationMs
uint32_t calibrateWindow(uint32_t durationMs) {
  uint32_t sum = 0, cnt = 0, end = millis() + durationMs;
  while (millis() < end) {
    sum += analogRead(EMG_PIN);
    cnt++;
    delay(10);
  }
  return cnt ? sum / cnt : 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // 1) Baseline (relaxed)
  Serial.println("Calibrating baseline (relaxed) for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);

  // 2) Flex MVC (comfortable max)
  Serial.println("Calibrating flex MVC for 5 s…");
  flexMVC = calibrateWindow(5000);
  Serial.print("→ flexMVC = "); Serial.println(flexMVC);

  // 3) Strain MVC (over‑exertion)
  Serial.println("Calibrating strain MVC for 5 s…");
  strainMVC = calibrateWindow(5000);
  Serial.print("→ strainMVC = "); Serial.println(strainMVC);

  // Build single normalization scale from baseline…strainMVC
  invRange = 1.0f / float(strainMVC - baseline);
  Serial.println("Calibration complete. Starting inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read and normalize
  int raw = analogRead(EMG_PIN);
  float norm = (raw <= int(baseline)) ? 0.0f
             : (raw >= int(strainMVC)) ? 1.0f
             : float(raw - baseline) * invRange;
  window_buffer[window_index++] = norm;

  // 2) When window is full, run inference
  if (window_index < WINDOW_SIZE) return;
  window_index = 0;

  signal_t signal;
  if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
    Serial.println("ERR: signal_from_buffer");
    return;
  }
  ei_impulse_result_t res;
  if (run_classifier(&signal, &res) != EI_IMPULSE_OK) {
    Serial.println("ERR: run_classifier");
    return;
  }

  // 3) Print just the predictions
  Serial.print("Prediction → ");
  Serial.print("Relaxed:");
  Serial.print(res.classification[0].value * 100, 1);
  Serial.print("%  Flexed:");
  Serial.print(res.classification[1].value * 100, 1);
  Serial.print("%  Strained:");
  Serial.print(res.classification[2].value * 100, 1);
  Serial.println("%");
}

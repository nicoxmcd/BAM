#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// Rolling sample buffer
static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

// Calibration points
uint32_t baseline  = 0;  // fully relaxed
uint32_t flexMVC   = 1;  // comfortable max contraction
uint32_t strainMVC = 2;  // over-exertion peak
float    invRange  = 1.0f;

// Exponential smoothing state
static float smooth_val = 0.0f;
// α between 0–1: lower = heavier smoothing
constexpr float SMOOTH_ALPHA = 0.1f;

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
  Serial.println("Calibrating baseline for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);

  // 2) Flex MVC (comfortable max)
  Serial.println("Calibrating flex MVC for 5 s…");
  flexMVC = calibrateWindow(5000);
  Serial.print("→ flexMVC = "); Serial.println(flexMVC);

  // 3) Strain MVC (over-exertion peak)
  Serial.println("Calibrating strain MVC for 5 s…");
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

  // 1) Read raw EMG
  int raw = analogRead(EMG_PIN);

  // 2) Compute absolute deviation from baseline, then normalize to [0..1]
  float dev = float(raw) - float(baseline);
  if (dev < 0) dev = -dev;                     // absolute
  float norm = dev * invRange;                 // scale
  if (norm > 1.0f) norm = 1.0f;                // clamp
  if (norm < 0.0f) norm = 0.0f;

  // 3) Exponential smoothing to reduce spike‐noise
  smooth_val = SMOOTH_ALPHA * norm + (1.0f - SMOOTH_ALPHA) * smooth_val;

  // 4) Fill the EI buffer
  window_buffer[window_index++] = smooth_val;

  // 5) Once full, infer
  if (window_index >= WINDOW_SIZE) {
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

    // 6) Print the predictions
    Serial.print("Prediction → ");
    Serial.print("Relaxed:");
    Serial.print(res.classification[0].value * 100, 1);
    Serial.print("%  Flexed:");
    Serial.print(res.classification[1].value * 100, 1);
    Serial.print("%  Strained:");
    Serial.print(res.classification[2].value * 100, 1);
    Serial.println("%");
  }
}

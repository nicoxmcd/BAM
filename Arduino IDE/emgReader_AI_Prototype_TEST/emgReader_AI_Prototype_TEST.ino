#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define ENV_WINDOW     50     // 50 samples @100Hz = 0.5s RMS

// Rolling sample buffer for EI
static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

// Rolling buffer for RMS envelope
static float   env_buffer[ENV_WINDOW];
static uint8_t env_index = 0;

// Calibration points
uint32_t baseline  = 0;  // fully relaxed
uint32_t flexMVC   = 1;  // comfortable max contraction
uint32_t strainMVC = 2;  // over-exertion peak
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

  // Build normalization scale from baseline…strainMVC
  invRange = 1.0f / float(strainMVC - baseline);
  Serial.println("Calibration complete. Open Serial Plotter at 115200 to view RAW, NORM, RMS");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // --- 1) Read raw EMG
  int raw = analogRead(EMG_PIN);

  // --- 2) Full-wave rectify around baseline
  float dev = float(raw) - float(baseline);
  if (dev < 0) dev = -dev;

  // --- 3) Normalize to [0..1]
  float norm = dev * invRange;
  if (norm > 1.0f) norm = 1.0f;
  if (norm < 0.0f) norm = 0.0f;

  // --- 4) Build RMS envelope buffer
  env_buffer[env_index++] = norm;
  if (env_index >= ENV_WINDOW) env_index = 0;

  // compute RMS of last ENV_WINDOW samples
  float sumsq = 0.0f;
  for (int i = 0; i < ENV_WINDOW; i++) {
    sumsq += env_buffer[i] * env_buffer[i];
  }
  float rms = sqrtf(sumsq / ENV_WINDOW);

  // --- 5) Debug: print RAW, NORM, RMS for Serial Plotter
  // Format: RAW:<adc>  NORM:<0–1>  RMS:<0–1>
  Serial.print("RAW:");   Serial.print(raw);
  Serial.print("  NORM:"); Serial.print(norm, 3);
  Serial.print("  RMS:");  Serial.println(rms, 3);

  // --- 6) Fill the EI buffer with rms
  window_buffer[window_index++] = rms;

  // --- 7) Once the window is full, run inference
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

    // --- 8) Print the predictions
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

#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define ENV_WINDOW     50     // 50 samples @100Hz = 0.5s RMS
#define SUMMARY_MS     5000   // report every 5 seconds

// Rolling sample buffer for EI
static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

// Rolling buffer for RMS envelope
static float   env_buffer[ENV_WINDOW];
static uint8_t env_index = 0;

// Calibration points
uint32_t baseline  = 0;  
uint32_t flexMVC   = 1;  
uint32_t strainMVC = 2;  
float    invRange  = 1.0f;

// Accumulators for 5 s summary
static float   sum_prob[3] = {0.0f, 0.0f, 0.0f};
static uint16_t inf_count  = 0;
static uint32_t last_summary_ms = 0;

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

  // Calibration
  Serial.println("Calibrating baseline for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);

  Serial.println("Calibrating flex MVC for 5 s…");
  flexMVC = calibrateWindow(5000);
  Serial.print("→ flexMVC = "); Serial.println(flexMVC);

  Serial.println("Calibrating strain MVC for 5 s…");
  strainMVC = calibrateWindow(5000);
  Serial.print("→ strainMVC = "); Serial.println(strainMVC);

  invRange = 1.0f / float(strainMVC - baseline);

  Serial.println("Calibration complete.");
  Serial.println("Beginning live inference. Summary every 5 s.");
  last_summary_ms = millis();
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read raw EMG
  int raw = analogRead(EMG_PIN);

  // 2) Full‐wave rectify around baseline
  float dev = float(raw) - float(baseline);
  if (dev < 0) dev = -dev;

  // 3) Normalize to [0..1]
  float norm = dev * invRange;
  norm = (norm > 1.0f) ? 1.0f : (norm < 0.0f) ? 0.0f : norm;

  // 4) Build RMS envelope buffer
  env_buffer[env_index++] = norm;
  if (env_index >= ENV_WINDOW) env_index = 0;

  // Compute RMS
  float sumsq = 0.0f;
  for (int i = 0; i < ENV_WINDOW; i++) sumsq += env_buffer[i] * env_buffer[i];
  float rms = sqrtf(sumsq / ENV_WINDOW);

  // 5) Fill the EI input buffer
  window_buffer[window_index++] = rms;

  // 6) When window full, run inference
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

    // 7) Accumulate probabilities
    for (uint8_t i = 0; i < 3; i++) {
      sum_prob[i] += res.classification[i].value;
    }
    inf_count++;

    // 8) Every SUMMARY_MS ms, print the averaged result
    uint32_t now = millis();
    if (now - last_summary_ms >= SUMMARY_MS) {
      Serial.println();
      Serial.print("=== 5s Summary → ");
      // Compute average & find max class
      float best_val = -1;
      int   best_i   = 0;
      for (uint8_t i = 0; i < 3; i++) {
        float avg = sum_prob[i] / float(inf_count);
        Serial.print(i==0 ? "Relaxed:" : i==1 ? "Flexed:" : "Strained:");
        Serial.print(avg * 100, 1);
        Serial.print("%  ");
        if (avg > best_val) {
          best_val = avg;
          best_i   = i;
        }
      }
      Serial.print("→ Final: ");
      Serial.print(best_i==0 ? "Relaxed" : best_i==1 ? "Flexed" : "Strained");
      Serial.print(" (");
      Serial.print(best_val * 100, 1);
      Serial.println("%)");
      Serial.println("==========================");
      Serial.println();

      // Reset for next epoch
      sum_prob[0] = sum_prob[1] = sum_prob[2] = 0.0f;
      inf_count   = 0;
      last_summary_ms += SUMMARY_MS;
    }
  }
}

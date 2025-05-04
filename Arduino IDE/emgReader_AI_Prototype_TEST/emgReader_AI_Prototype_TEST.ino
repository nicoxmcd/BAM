#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // Edge Impulse inferencing

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// Buffer for EI; we’ll flood it with the latest sample
static float window_buffer[WINDOW_SIZE];

// Calibration points
uint32_t baseline  = 0;  // relaxed MVC
uint32_t flexMVC   = 1;  // max‐comfortable MVC
float    invRange  = 1.0f;

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

  // Calibrate just the two MVC points
  Serial.println("Calibrating baseline (relaxed) for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);

  Serial.println("Calibrating flex MVC for 5 s…");
  flexMVC = calibrateWindow(5000);
  Serial.print("→ flexMVC = "); Serial.println(flexMVC);

  invRange = 1.0f / float(flexMVC - baseline);

  Serial.println("Calibration complete. Live inferencing every sample:");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;
  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read raw
  int raw = analogRead(EMG_PIN);

  // 2) Normalize [baseline…flexMVC] → [0…1]
  float norm;
  if (raw <= int(baseline)) {
    norm = 0.0f;
  } else if (raw >= int(flexMVC)) {
    norm = 1.0f;
  } else {
    norm = float(raw - baseline) * invRange;
  }

  // 3) Flood the EI window buffer with this single value
  for (int i = 0; i < WINDOW_SIZE; i++) {
    window_buffer[i] = norm;
  }

  // 4) Build signal and run inference
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

  // 5) Print RAW + prediction
  Serial.print("RAW:"); Serial.print(raw);
  Serial.print("  → Relaxed:");
  Serial.print(res.classification[0].value * 100, 1); Serial.print("%");
  Serial.print("  Flexed:");
  Serial.print(res.classification[1].value * 100, 1); Serial.print("%");
  Serial.print("  Strained:");
  Serial.print(res.classification[2].value * 100, 1); Serial.println("%");
}

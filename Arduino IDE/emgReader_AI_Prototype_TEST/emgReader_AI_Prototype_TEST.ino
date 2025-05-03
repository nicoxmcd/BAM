#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>  // your Edge Impulse inferencing header

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

static float   window_buffer[WINDOW_SIZE];
static uint16_t window_index = 0;

// Calibration globals
uint32_t baseline = 0, maxMVC = 1;
float    invRange  = 1.0f;

// Helper: average ADC over timespan
uint32_t calibrateWindow(uint32_t ms) {
  uint32_t sum=0, cnt=0, end=millis()+ms;
  while(millis()<end) {
    sum += analogRead(EMG_PIN);
    cnt++;
    delay(10);
  }
  return cnt ? sum/cnt : 0;
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // 1) Calibrate resting baseline
  Serial.println("Calibrating baseline (relaxed) for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);

  // 2) Calibrate max MVC
  Serial.println("Calibrating max MVC (flex) for 5 s…");
  maxMVC = calibrateWindow(5000);
  Serial.print("→ maxMVC = "); Serial.println(maxMVC);

  invRange = 1.0f / float(maxMVC - baseline);
  Serial.println("Calibration complete. Starting inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    // --- sample & normalize
    int raw = analogRead(EMG_PIN);
    float rel = (raw <= baseline) ? 0.0f
              : (raw >= maxMVC)    ? 1.0f
              : float(raw - baseline) * invRange;

    window_buffer[window_index++] = rel;

    // debug
    Serial.print("Raw:"); Serial.print(raw);
    Serial.print("  Rel:"); Serial.println(rel,4);

    // --- once we have a full window, run inference
    if (window_index >= WINDOW_SIZE) {
      window_index = 0;

      signal_t signal;
      if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) == 0) {
        ei_impulse_result_t r;
        if (run_classifier(&signal, &r) == EI_IMPULSE_OK) {
          // print all 3 class confidences:
          Serial.print(">>> Relaxed: "); 
            Serial.print(r.classification[0].value * 100,1); Serial.print("%   ");
          Serial.print("Flexed: ");
            Serial.print(r.classification[1].value * 100,1); Serial.print("%   ");
          Serial.print("Strained: ");
            Serial.print(r.classification[2].value * 100,1); Serial.println("%");
        }
        else {
          Serial.println("ERR: run_classifier failed");
        }
      }
      else {
        Serial.println("ERR: signal_from_buffer failed");
      }
    }

    nextMicros += interval;
  }
}

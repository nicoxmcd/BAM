#define ARDUINO_USB_CDC_ON_BOOT 1    // <<< Must come before TinyUSB include
#include <Adafruit_TinyUSB.h>
#include <BAM_Monitoring_AI_inferencing.h>

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

static float  window_buffer[WINDOW_SIZE];
static size_t window_index = 0;

void setup() {
  Serial.begin(115200);           // Now links to TinyUSB CDC correctly
  while (!Serial);                // Wait for host to open Serial port
  Serial.println("USB CDC & AI Inference starting…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t    interval   = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read raw EMG
  int raw = analogRead(EMG_PIN);
  window_buffer[window_index++] = float(raw);

  // 2) Once buffer is full, run inference
  if (window_index >= WINDOW_SIZE) {
    window_index = 0;

    signal_t signal;
    if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
      Serial.println("ERR: buffer→signal");
      return;
    }
    ei_impulse_result_t result;
    if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
      Serial.println("ERR: classifier");
      return;
    }

    // 3) Print all three class confidences
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

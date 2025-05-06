#define ARDUINO_USB_CDC_ON_BOOT 1    // Must come before TinyUSB include
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <BAM_Monitoring_AI_inferencing.h>

#define EMG_PIN        A1
#define SAMPLE_RATE_HZ 100
#define WINDOW_SIZE    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// Rolling sample buffer
static float  window_buffer[WINDOW_SIZE];
static size_t window_index = 0;

// BLE UART (Nordic UART Service) instance
BLEUart bleuart;

void setup() {
  // USB-CDC for debug
  Serial.begin(115200);
  while (!Serial);

  // BLE UART setup
  Bluefruit.begin();
  Bluefruit.setName("EMG-Sensor");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  Serial.println("USB CDC & BLE UART ready. Starting AI inference…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval   = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) Read raw EMG and buffer
  int raw = analogRead(EMG_PIN);
  window_buffer[window_index++] = float(raw);

  // 2) When buffer is full, run inference
  if (window_index >= WINDOW_SIZE) {
    window_index = 0;

    // Wrap into signal_t
    signal_t signal;
    if (numpy::signal_from_buffer(window_buffer, WINDOW_SIZE, &signal) != 0) {
      Serial.println("ERR: buffer→signal");
    } else {
      ei_impulse_result_t result;
      if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
        Serial.println("ERR: classifier");
      } else {
        // 3) Build CSV string: R:xx,F:yy,S:zz
        char out[64];
        int len = snprintf(out, sizeof(out),
          "R:%.1f,F:%.1f,S:%.1f\n",
          result.classification[0].value * 100.0f,
          result.classification[1].value * 100.0f,
          result.classification[2].value * 100.0f
        );
        // Send to BLE app
        if (bleuart.connected()) {
          bleuart.write((uint8_t*)out, len);
        }
        // Also echo to Serial for debug
        Serial.print(out);
      }
    }
  }
}

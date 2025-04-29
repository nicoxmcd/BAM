#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define INPUT_FEATURES     EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define SAMPLE_INTERVAL_MS EI_CLASSIFIER_INTERVAL_MS
#define BLE_PACKET_SIZE    20    // max BLE packet
#define CAL_DURATION_MS    5000  // 5 s calibration

// BLE UART (Nordic UART Service)
BLEUart bleuart;

// Timing
static uint32_t nextMicros;

// Calibration state
uint32_t baseline = 0, maxMVC = 1;
float    invRange  = 1.0f;

// Input buffer
static float inputBuffer[INPUT_FEATURES];

// Average over durationMs on A1
uint32_t calibrateWindow(uint32_t durationMs) {
  uint32_t sum = 0, count = 0;
  uint32_t end = millis() + durationMs;
  while (millis() < end) {
    sum   += analogRead(A1);
    count += 1;
    delay(10);
  }
  return count ? (sum / count) : 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // --- Auto-calibrate at startup ---
  Serial.println("Auto-calibrating baseline...");
  baseline = calibrateWindow(CAL_DURATION_MS);
  Serial.print("Baseline = "); Serial.println(baseline);

  Serial.println("Auto-calibrating MVC...");
  maxMVC = calibrateWindow(CAL_DURATION_MS);
  Serial.print("MVC = "); Serial.println(maxMVC);

  invRange = 1.0f / float(maxMVC - baseline);
  Serial.println("Calibration complete.");

  // --- BLE UART setup ---
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  bleuart.begin();  // start UART service

  // advertise UART
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  nextMicros = micros();
  Serial.println("BLE & ML ready. Streaming predictions below:");
}

void loop() {
  // --- Sample into buffer (with raw/norm debug) ---
  uint32_t interval = SAMPLE_INTERVAL_MS;
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    int raw = analogRead(A1);
    float rel;
    if      (raw <= int(baseline)) rel = 0.0f;
    else if (raw >= int(maxMVC))   rel = 1.0f;
    else                            rel = (raw - float(baseline)) * invRange;
    inputBuffer[i] = rel;

    // Debug: show raw & normalized
    Serial.print("raw=");   Serial.print(raw);
    Serial.print("  norm="); Serial.println(rel, 4);

    delay(interval);
  }

  // --- Run inference ---
  signal_t signal;
  if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) return;
  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) != EI_IMPULSE_OK) return;

  // --- Serial print prediction ---
  Serial.print("Prediction: ");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    float p = result.classification[i].value * 100.0f;
    Serial.print(result.classification[i].label);
    Serial.print(" ");
    Serial.print(p, 1);
    Serial.print("%  ");
  }
  Serial.println();

  // --- Build & send BLE packet via UART (ASCII) ---
  char buf[BLE_PACKET_SIZE + 1];
  int idx = 0;
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT && idx < BLE_PACKET_SIZE - 6; i++) {
    int pct = int(result.classification[i].value * 100.0f + 0.5f);
    buf[idx++] = result.classification[i].label[0];
    buf[idx++] = ':';
    buf[idx++] = char('0' + (pct / 10));
    buf[idx++] = char('0' + (pct % 10));
    buf[idx++] = ' ';
  }
  buf[idx] = '\0';

  // send over BLE UART
  bleuart.print(buf);

  // optional: echo on Serial
  Serial.println(buf);
}

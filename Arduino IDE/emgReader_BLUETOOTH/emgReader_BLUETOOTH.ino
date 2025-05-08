#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define INPUT_FEATURES     EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define SAMPLE_INTERVAL_MS EI_CLASSIFIER_INTERVAL_MS
#define BLE_PACKET_SIZE    20  // max BLE packet

// BLE services & characteristics
BLEService       emgService(0x181A);
BLECharacteristic emgChar(0x2A58);
BLEService        calibService(0x181B);
BLECharacteristic calibChar(0x2A59);

// Calibration state
volatile uint8_t  calibCmd = 0;
uint32_t          baseline = 0, maxMVC = 1;
float             invRange  = 1.0f;

// Input buffer
static float      inputBuffer[INPUT_FEATURES];

// Simple average over durationMs on A1
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

void calibWriteCallback(uint16_t, BLECharacteristic*, uint8_t* data, uint16_t len) {
  if (len) calibCmd = data[0];
}

void setup() {
  // --- Start BLE only ---
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  // EMG notify service
  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(BLE_PACKET_SIZE);
  emgChar.begin();

  // Calibration write service
  calibService.begin();
  calibChar.setProperties(CHR_PROPS_WRITE);
  calibChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  calibChar.setWriteCallback(calibWriteCallback, false);
  calibChar.begin();

  // Advertise both
  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addService(calibService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();
}

void loop() {
  // 1) Handle calibration
  if (calibCmd == 0x01) {
    baseline = calibrateWindow(5000);
    invRange = 1.0f / float(maxMVC - baseline);
    calibCmd = 0;
  }
  else if (calibCmd == 0x02) {
    maxMVC   = calibrateWindow(5000);
    invRange = 1.0f / float(maxMVC - baseline);
    calibCmd = 0;
  }

  // 2) Collect normalized EMG window
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    int raw = analogRead(A1);
    float rel = (raw <= baseline) ? 0.0f
              : (raw >= maxMVC)   ? 1.0f
              : (raw - float(baseline)) * invRange;
    inputBuffer[i] = rel;
    delay(SAMPLE_INTERVAL_MS);
  }

  // 3) Inference
  signal_t signal;
  if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) return;
  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) != EI_IMPULSE_OK)   return;

  // 4) Build notification string
  char buf[BLE_PACKET_SIZE];
  int pct0 = int(result.classification[0].value * 100 + 0.5f);
  int pct1 = int(result.classification[1].value * 100 + 0.5f);
  int pct2 = int(result.classification[2].value * 100 + 0.5f);
  int len = snprintf(buf, sizeof(buf), "R:%d,F:%d,S:%d", pct0, pct1, pct2);

  // 5) Send over BLE
  if (emgChar.notifyEnabled()) {
    emgChar.notify((uint8_t*)buf, len);
  }
}

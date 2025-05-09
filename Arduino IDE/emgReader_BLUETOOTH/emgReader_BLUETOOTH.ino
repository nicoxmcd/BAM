#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define INPUT_FEATURES     EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define SAMPLE_INTERVAL_MS EI_CLASSIFIER_INTERVAL_MS
#define BLE_PACKET_SIZE    20  // max BLE packet

// BLE services & characteristics
BLEService       emgService(0x181A);
BLECharacteristic emgChar(0x2A58);

// Calibration state
uint32_t          baseline = 0, maxMVC = 1;
float             invRange  = 1.0f;

// Input buffer
static float      inputBuffer[INPUT_FEATURES];

// === Helper: average ADC over durationMs on A1 ===
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
  // 1) BLE init
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");
  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(BLE_PACKET_SIZE);
  emgChar.begin();
  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();

  // 2) Auto-calibrate baseline (relaxed) & maxMVC (full flex)
  Serial.begin(115200);                 // for debug if USB is present
  delay(200);                           
  Serial.println("Auto-calibrating baseline (relaxed) for 5 s…");
  baseline = calibrateWindow(5000);
  Serial.print("→ baseline = "); Serial.println(baseline);  // turn0search6

  Serial.println("Auto-calibrating max MVC (flex) for 5 s…");
  maxMVC = calibrateWindow(5000);
  Serial.print("→ maxMVC   = "); Serial.println(maxMVC);

  invRange = 1.0f / float(maxMVC - baseline);
  Serial.println("Calibration complete. Starting inference…");
}

void loop() {
  // 3) Fill window of normalized EMG
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    int raw = analogRead(A1);
    float rel = (raw <= int(baseline)) ? 0.0f
              : (raw >= int(maxMVC))   ? 1.0f
              : (raw - float(baseline)) * invRange;
    inputBuffer[i] = rel;
    delay(SAMPLE_INTERVAL_MS);         // match your training rate :contentReference[oaicite:0]{index=0}
  }

  // 4) Wrap + classify
  signal_t signal;
  if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) {
    return;
  }
  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
    return;
  }

  // 5) Build notification: "R:xx,F:yy,S:zz"
  char buf[BLE_PACKET_SIZE];
  int p0 = int(result.classification[0].value * 100 + 0.5f);
  int p1 = int(result.classification[1].value * 100 + 0.5f);
  int p2 = int(result.classification[2].value * 100 + 0.5f);
  int len = snprintf(buf, sizeof(buf), "R:%d,F:%d,S:%d", p0, p1, p2);

  // 6) Send over BLE notify
  if (emgChar.notifyEnabled()) {
    emgChar.notify((uint8_t*)buf, len);
  }
}

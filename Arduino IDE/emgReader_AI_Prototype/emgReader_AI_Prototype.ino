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

volatile uint8_t  calibCmd = 0;
uint32_t          baseline = 0, maxMVC = 1;
float             invRange  = 1.0f;

// Pre-allocated input buffer
static float      inputBuffer[INPUT_FEATURES];

// Simple average over durationMs on A1
uint32_t calibrateWindow(uint32_t durationMs) {
  uint32_t sum = 0, count = 0, end = millis() + durationMs;
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
  Serial.begin(115200);
  while (!Serial);

  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(BLE_PACKET_SIZE);
  emgChar.begin();

  calibService.begin();
  calibChar.setProperties(CHR_PROPS_WRITE);
  calibChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  calibChar.setWriteCallback(calibWriteCallback, false);
  calibChar.begin();

  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addService(calibService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();

  Serial.println("BLE & ML ready. Send calibration commands, then inference will print here.");
}

void loop() {
  // 1) Handle calibration commands
  if (calibCmd == 0x01) {
    Serial.println("Calibrating baseline...");
    baseline = calibrateWindow(5000);
    Serial.print("Baseline = "); Serial.println(baseline);
    calibCmd = 0;
  }
  else if (calibCmd == 0x02) {
    Serial.println("Calibrating MVC...");
    maxMVC = calibrateWindow(5000);
    Serial.print("MVC = "); Serial.println(maxMVC);
    calibCmd = 0;
  }

  // 2) Sample into buffer
  uint32_t interval = SAMPLE_INTERVAL_MS;
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    int raw = analogRead(A1);
    float rel;
    if      (raw <= int(baseline)) rel = 0.0f;
    else if (raw >= int(maxMVC))    rel = 1.0f;
    else                             rel = (raw - float(baseline)) * invRange;
    inputBuffer[i] = rel;
    delay(interval);
  }

  // 3) Run inference
  signal_t signal;
  if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) return;
  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) != EI_IMPULSE_OK) return;

  // 4) Print to Serial for debugging
  Serial.print("Prediction: ");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    float p = result.classification[i].value * 100.0f;
    Serial.print(result.classification[i].label);
    Serial.print(" ");
    Serial.print(p, 1);
    Serial.print("%  ");
  }
  Serial.println();

  // 5) Build and send BLE packet
  char buf[BLE_PACKET_SIZE];
  int  idx = 0;
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT && idx < BLE_PACKET_SIZE - 6; i++) {
    int pct = int(result.classification[i].value * 100.0f + 0.5f);
    buf[idx++] = result.classification[i].label[0];
    buf[idx++] = ':';
    buf[idx++] = char('0' + (pct / 10));
    buf[idx++] = char('0' + (pct % 10));
    buf[idx++] = ' ';
  }
  if (emgChar.notifyEnabled()) {
    emgChar.notify((uint8_t*)buf, idx);
  }
}

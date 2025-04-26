#include <BAM_Monitoring_Training__inferencing.h>
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>



#define INPUT_FEATURES         EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
// The sampling interval (ms) your model was trained with
#define SAMPLE_INTERVAL_MS     EI_CLASSIFIER_INTERVAL_MS

// --- BLE Services & Characteristics ---

// EMG classification service
BLEService       emgService(0x181A);
BLECharacteristic emgChar(0x2A58);    // Notify: classification result

// Calibration service
BLEService        calibService(0x181B);
BLECharacteristic calibChar(0x2A59);  // Write: 0x01=baseline, 0x02=MVC

volatile uint8_t  calibCmd = 0;
uint32_t          baseline = 0, maxMVC = 1;  // avoid div0

// --- Helpers ---

// Average analogRead over `durationMs` ms
uint32_t calibrateWindow(uint8_t pin, uint32_t durationMs) {
  uint32_t sum = 0, count = 0;
  uint32_t end = millis() + durationMs;
  while (millis() < end) {
    sum  += analogRead(pin);
    count++;
    delay(10);
  }
  return (count ? sum / count : 0);
}

// Callback when app writes to calibChar
void calibWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len >= 1) calibCmd = data[0];
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // --- Initialize BLE ---
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  // EMG Service
  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(20);
  emgChar.begin();

  // Calibration Service
  calibService.begin();
  calibChar.setProperties(CHR_PROPS_WRITE);
  calibChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  calibChar.setWriteCallback(calibWriteCallback, false);
  calibChar.begin();

  // Advertise both services
  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addService(calibService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();

  Serial.println("BLE Initialized. Awaiting calibration commands...");
}

void loop() {
  // --- Handle Calibration Commands (APP-activated) ---
  if (calibCmd == 0x01) {
    Serial.println("Calibrating baseline (relaxed) on A1...");
    baseline = calibrateWindow(A1, 5000);
    Serial.print("Baseline set to: "); Serial.println(baseline);
    calibCmd = 0;
  }
  else if (calibCmd == 0x02) {
    Serial.println("Calibrating MVC (max contraction) on A1...");
    maxMVC = calibrateWindow(A1, 5000);
    Serial.print("Max MVC set to: "); Serial.println(maxMVC);
    calibCmd = 0;
  }

  // --- Sample a Window of EMG Data from A1 ---
  float input[INPUT_FEATURES];
  uint32_t interval = SAMPLE_INTERVAL_MS;
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    int raw = analogRead(A1);
    // Normalize 0…1 based on MVC
    float rel = float(raw - baseline) / float(maxMVC - baseline);
    rel = constrain(rel, 0.0f, 1.0f);
    // Scale back to ADC range if needed (model trained on raw ADC)
    input[i] = rel * float(EI_CLASSIFIER_RAW_SAMPLE_MAX);
    delay(interval);
  }

  // --- Run Edge Impulse Inference ---
  signal_t signal;
  if (numpy::signal_from_buffer(input, INPUT_FEATURES, &signal) != 0) {
    Serial.println("ERR: signal_from_buffer"); return;
  }

  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
    Serial.println("ERR: run_classifier"); return;
  }

  // --- Build Notification ---
  char buf[21] = {0};
  int  idx = 0;
  Serial.print("Prediction: ");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    float p = result.classification[i].value * 100.0f;
    Serial.printf("%s:%.0f%% ", result.classification[i].label, p);
    idx += snprintf(buf + idx, sizeof(buf) - idx, "%c:%.0f%% ",
      result.classification[i].label[0], p);
    if (idx >= (int)sizeof(buf)) break;
  }
  Serial.println();

  // --- Notify if client subscribed ---
  if (emgChar.notifyEnabled()) {
    emgChar.notify((uint8_t*)buf, min(idx, 20));
  }

  delay(10);
}

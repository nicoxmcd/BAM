#define ARDUINO_USB_CDC_ON_BOOT 1    // enable USB-CDC even if no USB host
#include <Adafruit_TinyUSB.h>       // must come after the above define
#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define EMG_PIN            A1
#define SAMPLE_INTERVAL_MS EI_CLASSIFIER_INTERVAL_MS
#define INPUT_FEATURES     EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define BLE_PACKET_SIZE    20  // max BLE packet

// BLE service & characteristic
BLEService       emgService(0x181A);
BLECharacteristic emgChar(0x2A58);

// rolling input buffer
static float      inputBuffer[INPUT_FEATURES];
static size_t     bufferIndex = 0;

void setup() {
  // USB-CDC for Serial Monitor
  Serial.begin(115200);
  while (!Serial);

  // BLE initialization
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(BLE_PACKET_SIZE);
  emgChar.begin();

  Bluefruit.Advertising
    .addService(emgService)
    .addName()
    .start();

  Serial.println("BLE & AI inference starting…");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = SAMPLE_INTERVAL_MS * 1000;

  // wait until it's time for next sample
  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) read raw EMG and buffer it
  float raw = analogRead(EMG_PIN);
  inputBuffer[bufferIndex++] = raw;

  // 2) once we've filled the window, run inference
  if (bufferIndex >= INPUT_FEATURES) {
    bufferIndex = 0;

    // wrap into signal_t
    signal_t signal;
    if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) {
      Serial.println("ERR: signal_from_buffer");
      return;
    }

    // run classifier
    ei_impulse_result_t result;
    if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
      Serial.println("ERR: run_classifier");
      return;
    }

    // 3) print to Serial
    Serial.print("Prediction → ");
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      Serial.print(result.classification[i].label);
      Serial.print(":");
      Serial.print(result.classification[i].value * 100, 1);
      Serial.print("%  ");
    }
    Serial.println();

    // 4) build BLE packet "R:xx,F:yy,S:zz"
    char buf[BLE_PACKET_SIZE];
    int p0 = int(result.classification[0].value * 100 + 0.5f);
    int p1 = int(result.classification[1].value * 100 + 0.5f);
    int p2 = int(result.classification[2].value * 100 + 0.5f);
    int len = snprintf(buf, sizeof(buf), "R:%d,F:%d,S:%d", p0, p1, p2);

    // 5) send over BLE
    if (emgChar.notifyEnabled()) {
      emgChar.notify((uint8_t*)buf, len);
    }
  }
}

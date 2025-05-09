#define ARDUINO_USB_CDC_ON_BOOT 1    // enable TinyUSB CDC
#include <Adafruit_TinyUSB.h>       // must follow the above define
#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define EMG_PIN            A1
#define SAMPLE_INTERVAL_MS EI_CLASSIFIER_INTERVAL_MS
#define INPUT_FEATURES     EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define BLE_PACKET_SIZE    20

BLEService       emgService(0x181A);
BLECharacteristic emgChar(0x2A58);

static float inputBuffer[INPUT_FEATURES];
static size_t bufferIndex = 0;

void setup() {
  // 1) USB-CDC for debug (won't block if no host)
  Serial.begin(115200);
  delay(200);
  Serial.println("BLE & AI inference starting…");

  // 2) BLE init
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  // EMG notify service
  emgService.begin();
  emgChar.setProperties(CHR_PROPS_NOTIFY);
  emgChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgChar.setFixedLen(BLE_PACKET_SIZE);
  emgChar.begin();

  // Advertise
  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = SAMPLE_INTERVAL_MS * 1000;

  // wait for next sample
  if (micros() < nextMicros) return;
  nextMicros += interval;

  // 1) buffer raw EMG
  inputBuffer[bufferIndex++] = float(analogRead(EMG_PIN));

  // 2) once full window → inference
  if (bufferIndex >= INPUT_FEATURES) {
    bufferIndex = 0;

    signal_t signal;
    if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) {
      Serial.println("ERR: buffer→signal");
      return;
    }
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

    // 5) notify over BLE
    if (emgChar.notifyEnabled()) {
      emgChar.notify((uint8_t*)buf, len);
    }
  }
}

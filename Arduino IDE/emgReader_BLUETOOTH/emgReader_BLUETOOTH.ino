#include <bluefruit.h>  // Adafruit nRF5 BLE library

#define EMG_PIN         A1
#define SAMPLE_RATE_HZ  100
#define CAL_DURATION_MS 5000  // 5 s for each calibration step

// BLE UART (Nordic UART Service) instance
BLEUart bleuart;

// Timing
static uint32_t nextMicros;

// Calibration values
uint32_t baseline = 0, maxMVC = 1;
float    invRange = 1.0f;

// Streaming flag
bool streaming = false;

// Simple average over durationMs on A1
uint32_t calibrateWindow(uint32_t durationMs) {
  uint32_t sum = 0, count = 0;
  uint32_t end = millis() + durationMs;
  while (millis() < end) {
    sum   += analogRead(EMG_PIN);
    count += 1;
    delay(10);
  }
  return count ? (sum / count) : 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Bluefruit.begin();
  Bluefruit.setName("EMG-Sensor");

  // Start BLE UART
  bleuart.begin();

  // Advertise
  Bluefruit.Advertising
    .addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE)
    .addService(bleuart)
    .start();

  nextMicros = micros();
  Serial.println("BLE UART ready. Send INIT, START or STOP.");
}

void loop() {
  // 1) Handle incoming BLE UART commands
  if (bleuart.available()) {
    String cmd = bleuart.readLine();   // read until '\n'
    cmd.trim();
    if (cmd == "INIT") {
      // Run baseline calibration
      bleuart.println("CALIBRATING BASELINE...");
      baseline = calibrateWindow(CAL_DURATION_MS);
      bleuart.print("BASELINE=");
      bleuart.println(baseline);

      // Run MVC calibration
      bleuart.println("CALIBRATING MVC...");
      maxMVC = calibrateWindow(CAL_DURATION_MS);
      bleuart.print("MVC=");
      bleuart.println(maxMVC);

      // Compute inverse range
      invRange = 1.0f / float(maxMVC - baseline);
      bleuart.println("INIT DONE");
    }
    else if (cmd == "START") {
      streaming = true;
      bleuart.println("STREAMING STARTED");
    }
    else if (cmd == "STOP") {
      streaming = false;
      bleuart.println("STREAMING STOPPED");
    }
    else {
      bleuart.print("UNKNOWN CMD: ");
      bleuart.println(cmd);
    }
  }

  // 2) If streaming, sample & send
  if (streaming && Bluefruit.connected()) {
    uint32_t now = micros();
    uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;
    if (now >= nextMicros) {
      int raw = analogRead(EMG_PIN);
      // Normalize 0–1
      float rel = (raw <= (int)baseline) ? 0.0f
                 : (raw >= (int)maxMVC)     ? 1.0f
                 : (raw - float(baseline)) * invRange;
      // Send as ASCII float
      bleuart.println(rel, 4);      // 4 decimal places
      // Optional serial echo
      Serial.println(rel, 4);

      nextMicros += interval;
      if (now - nextMicros > interval) {
        nextMicros = now + interval;
      }
    }
  }
}

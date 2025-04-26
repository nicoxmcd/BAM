#include <bluefruit.h>  // Adafruit nRF5 BLE library

#define EMG_PIN         A1
#define SAMPLE_RATE_HZ  100
#define CAL_DURATION_MS 5000    // 5 s for each calibration step

// BLE UART (Nordic UART Service) instance
BLEUart bleuart;

// Timing
static uint32_t nextMicros;

// Calibration values
uint32_t baseline = 0, maxMVC = 1;
float    invRange  = 1.0f;

// Streaming flag
bool streaming = false;

// Incoming command buffer
String cmdBuffer;

// Average over durationMs on EMG_PIN
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

  // BLE init
  Bluefruit.begin();
  Bluefruit.setName("EMG-Sensor");

  // Start BLE UART
  bleuart.begin();

  // Advertise UART service
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  // bleuart.service() returns the underlying BLEService
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  nextMicros = micros();
  Serial.println("BLE UART ready. Send INIT, START or STOP.");
}

void loop() {
  // 1) Read any incoming bytes and build a line
  while (bleuart.available()) {
    char c = (char)bleuart.read();
    if (c == '\r') continue;            // ignore CR
    if (c == '\n') {
      // full line received in cmdBuffer
      cmdBuffer.trim();
      if (cmdBuffer.equalsIgnoreCase("INIT")) {
        bleuart.println("CALIBRATING BASELINE...");
        baseline = calibrateWindow(CAL_DURATION_MS);
        bleuart.print("BASELINE="); bleuart.println(baseline);

        bleuart.println("CALIBRATING MVC...");
        maxMVC = calibrateWindow(CAL_DURATION_MS);
        bleuart.print("MVC="); bleuart.println(maxMVC);

        invRange = 1.0f / float(maxMVC - baseline);
        bleuart.println("INIT DONE");
      }
      else if (cmdBuffer.equalsIgnoreCase("START")) {
        streaming = true;
        bleuart.println("STREAMING STARTED");
      }
      else if (cmdBuffer.equalsIgnoreCase("STOP")) {
        streaming = false;
        bleuart.println("STREAMING STOPPED");
      }
      else {
        bleuart.print("UNKNOWN CMD: ");
        bleuart.println(cmdBuffer);
      }
      cmdBuffer = "";  // clear for next line
    }
    else {
      cmdBuffer += c;
      // guard against overflow
      if (cmdBuffer.length() > 32) cmdBuffer = "";
    }
  }

  // 2) If streaming, sample & send
  if (streaming && Bluefruit.connected()) {
    uint32_t now = micros();
    uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;
    if (now >= nextMicros) {
      int raw = analogRead(EMG_PIN);
      float rel = (raw <= (int)baseline) ? 0.0f
                 : (raw >= (int)maxMVC)     ? 1.0f
                 : (raw - float(baseline)) * invRange;

      // Send normalized EMG as ASCII float
      bleuart.println(rel, 4);
      Serial.println(rel, 4);  // debug echo

      nextMicros += interval;
      // correct for any drift
      if (now - nextMicros > interval) {
        nextMicros = now + interval;
      }
    }
  }
  else {
    // idle
    delay(10);
  }
}

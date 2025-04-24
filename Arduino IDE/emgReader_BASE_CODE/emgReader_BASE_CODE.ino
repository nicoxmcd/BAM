#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>

#define EMG_PIN A1
#define SAMPLE_RATE_HZ 100

bool initialized = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // This line is only needed once for the forwarder to detect your device
  Serial.println("Edge Impulse Data Forwarder");
}

void loop() {
  static uint32_t nextMicros = micros();
  const auto interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    int rawValue = analogRead(EMG_PIN);

    // <-- ONLY send the number, no “muscle,” prefix
    Serial.println(rawValue);

    nextMicros += interval;
  }
}

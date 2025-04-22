#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>

// Pin where your MyoWare SIG output is connected
#define EMG_PIN A1

// Desired sampling rate for training (in Hz)
#define SAMPLE_RATE_HZ 100

void setup() {
  // Initialize serial for Edge Impulse data forwarder
  Serial.begin(115200);
  while (!Serial);

  // Print the magic string so the data forwarder recognizes this port
  Serial.println("Edge Impulse Data Forwarder");
}

void loop() {
  static uint32_t nextMicros = micros();
  const uint32_t interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    // Read raw EMG from MyoWare
    int rawValue = analogRead(EMG_PIN);

    // Send in “label,value” format; Edge Impulse will pick up “muscle” channel
    Serial.print("muscle,");
    Serial.println(rawValue);

    // Schedule next sample
    nextMicros += interval;
  }
}

#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>

// Pin where your MyoWare SIG output is connected
#define EMG_PIN            A1      

// Desired training sampling rate (Hz)
#define SAMPLE_RATE_HZ     100     

// How many quick reads to average per frame
#define OVERSAMPLE_COUNT   4      

// Computed interval between frames, in microseconds
#define FRAME_INTERVAL_US  (1000000UL / SAMPLE_RATE_HZ)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Let Edge Impulse CLI detect us
  Serial.println("Edge Impulse Data Forwarder");

  // (Optional) on nRF52840 you can bump to 12‑bit ADC
  analogReadResolution(12);
}

void loop() {
  static uint32_t nextMicros = micros();
  uint32_t now = micros();

  // Only run when our next frame is due
  if (now >= nextMicros) {
    // --- 1) Gather & average a few quick samples ---
    uint32_t sum = 0;
    // Spread the oversamples evenly over the frame interval
    uint32_t slice = FRAME_INTERVAL_US / OVERSAMPLE_COUNT;
    for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++) {
      sum += analogRead(EMG_PIN);
      delayMicroseconds(slice);
    }
    int avgValue = sum / OVERSAMPLE_COUNT;

    // --- 2) Send to Edge Impulse ---
    Serial.print("muscle,");
    Serial.println(avgValue);

    // --- 3) Schedule the next frame ---
    nextMicros += FRAME_INTERVAL_US;
    // If we’ve fallen more than one frame behind, reset to avoid drift
    if (now - nextMicros > FRAME_INTERVAL_US) {
      nextMicros = now + FRAME_INTERVAL_US;
    }
  }
}

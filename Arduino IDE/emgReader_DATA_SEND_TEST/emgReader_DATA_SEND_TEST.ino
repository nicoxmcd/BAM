#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>

const int   SENSOR_PIN        = A1;              // New pin <--- :)
const int   SAMPLE_RATE_HZ    = 200;             // how many readings per second
const float SMOOTH_ALPHA      = 0.1f;            // smoothing factor (0–1)

float       smoothedValue = 0;                   // EMA filter state

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Edge Impulse Data Forwarder");  // Data forwarder looks for this
}

// Read, rectify, smooth, and send
void loop() {
  static unsigned long nextMicros = micros();
  const unsigned long interval = 1000000UL / SAMPLE_RATE_HZ;

  if (micros() >= nextMicros) {
    int raw = analogRead(SENSOR_PIN);

    // full‑wave rectification around a dynamic “zero” baseline if needed:
    float val = abs(raw - 512);      // assuming 10‑bit ADC center at ~512
    // exponential moving average
    smoothedValue = SMOOTH_ALPHA * val + (1 - SMOOTH_ALPHA) * smoothedValue;

    // send in the “label,value” format Edge Impulse expects:
    Serial.print("muscle,");
    Serial.println((int)smoothedValue);

    nextMicros += interval;
  }
}

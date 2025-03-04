#include <bluefruit.h>

#include <Adafruit_TinyUSB.h>


void setup() {
    Serial.begin(9600); // Initialize serial communication at 9600 baud
}

void loop() {
    int muscleSignal = analogRead(A0); // Read from analog pin A0
    Serial.println(muscleSignal); // Print value to Serial Monitor
    delay(100); // Small delay for readability
}

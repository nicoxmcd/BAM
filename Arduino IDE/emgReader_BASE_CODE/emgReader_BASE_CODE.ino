#include <Adafruit_TinyUSB.h>

#include <Adafruit_TinyUSB.h>

#include <bluefruit.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial test running...");
}

void loop() {
    Serial.println(analogRead(A0));  // Print raw MyoWare sensor value
    delay(500);
}

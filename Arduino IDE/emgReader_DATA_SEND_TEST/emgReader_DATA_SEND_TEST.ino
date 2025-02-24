#include <Adafruit_TinyUSB.h>

void setup() {
  Serial.begin(115200);
  // Wait for the Serial Monitor to connect (if needed)
  while (!Serial) { delay(10); }
  Serial.println("Live Muscle Data Output from MyoWare 2.0");
}

void loop() {
  // Read the muscle signal from the MyoWare sensor (analog pin A0)
  int muscleSignal = analogRead(A0);
  
  // Print the raw sensor value to the Serial Monitor
  Serial.println(muscleSignal);

  // Short delay to control the update rate (adjust as needed)
  delay(100);
}

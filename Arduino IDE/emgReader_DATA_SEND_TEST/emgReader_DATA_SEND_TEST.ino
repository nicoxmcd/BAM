#include <bluefruit.h>

// Create a BLE UART service instance
BLEUart bleuart;

void setup() {
  // Initialize Serial for debugging & plotting
  Serial.begin(115200);
  while (!Serial);
  Serial.println("BLE Serial test running...");

  // Initialize Bluefruit with default settings
  Bluefruit.begin();
  Bluefruit.setName("MyoWare BLE");
  
  // Initialize the BLE UART service
  bleuart.begin();
  
  // Add the UART service to advertising packet and start advertising
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  Serial.println("BLE Initialized and Advertising");
}

void loop() {
  // Read raw data from the MyoWare sensor on analog pin A0
  int sensorValue = analogRead(A0);

  // Print the sensor value to the Serial Plotter
  Serial.println(sensorValue);

  // Send the sensor value over BLE UART (newline for each sample)
  bleuart.print(sensorValue);
  bleuart.print("\n");

  delay(100);  // Adjust the delay as needed for your sampling rate
}

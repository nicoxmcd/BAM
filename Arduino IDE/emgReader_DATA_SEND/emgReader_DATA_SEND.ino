#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>

// Define BLE service and characteristic
BLEService emgService(0x181A);         // Custom EMG Service UUID
BLECharacteristic emgCharacteristic(0x2A58); // Custom EMG Data Characteristic

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection
  Serial.println("Running Live Muscle Data Transmission...");

  // Initialize BLE
  Bluefruit.begin();
  Bluefruit.setName("EMG Sensor");

  // Start EMG service and characteristic
  emgService.begin();
  emgCharacteristic.setProperties(CHR_PROPS_NOTIFY);
  emgCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  emgCharacteristic.setFixedLen(20); // BLE packet max size: 20 bytes
  emgCharacteristic.begin();

  // Advertise the service
  Bluefruit.Advertising.addService(emgService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start();
}

void loop() {
  // Read the muscle signal from the MyoWare sensor (analog pin A0)
  int muscleSignal = analogRead(A0);
  Serial.print("Muscle Signal: ");
  Serial.println(muscleSignal);

  // Build a BLE message (e.g., "EMG:1023")
  char bleMessage[21] = {0};  // 20 characters + null terminator
  snprintf(bleMessage, sizeof(bleMessage), "EMG:%d", muscleSignal);

  // Send the BLE notification with the live muscle data
  emgCharacteristic.notify((uint8_t*)bleMessage, strlen(bleMessage));

  delay(100); // Adjust delay as needed for your application
}


#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <Ahuang12-project-1_inferencing.h>

#define INPUT_FEATURES EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// Define BLE service and characteristic
BLEService emgService(0x181A);         // Custom EMG Service UUID
BLECharacteristic emgCharacteristic(0x2A58); // Custom EMG Data Characteristic

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Running Muscle Strain Detection...");

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
  // Read the muscle signal
  int muscleSignal = analogRead(A0);

  // Prepare the input array for the classifier
  float input[INPUT_FEATURES];
  for (size_t i = 0; i < INPUT_FEATURES; i++) {
    input[i] = (float)muscleSignal;
  }

  // Wrap the input in the signal_t structure
  signal_t signal;
  numpy::signal_from_buffer(input, INPUT_FEATURES, &signal);

  // Run the classifier
  ei_impulse_result_t result;
  if (run_classifier(&signal, &result) == EI_IMPULSE_OK) {
    Serial.print("Prediction: ");

    char bleMessage[21] = {0}; // 20 characters + null terminator
    int offset = 0;

    // Process each classification result
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      float percent = result.classification[i].value * 100.0f;
      
      // Print to serial monitor
      Serial.print(result.classification[i].label);
      Serial.print(": ");
      Serial.print(percent, 0);
      Serial.print("% ");

      // Build the BLE message, ensuring we don't overflow the buffer
      offset += snprintf(bleMessage + offset, sizeof(bleMessage) - offset, "%s:%.0f%% ", 
                         result.classification[i].label, percent);
      if (offset >= sizeof(bleMessage)) break;  // Prevent buffer overflow
    }
    Serial.println();

    // Send the BLE notification if data is available
    if (offset > 0) {
      emgCharacteristic.notify((uint8_t*)bleMessage, strlen(bleMessage));
    }
  }

  delay(100); // Adjust delay as needed
}

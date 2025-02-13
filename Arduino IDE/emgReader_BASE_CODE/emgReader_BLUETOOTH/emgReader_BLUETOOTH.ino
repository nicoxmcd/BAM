#include <bluefruit.h>
#include <EdgeImpulse_Inference.h>

BLEUart bleuart;

void setup() {
    Serial.begin(115200);
    Bluefruit.begin();
    Bluefruit.setName("MyoWare nRF52840");
    bleuart.begin();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.Advertising.start();

    Serial.println("BLE Ready");
}

void loop() {
    int muscleSignal = analogRead(A0);

    float input[] = { muscleSignal };
    ei_impulse_result_t result;

    if (run_classifier(input, &result) == EI_IMPULSE_OK) {
        String output = "Classification: " + String(result.classification[0].label) +
                        " Confidence: " + String(result.classification[0].value, 2);
        
        Serial.println(output);
        bleuart.print(output + "\n");  // Send via BLE
    }

    delay(100);
}

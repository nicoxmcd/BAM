#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <Ahuang12-project-1_inferencing.h>


#define INPUT_FEATURES EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE // Expected feature size

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Running Muscle Strain Detection...");
}

void loop() {
    int muscleSignal = analogRead(A0);
    
    // Convert to float array
    float input[INPUT_FEATURES];  // Ensure correct feature size
    for (size_t i = 0; i < INPUT_FEATURES; i++) {
        input[i] = (float)muscleSignal;  // Fill the array with signal value
    }

    // Wrap input in signal_t structure
    signal_t signal;
    numpy::signal_from_buffer(input, INPUT_FEATURES, &signal);

    ei_impulse_result_t result;

    // Corrected function call
    if (run_classifier(&signal, &result) == EI_IMPULSE_OK) {
        Serial.print("Prediction: ");
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            Serial.print(result.classification[i].label);
            Serial.print(": ");
            Serial.print(result.classification[i].value * 100);
            Serial.print("% ");
        }
        Serial.println();
    }

    delay(50);
}

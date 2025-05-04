#include <BAM_Monitoring_AI_inferencing.h>

// Set up buffer for one feature (assuming 1D input like analogRead)
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  int rawValue = analogRead(A0);  // Replace A0 with your sensor pin

  // Normalize the input (Edge Impulse usually scales inputs to [0,1])
  float normalized = (float)rawValue / 1023.0;
  features[0] = normalized;

  // Create the signal object expected by Edge Impulse
  signal_t signal;
  int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0) {
    ei_printf("Failed to create signal from buffer (%d)\n", err);
    return;
  }

  // Run the inference
  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  if (res != EI_IMPULSE_OK) {
    ei_printf("Inference failed (%d)\n", res);
    return;
  }

  // Output RAW value
  Serial.print("RAW:");
  Serial.print(rawValue);
  Serial.print("  → ");

  // Parse and print each class probability
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    Serial.print(result.classification[ix].label);
    Serial.print(":");
    Serial.print(result.classification[ix].value * 100, 1);
    Serial.print("%  ");
  }

  Serial.println();

  delay(500);  // Adjust sampling rate as needed
}

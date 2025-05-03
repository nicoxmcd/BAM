#include <BAM_Monitoring_AI_inferencing.h>
#include <bluefruit.h>

#define EMG_PIN A1
// Sampling and Edge Impulse parameters
#define SAMPLE_RATE_HZ        100
#define SAMPLE_INTERVAL_US    (1000000UL / SAMPLE_RATE_HZ)
#define INPUT_FEATURES        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

// BLE UART (Nordic UART Service)
BLEUart bleuart;

// Calibration state
typedef uint32_t u32;
static u32 baseline = 0, maxMVC = 1;
static float invRange = 1.0f;

// Buffer for live window
typedef float feature_t;
static feature_t inputBuffer[INPUT_FEATURES];
static size_t bufferIndex = 0;

// --- Helpers ---
// Simple average on EMG_PIN over durationMs
u32 calibrateWindow(u32 durationMs) {
    u32 sum = 0, count = 0;
    u32 end = millis() + durationMs;
    while (millis() < end) {
        sum += analogRead(EMG_PIN);
        count++;
        delay(10);
    }
    return count ? (sum / count) : 0;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Auto-calibrate at startup
    Serial.println("Auto-calibrating baseline...");
    baseline = calibrateWindow(5000);
    Serial.print("Baseline = "); Serial.println(baseline);

    Serial.println("Auto-calibrating MVC...");
    maxMVC = calibrateWindow(5000);
    Serial.print("MVC = "); Serial.println(maxMVC);

    invRange = 1.0f / float(maxMVC - baseline);
    Serial.println("Calibration complete.");

    // BLE UART start
    Bluefruit.begin();
    Bluefruit.setName("EMG Sensor");

    bleuart.begin();
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.Advertising.start();

    Serial.println("BLE UART ready. Beginning live inference...");
}

void loop() {
    // 1) Sample at precise rate and fill buffer
    static u32 nextMicros = micros();
    if (micros() < nextMicros) return;
    nextMicros += SAMPLE_INTERVAL_US;

    int raw = analogRead(EMG_PIN);
    float norm;
    if      (raw <= int(baseline)) norm = 0.0f;
    else if (raw >= int(maxMVC))   norm = 1.0f;
    else                             norm = (raw - float(baseline)) * invRange;

    // store in circular buffer
    inputBuffer[bufferIndex++] = norm;
    // debug raw + norm
    Serial.print("raw="); Serial.print(raw);
    Serial.print("  norm="); Serial.println(norm, 4);

    if (bufferIndex < INPUT_FEATURES) return;
    bufferIndex = 0;  // reset for next window

    // 2) Prepare and run classifier
    signal_t signal;
    if (numpy::signal_from_buffer(inputBuffer, INPUT_FEATURES, &signal) != 0) {
        Serial.println("ERR: signal_from_buffer");
        return;
    }
    ei_impulse_result_t result;
    if (run_classifier(&signal, &result) != EI_IMPULSE_OK) {
        Serial.println("ERR: run_classifier");
        return;
    }

    // 3) Print prediction
    Serial.print("Prediction: ");
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        float p = result.classification[i].value * 100.0f;
        Serial.print(result.classification[i].label);
        Serial.print(" ");
        Serial.print(p, 1);
        Serial.print("%  ");
    }
    Serial.println();
}

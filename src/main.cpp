// =============================================================================
// Phase 1: DS18B20 + BMP581 — combined sensor reading
//
// Wiring:
//   DS18B20  data → GP2        (4.7k pull-up to 3.3V required)
//   BMP581   SDA  → GP0  (I2C0)
//   BMP581   SCL  → GP1  (I2C0)
// =============================================================================

#include <Arduino.h>
#include <Wire.h>
#include "tempsensor.h"
#include "altimeter.h"

// ── Config ────────────────────────────────────────────────────────────────────
static const uint8_t DS18B20_PIN     = 2;
static const int     SAMPLE_COUNT    = 200;
static const int     SAMPLE_DELAY_MS = 1000;

// ── Sensor objects ────────────────────────────────────────────────────────────
TempSensor tempSensor(DS18B20_PIN);
Altimeter  altimeter;               // uses default 1013.25 hPa sea level

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("=== Phase 1: Dual Sensor Test ===\n");

    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    if (!altimeter.begin(&Wire)) {
        while (1) delay(100);       // begin() already printed the error
    }

    tempSensor.begin();

    Serial.println();
    Serial.println("sample, timestamp_ms, pressure_hPa, bmp_temp_C, ds18b20_temp_C, altitude_m");
}

void loop() {
    static int count = 0;

    if (count >= SAMPLE_COUNT) {
        Serial.println("Logging complete.");
        while (1) delay(1000);
    }

    AltimeterReading alt = altimeter.read();
    float            ds_temp = tempSensor.getTempC();

    // Only print if BMP581 read succeeded
    // DS18B20 returns DEVICE_DISCONNECTED_C (-127) on error — still logged so
    // you can see it in the output rather than silently dropping the row
    if (alt.valid) {
        Serial.printf("%d, %lu, %.2f, %.2f, %.2f, %.2f\n",
                      count + 1,
                      millis(),
                      alt.pressure_hPa,
                      alt.temperature_C,
                      ds_temp,
                      alt.altitude_m);
        count++;
    }
    // If alt.valid is false, altimeter.read() already printed a warning —
    // count does NOT increment so you always get SAMPLE_COUNT valid rows

    delay(SAMPLE_DELAY_MS);
}
// =============================================================================
// ESP32 — BMP581 + DS18B20 + PCF8523 → LittleFS CSV logging
//
// Wiring:
//   BMP581   SDA → GPIO21  (I2C default)
//   BMP581   SCL → GPIO22  (I2C default)
//   PCF8523  SDA → GPIO21  (shared I2C bus)
//   PCF8523  SCL → GPIO22  (shared I2C bus)
//   DS18B20  DQ  → GPIO4   (4.7k pull-up to 3.3V required)
// =============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

#include "tempsensor.h"
#include "altimeter.h"
#include "RTC.h"


// ── Config ────────────────────────────────────────────────────────────────────
static const uint8_t DS18B20_PIN     = 4;
static const int     SAMPLE_DELAY_MS = 1000;  // ms between samples (0 = as fast as possible)

static const char* LOG_FILE = "/log.csv";     // LittleFS paths start with /

// ── Sensor objects ────────────────────────────────────────────────────────────
TempSensor  tempSensor(DS18B20_PIN);
Altimeter   altimeter;
PCF8523RTC  rtc;

// ── Helpers ───────────────────────────────────────────────────────────────────
bool initFS() {
    // FORMAT_LITTLEFS_IF_FAILED: first-run or corruption safety net
    if (!LittleFS.begin()) { 
        Serial.println("ERROR: LittleFS mount failed.");
        return false;
    }
    Serial.println("LittleFS mounted.");
    return true;
}

bool initLogFile() {
    bool exists = LittleFS.exists(LOG_FILE);

    // "a" = append; creates the file if it doesn't exist
    File f = LittleFS.open(LOG_FILE, "a");
    if (!f) {
        Serial.println("ERROR: Could not open log.csv for writing.");
        return false;
    }

    if (!exists) {
        f.println("timestamp,ds18b20_temp_C,pressure_hPa,bmp_temp_C,altitude_m");
        Serial.println("Created log.csv with header.");
    } else {
        Serial.println("Appending to existing log.csv.");
    }

    f.close();
    return true;
}

void writeLogRow(const String& ts, float dsTemp,
                 float pressure, float bmpTemp, float altitude) {
    Serial.printf("%s, %.2f C, %.2f hPa, %.2f C, %.2f m\n",
                  ts.c_str(), dsTemp, pressure, bmpTemp, altitude);

    File f = LittleFS.open(LOG_FILE, "a");
    if (!f) {
        Serial.println("WARNING: Could not open log.csv to append row.");
        return;
    }

    f.printf("%s,%.2f,%.2f,%.2f,%.2f\n",
             ts.c_str(), dsTemp, pressure, bmpTemp, altitude);
    f.close();
}

// Fallback timestamp using millis() if RTC isn't available
String uptimeString() {
    unsigned long s = millis() / 1000;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", s/3600, (s%3600)/60, s%60);
    return String(buf);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== ESP32 Sensor Logger (LittleFS) ===\n");

    // ESP32 default I2C pins are 21 (SDA) and 22 (SCL)
    // Wire.begin() with no args uses those defaults
    // Wire.setPins();
    Wire.begin();

    // BMP581
    if (!altimeter.begin(&Wire)) {
        Serial.println("FATAL: BMP581 init failed. Halting.");
        while (1) delay(100);
    }

    // DS18B20
    tempSensor.begin();

    // RTC — warn but don't halt if it fails; fall back to uptime
    if (!rtc.begin(&Wire)) {
        Serial.println("WARNING: RTC begin failed. Will use uptime timestamps.");
    } else if (rtc.lostPower()) {
        Serial.println("WARNING: RTC lost power / not set. Timestamps may be wrong.");
        Serial.println("  → Set the RTC time before logging critical data.");
    }

    // LittleFS
    if (!initFS() || !initLogFile()) {
        Serial.println("FATAL: Storage init failed. Halting.");
        while (1) delay(1000);
    }

    Serial.println("\nLogging started. Each row: timestamp, ds18b20_C, pressure_hPa, bmp_C, altitude_m\n");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    AltimeterReading alt = altimeter.read();
    float dsTemp = tempSensor.getTempC();

    if (!alt.valid) {
        Serial.println("WARNING: Invalid altimeter reading, skipping sample.");
        delay(SAMPLE_DELAY_MS);
        return;
    }

    // Use RTC timestamp if available, otherwise fall back to uptime
    String ts = rtc.timestampString();
    if (ts == "RTC_READ_ERROR") {
        ts = uptimeString();
    }

    writeLogRow(ts, dsTemp, alt.pressure_hPa, alt.temperature_C, alt.altitude_m);

    delay(SAMPLE_DELAY_MS);
}
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
#include <SdFat.h>
#include "tempsensor.h"
#include "altimeter.h"
#include "RTC.h"

// ── Config ────────────────────────────────────────────────────────────────────
static const uint8_t DS18B20_PIN     = 2;
static const int     SAMPLE_COUNT    = 200;
static const int     SAMPLE_DELAY_MS = 1000;

static const uint8_t SD_SCK_PIN  = 6;   // CLK
static const uint8_t SD_MOSI_PIN = 7;   // DI
static const uint8_t SD_MISO_PIN = 8;   // DO
static const uint8_t SD_CS_PIN   = 9;   // CS
//static const uint8_t SD_CD_PIN   = 13;  // CD

static const char* LOG_FILE = "log.csv";

// ── Sensor objects ────────────────────────────────────────────────────────────
TempSensor tempSensor(DS18B20_PIN);
Altimeter  altimeter;               // uses default 1013.25 hPa sea level

//software SPI for SdFat bc of pins
SoftSpiDriver<SD_MISO_PIN, SD_MOSI_PIN, SD_SCK_PIN> softSpi;
SdFat sd;
FsFile logFile;

//RTC
PCF8523RTC rtc;

String uptimeString();

// ── Helpers ───────────────────────────────────────────────────────────────────
bool initSDCard() {
    // Lower speed is safer when bringing up rough hardware
    SdSpiConfig config(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(4), &softSpi);

    if (!sd.begin(config)) {
        Serial.println("ERROR: SD card init failed.");
        return false;
    }

    Serial.println("SD card initialized.");

    bool fileExists = sd.exists(LOG_FILE);

    logFile = sd.open(LOG_FILE, O_WRONLY | O_CREAT | O_AT_END);
    if (!logFile) {
        Serial.println("ERROR: Could not open log.csv");
        return false;
    }

    if (!fileExists) {
        logFile.println("time,temp,altitude");
        logFile.flush();
        Serial.println("Created log.csv header");
    } else {
        Serial.println("Appending to existing log.csv");
    }

    return true;
}

void writeLogRow(const String& timeStr, float temp, float altitude) {
    Serial.printf("%s, %.2f, %.2f\n", timeStr.c_str(), temp, altitude);

    if (logFile) {
        logFile.printf("%s,%.2f,%.2f\n", timeStr.c_str(), temp, altitude);
        logFile.flush();
    }
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("=== Temp + Altitude + SD Logging (Soft SPI) ===\n");

    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    if (!altimeter.begin(&Wire)) {
        Serial.println("ERROR: BMP581 init failed.");
        while (1) delay(100);       // begin() already printed the error
    }

    tempSensor.begin();

    // SD
    if (!initSDCard()) {
        Serial.println("ERROR: SD setup failed.");
        while (1) delay(1000);
    }

    Serial.println();
    Serial.println("sample, timestamp_ms, pressure_hPa, bmp_temp_C, ds18b20_temp_C, altitude_m");
}

void loop() {
    static int count = 0;

    if (count >= SAMPLE_COUNT) {
        Serial.println("Logging complete.");
        if (logFile) {
            logFile.flush();
            logFile.close();
        }
        while (1) delay(1000);
    }

    AltimeterReading alt = altimeter.read();
    float ds_temp = tempSensor.getTempC();

    // Only print if BMP581 read succeeded
    // DS18B20 returns DEVICE_DISCONNECTED_C (-127) on error — still logged so
    // you can see it in the output rather than silently dropping the row
    if (alt.valid) {
        String timeStr = uptimeString();
        writeLogRow(timeStr, ds_temp, alt.altitude_m);
        count++;
    } else {
        Serial.println("WARNING: Invalid altimeter reading, skipping sample.");
    }
    // If alt.valid is false, altimeter.read() already printed a warning —
    // count does NOT increment so you always get SAMPLE_COUNT valid rows

    delay(SAMPLE_DELAY_MS);
}

String uptimeString() {
    unsigned long totalSeconds = millis() / 1000;

    unsigned long hours   = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buf);
}
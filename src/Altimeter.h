#pragma once
#include <Wire.h>
#include <Adafruit_BMP5xx.h>

struct AltimeterReading {
    float pressure_hPa;
    float temperature_C;
    float altitude_m;
    bool  valid;        // false if performReading() failed
};

class Altimeter {
public:
    // seaLevelPressure_hPa: local reference for altitude calculation.
    // Defaults to ISA standard 1013.25 hPa — update before launch if possible.
    Altimeter(float seaLevelPressure_hPa = 1013.25f);

    // Call once from setup() after Wire.begin()
    bool begin(TwoWire* wire = &Wire, uint8_t i2cAddr = 0x47);

    // Returns a reading struct — check .valid before using values
    AltimeterReading read();

private:
    Adafruit_BMP5xx _bmp;
    float           _seaLevelPressure;
};
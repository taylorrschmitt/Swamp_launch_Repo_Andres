#include "altimeter.h"
#include <Arduino.h>

Altimeter::Altimeter(float seaLevelPressure_hPa)
    : _seaLevelPressure(seaLevelPressure_hPa) {}

bool Altimeter::begin(TwoWire* wire, uint8_t i2cAddr) {
    if (!_bmp.begin(i2cAddr, wire)) {
        Serial.println("[BMP581] FATAL: Not found. Check wiring and I2C address.");
        return false;
    }
    Serial.println("[BMP581] Found.");

    // Settings hardcoded to your established values
    _bmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
    _bmp.setPressureOversampling(BMP5XX_OVERSAMPLING_4X);
    _bmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    _bmp.setOutputDataRate(BMP5XX_ODR_50_HZ);

    return true;
}

AltimeterReading Altimeter::read() {
    AltimeterReading result = {0, 0, 0, false};

    if (!_bmp.performReading()) {
        Serial.println("[BMP581] WARNING: Read failed, skipping sample.");
        return result;
    }

    result.pressure_hPa  = _bmp.pressure;
    result.temperature_C = _bmp.temperature;
    result.altitude_m    = _bmp.readAltitude(_seaLevelPressure);
    result.valid         = true;

    return result;
}
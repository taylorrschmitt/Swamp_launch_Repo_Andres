#include "tempsensor.h"
#include <Arduino.h>

TempSensor::TempSensor(uint8_t pin)
    : _oneWire(pin), _sensors(&_oneWire) {
    // No Serial, no delay — just initialize member objects
}

void TempSensor::begin() {
    _sensors.begin();

    int count = _sensors.getDeviceCount();
    Serial.print("[DS18B20] Devices found on OneWire bus: ");
    Serial.println(count);

    if (count == 0) {
        Serial.println("[DS18B20] WARNING: No devices found. Check wiring + 4.7k pull-up.");
    }
}

float TempSensor::getTempC() {
    _sensors.requestTemperatures();
    float t = _sensors.getTempCByIndex(0);

    if (t == DEVICE_DISCONNECTED_C) {
        Serial.println("[DS18B20] WARNING: Sensor disconnected or read error.");
    }

    return t;
}
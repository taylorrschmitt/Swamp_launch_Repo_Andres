#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>

class TempSensor {
public:
    TempSensor(uint8_t pin);
    void  begin();      // Call once from setup() — not in constructor
    float getTempC();

private:
    OneWire           _oneWire;
    DallasTemperature _sensors;
};
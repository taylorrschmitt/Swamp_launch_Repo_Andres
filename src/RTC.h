#ifndef PCF8523_H
#define PCF8523_H

#include <Arduino.h>
#include <Wire.h>

struct RtcDateTime {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
};

class PCF8523RTC {
public:
    static constexpr uint8_t I2C_ADDR = 0x68;

    PCF8523RTC();

    bool begin(TwoWire* wire = &Wire);
    bool read(RtcDateTime& dt);
    bool setDateTime(const RtcDateTime& dt);
    bool lostPower();
    String timestampString();   // "YYYY-MM-DD HH:MM:SS"

private:
    TwoWire* _wire;

    uint8_t bcdToDec(uint8_t val);
    uint8_t decToBcd(uint8_t val);

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t startReg, uint8_t* buffer, size_t len);
};

#endif
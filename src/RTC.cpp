#include "RTC.h"

PCF8523RTC::PCF8523RTC() : _wire(&Wire) {}

bool PCF8523RTC::begin(TwoWire* wire) {
    _wire = wire;
    return true;
}

uint8_t PCF8523RTC::bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint8_t PCF8523RTC::decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

bool PCF8523RTC::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(I2C_ADDR);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

bool PCF8523RTC::readRegisters(uint8_t startReg, uint8_t* buffer, size_t len) {
    _wire->beginTransmission(I2C_ADDR);
    _wire->write(startReg);
    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    size_t n = _wire->requestFrom((int)I2C_ADDR, (int)len);
    if (n != len) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        buffer[i] = _wire->read();
    }

    return true;
}

bool PCF8523RTC::lostPower() {
    // Seconds register bit 7 = oscillator stop flag
    uint8_t secReg = 0;
    if (!readRegisters(0x03, &secReg, 1)) {
        return true;
    }

    return (secReg & 0x80) != 0;
}

bool PCF8523RTC::read(RtcDateTime& dt) {
    uint8_t data[7];

    // Registers:
    // 0x03 seconds
    // 0x04 minutes
    // 0x05 hours
    // 0x06 days
    // 0x07 weekdays
    // 0x08 months
    // 0x09 years
    if (!readRegisters(0x03, data, 7)) {
        return false;
    }

    dt.second = bcdToDec(data[0] & 0x7F);
    dt.minute = bcdToDec(data[1] & 0x7F);
    dt.hour   = bcdToDec(data[2] & 0x3F);
    dt.day    = bcdToDec(data[3] & 0x3F);
    dt.month  = bcdToDec(data[5] & 0x1F);
    dt.year   = 2000 + bcdToDec(data[6]);

    return true;
}

bool PCF8523RTC::setDateTime(const RtcDateTime& dt) {
    _wire->beginTransmission(I2C_ADDR);
    _wire->write(0x03);  // start at seconds register

    _wire->write(decToBcd(dt.second));       // seconds
    _wire->write(decToBcd(dt.minute));       // minutes
    _wire->write(decToBcd(dt.hour));         // hours
    _wire->write(decToBcd(dt.day));          // days
    _wire->write((uint8_t)0);                // weekday placeholder
    _wire->write(decToBcd(dt.month));        // month
    _wire->write(decToBcd(dt.year - 2000));  // year offset from 2000

    return (_wire->endTransmission() == 0);
}

String PCF8523RTC::timestampString() {
    RtcDateTime dt;
    if (!read(dt)) {
        return "RTC_READ_ERROR";
    }

    char buf[24];
    snprintf(buf, sizeof(buf),
             "%04u-%02u-%02u %02u:%02u:%02u",
             dt.year, dt.month, dt.day,
             dt.hour, dt.minute, dt.second);

    return String(buf);
}
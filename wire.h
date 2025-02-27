/*
    I2C Master async polling

    Copyright (c) 2025  async by mztulip


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <Arduino.h>
#include "api/HardwareI2C.h"
#include <hardware/i2c.h>

#ifndef WIRE_BUFFER_SIZE
#define WIRE_BUFFER_SIZE 256
#endif

class AsyncTwoWire : public HardwareI2C {
public:
    AsyncTwoWire(i2c_inst_t *i2c, pin_size_t sda, pin_size_t scl);

    // Start as Master
    void begin() override;
    // Start as Slave
    void begin(uint8_t address) override;
    // Shut down the I2C interface
    void end() override;

    // Select IO pins to use.  Call before ::begin()
    bool setSDA(pin_size_t sda);
    bool setSCL(pin_size_t scl);

    void setClock(uint32_t freqHz) override;

    void beginTransmission(uint8_t) override;
    uint8_t endTransmission(bool stopBit) override;
    uint8_t endTransmission(void) override;

    size_t requestFrom(uint8_t address, size_t quantity, bool stopBit) override;
    size_t requestFrom(uint8_t address, size_t quantity) override;

    size_t write(uint8_t data) override;
    size_t write(const uint8_t * data, size_t quantity) override;

    inline size_t write(unsigned long n) {
        return write((uint8_t)n);
    }
    inline size_t write(long n) {
        return write((uint8_t)n);
    }
    inline size_t write(unsigned int n) {
        return write((uint8_t)n);
    }
    inline size_t write(int n) {
        return write((uint8_t)n);
    }
    using Print::write;


    virtual int available(void) override;
    virtual int read(void) override;
    virtual int peek(void) override;
    virtual void flush(void) override;

    void onReceive(void(*)(int));
    void onRequest(void(*)(void));

private:
    i2c_inst_t *_i2c;
    pin_size_t _sda;
    pin_size_t _scl;
    int _clkHz;

    uint8_t _addr;
    bool _running;
    bool _txBegun;

    uint8_t _buff[WIRE_BUFFER_SIZE];
    int _buffLen;
    int _buffOff;


    // TWI clock frequency
    static const uint32_t TWI_CLOCK = 100000;

};

extern AsyncTwoWire AsyncWire1;

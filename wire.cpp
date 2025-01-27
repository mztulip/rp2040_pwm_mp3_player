/*
    I2C Master/Slave library for the Raspberry Pi Pico RP2040

    Copyright (c) 2021 Earle F. Philhower, III <earlephilhower@yahoo.com>

    Based off of TWI/I2C library for Arduino Zero
    Copyright (c) 2015 Arduino LLC. All rights reserved.

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

#include <Arduino.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <hardware/regs/intctrl.h>
#include "wire.h"


AsyncTwoWire::AsyncTwoWire(i2c_inst_t *i2c, pin_size_t sda, pin_size_t scl) {
    _sda = sda;
    _scl = scl;
    _i2c = i2c;
    _clkHz = TWI_CLOCK;
    _running = false;
    _txBegun = false;
    _buffLen = 0;
}

bool AsyncTwoWire::setSDA(pin_size_t pin) {

    constexpr uint64_t valid[2] = { __bitset({0, 4, 8, 12, 16, 20, 24, 28}) /* I2C0 */,
                                    __bitset({2, 6, 10, 14, 18, 22, 26})  /* I2C1 */
                                  };

    if ((!_running) && ((1LL << pin) & valid[i2c_hw_index(_i2c)])) {
        _sda = pin;
        return true;
    }

    if (_sda == pin) {
        return true;
    }

    if (_running) {
        panic("FATAL: Attempting to set Wire%s.SDA while running", i2c_hw_index(_i2c) ? "1" : "");
    } else {
        panic("FATAL: Attempting to set Wire%s.SDA to illegal pin %d", i2c_hw_index(_i2c) ? "1" : "", pin);
    }
    return false;
}

bool AsyncTwoWire::setSCL(pin_size_t pin) {
    constexpr uint64_t valid[2] = { __bitset({1, 5, 9, 13, 17, 21, 25, 29}) /* I2C0 */,
                                    __bitset({3, 7, 11, 15, 19, 23, 27})  /* I2C1 */
                                  };

    if ((!_running) && ((1LL << pin) & valid[i2c_hw_index(_i2c)])) {
        _scl = pin;
        return true;
    }

    if (_scl == pin) {
        return true;
    }

    if (_running) {
        panic("FATAL: Attempting to set Wire%s.SCL while running", i2c_hw_index(_i2c) ? "1" : "");
    } else {
        panic("FATAL: Attempting to set Wire%s.SCL to illegal pin %d", i2c_hw_index(_i2c) ? "1" : "", pin);
    }
    return false;
}

void AsyncTwoWire::setClock(uint32_t hz) {
    _clkHz = hz;
    if (_running) {
        i2c_set_baudrate(_i2c, hz);
    }
}

// Master mode
void AsyncTwoWire::begin() {
    i2c_init(_i2c, _clkHz);
    i2c_set_slave_mode(_i2c, false, 0);
    gpio_set_function(_sda, GPIO_FUNC_I2C);
    gpio_pull_up(_sda);
    gpio_set_function(_scl, GPIO_FUNC_I2C);
    gpio_pull_up(_scl);

    _running = true;
    _txBegun = false;
    _buffLen = 0;
}
void AsyncTwoWire::begin(uint8_t addr) 
{
    //slave not implemented
}

void AsyncTwoWire::end()
{
    i2c_deinit(_i2c);

    pinMode(_sda, INPUT);
    pinMode(_scl, INPUT);
}

void AsyncTwoWire::beginTransmission(uint8_t addr) 
{
    if (!_running || _txBegun) {
        // ERROR
        return;
    }
    _addr = addr;
    _buffLen = 0;
    _buffOff = 0;
    _txBegun = true;
}

size_t AsyncTwoWire::requestFrom(uint8_t address, size_t quantity, bool stopBit) {
    return 0;
}

size_t AsyncTwoWire::requestFrom(uint8_t address, size_t quantity) {
    return requestFrom(address, quantity, true);
}

// Errors:
//  0 : Success
//  1 : Data too long
//  2 : NACK on transmit of address
//  3 : NACK on transmit of data
//  4 : Other error
//  5 : Timeout
uint8_t AsyncTwoWire::endTransmission(bool stopBit) {
       if (!_running || !_txBegun) {
        return 4;
    }
    _txBegun = false;

    auto len = _buffLen;
    // retx = i2c_write_blocking(i2c, addr, src, nbytes, false);
    auto ret = i2c_write_blocking(_i2c, _addr, _buff, _buffLen, !stopBit);

    _buffLen = 0;
    return (ret == len) ? 0 : 4;

    return 0;
}

uint8_t AsyncTwoWire::endTransmission() {
    return endTransmission(true);
}

size_t AsyncTwoWire::write(uint8_t ucData) {
    if (!_running) {
        return 0;
    }
    if (!_txBegun || (_buffLen == sizeof(_buff))) 
    {
        return 0;
    }
    _buff[_buffLen++] = ucData;
    return 1 ;
}

size_t AsyncTwoWire::write(const uint8_t *data, size_t quantity) {
    for (size_t i = 0; i < quantity; ++i) {
        if (!write(data[i])) {
            return i;
        }
    }

    return quantity;
}


void AsyncTwoWire::onReceive(void(*function)(int)) {
    // _onReceiveCallback = function;
}

void AsyncTwoWire::onRequest(void(*function)(void)) {
    // _onRequestCallback = function;
}

int AsyncTwoWire::available(void) {
    return _running  ? _buffLen - _buffOff : 0;
}

int AsyncTwoWire::read(void) {
    if (available()) {
        return _buff[_buffOff++];
    }
    return -1; // EOF
}

int AsyncTwoWire::peek(void) {
    if (available()) {
        return _buff[_buffOff];
    }
    return -1; // EOF
}

void AsyncTwoWire::flush(void) {
    // Do nothing, use endTransmission(..) to force
    // data transfer.
}

#ifndef __WIRE1_DEVICE
#define __WIRE1_DEVICE i2c1
#endif


#ifdef PIN_WIRE1_SDA
AsyncTwoWire AsyncWire1(__WIRE1_DEVICE, PIN_WIRE1_SDA, PIN_WIRE1_SCL);
#endif

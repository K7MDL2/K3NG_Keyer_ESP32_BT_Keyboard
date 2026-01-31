#include "IST8310.h"

IST8310::IST8310() {
}

bool IST8310::setup(TwoWire *wire, Print *print) {
    this->_wire = wire;
    // this->_leds = leds;
    this->_print = print;
    this->_i2c_address = IST8310_I2C_ADDRESS;

    this->_raw.x = 0.0;
    this->_raw.y = 0.0;
    this->_raw.z = 0.0;
    this->_flip_x_y = false;

    bool success = this->soft_reset();
    if (!success) {
        this->_print->println("fail to soft reset ist8310");
        return false;
    }

    return true;
}

bool IST8310::read_device_id() {
    bool success = this->read_register(IST8310_REGISTER_WHO_AM_I, &this->_device_id);
    if (!success) {
        return false;
    }

    if (this->_device_id != IST8310_DEVICE_ID) {
        return false;
    }

    return true;
}

bool IST8310::soft_reset() {
    uint8_t reset_bit = 0x1;
    bool success = this->write_register(IST8310_REGISTER_CNTL2, reset_bit);
    if (!success) {
        this->_print->println("fail to write soft reset bit to ist8310");
        return false;
    }

    for (size_t i = 0; i < IST8310_RESET_RETRIES; i++) {
        success = this->read_device_id();
        if (!success) {
            continue;
        }

        uint8_t value;
        success = this->read_register(IST8310_REGISTER_CNTL2, &value);
        if (!success) {
            continue;
        }

        if ((value & reset_bit) == 0) {
            return true;
        }
    }

    return false;
}

bool IST8310::read_register(uint8_t reg, uint8_t *value) {
    this->_wire->beginTransmission(this->_i2c_address);
    this->_wire->write(reg);
    uint8_t status = _wire->endTransmission();

    if (status != 0) {
        return false;
    }

    uint8_t length = 1;
    uint8_t n = _wire->requestFrom(this->_i2c_address, length);
    if (n != length) {
        return false;
    }

    *value = this->_wire->read();

    return true;
}

bool IST8310::write_register(uint8_t reg, uint8_t value) {
    this->_wire->beginTransmission(this->_i2c_address);
    this->_wire->write(reg);
    this->_wire->write(value);
    uint8_t status = _wire->endTransmission(true);

    return status == 0;
}

bool IST8310::read() {
    bool success = this->write_register(IST8310_REGISTER_CNTL1, 0x1);
    if (!success) {
        return false;
    }

    this->_wire->beginTransmission(this->_i2c_address);
    this->_wire->write(0x02);
    uint8_t status = _wire->endTransmission();
    if (status != 0) {
        return false;
    }

    uint8_t length = 7;
    uint8_t n = this->_wire->requestFrom(this->_i2c_address, length);
    if (n != length) {
        return false;
    }

    status = this->_wire->read();

    // read low / high x
    int16_t x_low = this->_wire->read();
    int16_t x_high = this->_wire->read();

    int16_t x = (x_high << 8) | x_low;

    // read low / high y
    int16_t y_low = this->_wire->read();
    int16_t y_high = this->_wire->read();

    int16_t y = (y_high << 8) | y_low;

    // read low / high z
    int16_t z_low = this->_wire->read();
    int16_t z_high = this->_wire->read();

    int16_t z = (z_high << 8) | z_low;

    this->_raw.x = float(x);
    this->_raw.y = float(y);
    this->_raw.z = float(z);

    return true;
}

bool IST8310::update() {
    bool success = this->read();
    if (!success) {
        return false;
    }

    float x = this->_raw.x;
    float y = this->_raw.y;

    // check if x and y needs to be fliped
    if (this->_flip_x_y) {
        x = this->_raw.y;
        y = this->_raw.x;
    }

    this->_magnetometer.x = x;
    this->_magnetometer.y = y;
    this->_magnetometer.z = this->_raw.z;

    return true;
}

bool IST8310::set_average(IST8310AverageY y, IST8310AverageXZ xz) {
    uint8_t value = (uint8_t(y) << 2) & uint8_t(xz);
    bool success = this->write_register(IST8310_REGISTER_AVGCNTL, value);
    if (!success) {
        return false;
    }
    return true;
}

Vec3f *IST8310::get_raw() {
    return &this->_raw;
}

Vec3f *IST8310::get_magnetometer() {
    return &this->_magnetometer;
}

bool IST8310::set_selftest(bool enabled) {
    uint8_t value = 0x0;
    if (enabled) {
        value = 1 << 6;
    }
    bool success = this->write_register(IST8310_REGISTER_STR, value);
    if (!success) {
        return false;
    }
    return true;
}

void IST8310::set_flip_x_y(bool flip) {
    this->_flip_x_y = flip;
}

void IST8310::set_declination_offset_radians(float declination_radians) {
    this->_declination_offset_radians = declination_radians;
}

bool IST8310::loop_read(Vec3f &sum, size_t loops, int timeout) {
    for (size_t i = 0; i < loops; i++) {
        bool success = this->read();
        if (!success) {
            return false;
        }

        sum.x += this->_raw.x;
        sum.y += this->_raw.y;
        sum.z += this->_raw.z;

        // this->_leds->sleep(timeout);
    }

    return true;
}

bool IST8310::calibration() {

    Vec3f test(0.0, 0.0, 0.0);
    Vec3f normal(0.0, 0.0, 0.0);

    Vec3f *tmp = &normal;

    for (size_t i = 0; i < 2; i++) {
        if (i == 1) {
            bool success = this->set_selftest(true);
            if (!success) {
                return false;
            }

            tmp = &test;
        }

        for (size_t j = 0; j < 30; j++) {
            bool success = this->read();
            if (!success) {
                return false;
            }

            if (j > 10) {
                tmp->x += this->_raw.x;
                tmp->y += this->_raw.y;
                tmp->z += this->_raw.z;
            }
        }
    }

    bool success = this->set_selftest(false);
    if (!success) {
        return false;
    }

    float x = fabsf(test.x - normal.x);
    float y = fabsf(test.y - normal.y);
    float z = fabsf(test.z - normal.z);
    float length = x + y + z;

    return true;
}


float IST8310::get_heading_degrees() {

    Vec3f *mag_latest_value = this -> get_magnetometer();

    // I followed these calculations from https://github.com/sleemanj/HMC5883L_Simple
    float mag_west = 0 - mag_latest_value->x;
    float mag_north = mag_latest_value->y;

    float heading = atan2(mag_west, mag_north);

    // Adjust the heading by the declination
    heading += this->_declination_offset_radians;

    // Correct for when signs are reversed.
    if(heading < 0)
        heading += 2*M_PI;
        
    // Check for wrap due to addition of declination.
    if(heading > 2*M_PI)
        heading -= 2*M_PI;
    
    // Convert radians to degrees
    return heading * 180/M_PI; 
}
#ifndef _MAGNETOMETER_H
#define _MAGNETOMETER_H

#include "vec3f.h"

class Magnetometer {
public:       
    virtual Vec3f *get_magnetometer() = 0;
};

#endif // _MAGNETOMETER_H
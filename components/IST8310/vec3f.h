#ifndef _VEC_3F_H
#define _VEC_3F_H

#include <math.h>
#include <Arduino.h>

#define VEC_3F_LEN_ZERO_EPSILON 0.00001

class Vec3f
{
public:
    Vec3f();
    Vec3f(float x, float y, float z);

    Vec3f set(float x, float y, float z);

    Vec3f invert();
    Vec3f add(Vec3f &n);
    Vec3f subtract(Vec3f &n);
    Vec3f scale_scalar(float s);
    Vec3f divide_scalar(float s);
    Vec3f multiply(Vec3f &n);
    
    float length();
    float length_squared();

    float dot(Vec3f &n);
    float angle_to(Vec3f &n);

    Vec3f normalize();

    Vec3f clone();

    float x;
    float y;
    float z;
};

#endif // _VEC_3F_H

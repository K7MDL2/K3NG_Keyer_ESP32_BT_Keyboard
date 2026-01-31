#include "vec3f.h"

Vec3f::Vec3f() {
    this->x = 0.0;
    this->y = 0.0;
    this->z = 0.0;
}

Vec3f::Vec3f(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

Vec3f Vec3f::set(float x, float y, float z) {
    Vec3f v(x, y, z);
    return v;
}

Vec3f Vec3f::scale_scalar(float s) {
    Vec3f v(this->x * s, this->y * s, this->z * s);
    return v;
}

Vec3f Vec3f::invert() {
    Vec3f v(this->x * -1.0, this->y * -1.0, this->z * -1.0);
    return v;
}

Vec3f Vec3f::divide_scalar(float s) {
    Vec3f v(this->x / s, this->y / s, this->z / s);
    return v;
}

Vec3f Vec3f::add(Vec3f &n) {
    Vec3f v(this->x + n.x, this->y + n.y, this->z + n.z);
    return v;
}

Vec3f Vec3f::subtract(Vec3f &n) {
    Vec3f v(this->x - n.x, this->y - n.y, this->z - n.z);
    return v;
}

Vec3f Vec3f::multiply(Vec3f &n) {
    Vec3f v(this->x * n.x, this->y * n.y, this->z * n.z);
    return v;
}

float Vec3f::length() {
    float ls = this->length_squared();
    return sqrt(ls);
}

float Vec3f::length_squared() {
    return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}

Vec3f Vec3f::normalize() {
    float l = this->length();
    if (l < VEC_3F_LEN_ZERO_EPSILON) {
        return Vec3f();
    }

    return this->divide_scalar(l);
}

Vec3f Vec3f::clone() {
    Vec3f v(this->x, this->y, this->z);
    return v;
}

float Vec3f::dot(Vec3f &n) {
    return this->x * n.x + this->y * n.y + this->z * n.z;
}

float Vec3f::angle_to(Vec3f &n) {
    float denominator = sqrt(n.length_squared() * this->length_squared());
    if (denominator < VEC_3F_LEN_ZERO_EPSILON) {
        return PI / 2.0;
    }

    float theta = this->dot(n) / denominator;
    float a = acos(constrain(theta, -1.0, 1.0));
    return a;
}
/**
  @file Vector3int32.h
  
  @maintainer Morgan McGuire, matrix@brown.edu

  @created 2008-07-01
  @edited  2008-07-01
  Copyright 2000-2008, Morgan McGuire.
  All rights reserved.
 */

#ifndef VECTOR3INT32_H
#define VECTOR3INT32_H

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {

/**
 A Vector3 that packs its fields into uint32s.
 */
#ifdef G3D_WIN32
    // Switch to tight alignment
    #pragma pack(push, 4)
#endif

class Vector3int32 {
private:
    // Hidden operators
    bool operator<(const Vector3int32&) const;
    bool operator>(const Vector3int32&) const;
    bool operator<=(const Vector3int32&) const;
    bool operator>=(const Vector3int32&) const;

public:
    G3D::int32              x;
    G3D::int32              y;
    G3D::int32              z;

    Vector3int32() : x(0), y(0), z(0) {}
    Vector3int32(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
    Vector3int32(const class Vector3& v);
    Vector3int32(class BinaryInput& bi);

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);

    inline G3D::int32& operator[] (int i) {
        debugAssert(i <= 2);
        return ((G3D::int32*)this)[i];
    }

    inline const G3D::int32& operator[] (int i) const {
        debugAssert(i <= 2);
        return ((G3D::int32*)this)[i];
    }

    inline Vector3int32 operator+(const Vector3int32& other) const {
        return Vector3int32(x + other.x, y + other.y, z + other.z);
    }

    inline Vector3int32 operator-(const Vector3int32& other) const {
        return Vector3int32(x - other.x, y - other.y, z - other.z);
    }

    inline Vector3int32 operator*(const Vector3int32& other) const {
        return Vector3int32(x * other.x, y * other.y, z * other.z);
    }

    inline Vector3int32 operator*(const int s) const {
        return Vector3int32(x * s, y * s, z * s);
    }

    inline Vector3int32& operator+=(const Vector3int32& other) {
        x += other.x;
        y += other.y;
        z += other.y;
        return *this;
    }

    inline Vector3int32& operator-=(const Vector3int32& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline Vector3int32& operator*=(const Vector3int32& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    inline bool operator== (const Vector3int32& rkVector) const {
        return ( x == rkVector.x && y == rkVector.y && z == rkVector.z );
    }

    inline bool operator!= (const Vector3int32& rkVector) const {
        return ( x != rkVector.x || y != rkVector.y || z != rkVector.z );
    }

    std::string toString() const;
}
#if defined(G3D_LINUX) || defined(G3D_OSX)
    __attribute((aligned(1)))
#endif
;

#ifdef G3D_WIN32
    #pragma pack(pop)
#endif

}
#endif

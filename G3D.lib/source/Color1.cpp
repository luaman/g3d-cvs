/**
 @file Color1.cpp

 Color class.

 @author Morgan McGuire, morgan@cs.williams.edu

 @created 2007-01-30
 @edited  2009-03-27
 */

#include "G3D/platform.h"
#include "G3D/Color1.h"
#include "G3D/Color1uint8.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Color3.h"

namespace G3D {

Color1::Color1(BinaryInput& bi) {
    deserialize(bi);
}


Color3 Color1::rgb() const {
    return Color3(value, value, value);
}


void Color1::deserialize(BinaryInput& bi) {
    value = bi.readFloat32();
}


void Color1::serialize(BinaryOutput& bo) const {
    bo.writeFloat32(value);
}


Color1::Color1(const class Color1uint8& other) {
    value = other.value / 255.0f;
}

} // namespace G3D


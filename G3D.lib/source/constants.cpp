/**
  @file constants.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2009-05-20
  @edited  2010-01-29
*/
#include "G3D/constants.h"
#include "G3D/Any.h"
#include "G3D/stringutils.h"

namespace G3D {

const std::string MirrorQuality::str[] = {"NONE", "STATIC_ENV", "DYNAMIC_PLANAR", "DYNAMIC_ENV", "BEST"};

MirrorQuality::MirrorQuality(const class Any& any) {
    *this = any;
}


MirrorQuality& MirrorQuality::operator=(const Any& any) {
    const std::string& s = toUpper(any.string());

    for (int i = 0; ! str[i].empty(); ++i) {
        if (s == str[i]) {
            value = Value(i);
            return *this;
        }
    }

    any.verify(false, "Unrecognized MirrorQuality constant");
    return *this;
}


MirrorQuality::operator Any() const {
    return toString();
}


const std::string& MirrorQuality::toString() const {
    return str[value];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string RefractionQuality::str[] = {"NONE", "STATIC_ENV", "DYNAMIC_FLAT", "DYNAMIC_ENV", "BEST"};

RefractionQuality::RefractionQuality(const class Any& any) {
    *this = any;
}


RefractionQuality& RefractionQuality::operator=(const Any& any) {
    const std::string& s = toUpper(any.string());

    for (int i = 0; ! str[i].empty(); ++i) {
        if (s == str[i]) {
            value = Value(i);
            return *this;
        }
    }

    any.verify(false, "Unrecognized RefractionQuality constant");
    return *this;
}


RefractionQuality::operator Any() const {
    return toString();
}


const std::string& RefractionQuality::toString() const {
    return str[value];
}

} // G3D

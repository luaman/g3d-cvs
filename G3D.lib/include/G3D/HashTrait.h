/**
  @file HashTrait.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2008-10-01
  @edited  2008-10-01
  Copyright 2000-2008, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_HASHTRAIT_H
#define G3D_HASHTRAIT_H

#include "G3D/platform.h"

/** @deprecated Use HashTrait */
template<typename Key>
struct GHashCode{};

/** Adapter for making old GHashCode functions still work */
template <class T>
struct HashTrait {
    static size_t hashCode(const T& k) {
        static GHashCode<T> f;
        return f(k);
    }
};

#endif

/**
 @file Random.h
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2009-01-02
 @edited  2009-01-02

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Random_h
#define G3D_Random_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {

/** Random number generator.

    Useful for generating consistent random numbers across platforms
    and when multiple threads are involved.

    The base implementation uses the SIMD Fast Mersenne Twister
    (SFMT-19937) algorithm.  

    @cite http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html
 */
class Random {
private:

    /** Constants */
    enum {
        N = 624,
        M = 397,
        R = 31,
        U = 11,
        S = 7,
        T = 15,
        L = 18,
        A = 0x9908B0DF,
        B = 0x9D2C5680,
        C = 0xEFC60000};

    /** State vector (these are the next N values that will be returned) */
    uint32     state[N];

    /** Index into state */
    int        index;

public:

    Random(uint32 seed = 0xF018A4D2);

    virtual ~Random();

    /** Each bit is random */
    virtual uint32 bits();

    /** Uniform random integer on the range [min, max] */
    virtual int integer(int min, int max);

    /** Uniform random float on the range [min, max] */
    virtual float uniform(float min, float max);

    /** Normally distributed reals. */
    virtual float gaussian(float mean, float stdev);
};

}

#endif

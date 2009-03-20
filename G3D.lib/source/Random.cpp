/**
 @file Random.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2009-01-02
 @edited  2009-01-02

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/Random.h"

namespace G3D {

Random::Random(uint32 seed) : mutex(0) {
    const uint32 X = 1812433253UL;

    state[0] = seed;
    for (index = 1; index < (int)N; ++index) {
        state[index] = X * (state[index - 1] ^ (state[index - 1] >> 30)) + index;
    }
}


Random::~Random() {
}


/** Generate the next N ints, and store them for readback later */
void Random::generate() {
    // Lower R bits
    static const uint32 LOWER_MASK = (1LU << R) - 1;

    // Upper (32 - R) bits
    static const uint32 UPPER_MASK = 0xFFFFFFFF << R;
    static const uint32 mag01[2] = {0, A};

    // First N - M
    for (unsigned int i = 0; i < N - M; ++i) {    
        uint32 x = (state[i] & UPPER_MASK) | (state[i + 1] & LOWER_MASK);
        state[i] = state[i + M] ^ (x >> 1) ^ mag01[x & 1];
    }

    // Rest
    for (unsigned int i = N - M + 1; i < N - 1; ++i) {    
        uint32 x = (state[i] & UPPER_MASK) | (state[i + 1] & LOWER_MASK);
        state[i] = state[i + (M - N)] ^ (x >> 1) ^ mag01[x & 1];
    }
        
    uint32 y = (state[N - 1] & UPPER_MASK) | (state[0] & LOWER_MASK);
    state[N - 1] = state[M - 1] ^ (y >> 1) ^ mag01[y & 1];
    index = 0;
}

    
int Random::integer(int low, int high) {
    int r = iFloor(low + (high - low + 1) * (double)bits() / 0xFFFFFFFFUL);

    // There is a *very small* chance of generating
    // a number larger than high.
    if (r > high) {
        return high;
    } else {
        return r;
    }
}

    
float Random::gaussian(float mean, float stdev) {

    // Using Box-Mueller method from http://www.taygeta.com/random/gaussian.html
    // Modified to specify standard deviation and mean of distribution
    float w, x1, x2;

    // Loop until w is less than 1 so that log(w) is negative
    do {
        x1 = uniform(-1.0, 1.0);
        x2 = uniform(-1.0, 1.0);

        w = float(square(x1) + square(x2));
    } while (w > 1.0f);

    // Transform to gassian distribution
    // Multiply by sigma (stdev ^ 2) and add mean.
    return x2 * (float)square(stdev) * sqrtf((-2.0f * logf(w) ) / w) + mean; 
}


} // G3D

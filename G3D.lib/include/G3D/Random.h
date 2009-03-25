/**
 @file Random.h
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2009-01-02
 @edited  2009-03-20

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Random_h
#define G3D_Random_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/GThread.h"

namespace G3D {

/** Random number generator.

    Useful for generating consistent random numbers across platforms
    and when multiple threads are involved.

    Uses the Fast Mersenne Twister (FMT-19937) algorithm.

    On average, uniform() runs about 2x-3x faster than rand().

    @cite http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html
 */
class Random {
private:

    /** Constants (important for the algorithm; do not modify) */
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

    /** Prevents multiple overlapping calls to generate(). 
     */
    Spinlock     lock;

    /** State vector (these are the next N values that will be returned) */
    uint32       state[N];

    /** Index into state */
    int          index;

    /** Generate the next N ints, and store them for readback later.
        Called from bits() */
    void generate();

public:

    Random(uint32 seed = 0xF018A4D2);

    ~Random();

    /** Each bit is random */
    inline uint32 bits() {
        // See http://en.wikipedia.org/wiki/Mersenne_twister

        // Make a local copy of the index variable to ensure that it
        // is not out of bounds
        int localIndex = index;

        // Automatically checks for index < 0 if corrupted
        // by unsynchronized threads.
        if ((unsigned int)localIndex >= (unsigned int)N) {
            generate();
            localIndex = 0;
        }
        // Increment the global index.  It may go out of bounds on
        // multiple threads, but the above check ensures that the
        // array index actually used never goes out of bounds.
        // It doesn't matter if we grab the same array index twice
        // on two threads, since the distribution of random numbers
        // will still be uniform.
        ++index;
        // Return the next random in the sequence
        uint32 r = state[localIndex];

        // Temper the result
        r ^=  r >> U;
        r ^= (r << S) & B;
        r ^= (r << T) & C;
        r ^=  r >> L;
        
        return r;    
    }

    /** Uniform random integer on the range [min, max] */
    int integer(int min, int max);

    /** Uniform random float on the range [min, max] */
    inline float uniform(float low, float high) {
        // We could compute the ratio in double precision here for
        // about 1.5x slower performance and slightly better
        // precision.
        return low + (high - low) * ((float)bits() / (float)0xFFFFFFFFUL);
    }

    /** Uniform random float on the range [0, 1] */
    inline float uniform() {
        // We could compute the ratio in double precision here for
        // about 1.5x slower performance and slightly better
        // precision.
        return (float)bits() / (float)0xFFFFFFFFUL;
    }

    /** Normally distributed reals. */
    float gaussian(float mean, float stdev);

    /**
       A shared instance for when the performance and features but not
       consistency of the class are desired.  It is slightly (10%)
       faster to use a distinct instance than to use the common one.
       
       Threadsafe.
    */
    inline static Random& common() {
        static Random r;
        return r;
    }
};

}

#endif

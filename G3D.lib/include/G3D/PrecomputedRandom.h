/**
 @file PrecomputedRandom.h
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2009-03-31
 @edited  2009-03-31

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_PrecomputedRandom_h
#define G3D_PrecomputedRandom_h

#include "G3D/platform.h"
#include "G3D/Random.h"

namespace G3D {

/** Fast uniform and cosHemi using precomputed data table. 
    Integers are computed using the Mersenne twister.

    Generates cosHemi about 13x faster than Random.

    Not threadsafe.*/
class PrecomputedRandom : public Random {
public:
    /** Put the cosHemi and the uniform together so that when
        alternating between them we stay in cache */
    class Data {
    public:
        float   cosHemiX;
        float   cosHemiY;
        float   cosHemiZ;
        float   uniform;
    };
protected:

    /** Array of 2^n elements. */
    Data*       m_data;

    /** 2^n - 1; the AND mask for computing a fast modulo */
    int         m_modMask;

    /** Index into m_data */
    int         m_index;

public:

    /** \param threadsafe Set to false if you know that this random
        will only be used on a single thread.  This eliminates the
        lock and improves performance on some platforms.

        \param dataSize Must be a power of 2

        \param data Will NOT be deleted by the destructor.
     */
    PrecomputedRandom(Data* data, int dataSize, uint32 seed = 0xF018A4D2);

    /** Uniform random float on the range [min, max] */
    virtual float uniform(float low, float high);

    /** Uniform random float on the range [0, 1] */
    virtual float uniform();

    /** Returns 3D unit vectors distributed according to 
        a cosine distribution about the z axis. */
    virtual void cosHemi(float& x, float& y, float& z);
};

}

#endif

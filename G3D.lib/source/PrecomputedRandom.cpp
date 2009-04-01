/**
 @file PrecomputedRandom.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2009-03-31
 @edited  2009-03-31

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/PrecomputedRandom.h"

namespace G3D {

PrecomputedRandom::PrecomputedRandom(Data* data, int dataSize, uint32 seed) : 
 Random(seed, false),
 m_data(data),
 m_modMask(dataSize - 1) {

    m_index = seed & m_modMask;
    alwaysAssertM(isPow2(dataSize), "dataSize must be a power of 2");
}


float PrecomputedRandom::uniform(float low, float high) {
    m_index = (m_index + 1) & m_modMask;
    return low + m_data[m_index].uniform * (high - low);
}


float PrecomputedRandom::uniform() {
    m_index = (m_index + 1) & m_modMask;
    return m_data[m_index].uniform;
}


void PrecomputedRandom::cosHemi(float& x, float& y, float& z) {
    m_index = (m_index + 1) & m_modMask;
    x = m_data[m_index].cosHemiX;
    y = m_data[m_index].cosHemiY;
    z = m_data[m_index].cosHemiZ;
}

}
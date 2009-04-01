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

PrecomputedRandom::PrecomputedRandom(const HemiUniformData* data1, const SphereBitsData* data2, int dataSize, uint32 seed) : 
 Random((void*)NULL),
 m_hemiUniform(data1),
 m_sphereBits(data2),
 m_modMask(dataSize - 1) {

    m_index = seed & m_modMask;
    alwaysAssertM(isPow2(dataSize), "dataSize must be a power of 2");
}


float PrecomputedRandom::uniform(float low, float high) {
    m_index = (m_index + 1) & m_modMask;
    return low + m_hemiUniform[m_index].uniform * (high - low);
}


float PrecomputedRandom::uniform() {
    m_index = (m_index + 1) & m_modMask;
    return m_hemiUniform[m_index].uniform;
}


void PrecomputedRandom::cosHemi(float& x, float& y, float& z) {
    m_index = (m_index + 1) & m_modMask;
    x = m_hemiUniform[m_index].cosHemiX;
    y = m_hemiUniform[m_index].cosHemiY;
    z = m_hemiUniform[m_index].cosHemiZ;
}


uint32 PrecomputedRandom::bits() {
    m_index = (m_index + 1) & m_modMask;
    return m_sphereBits[m_index].bits;
}


void PrecomputedRandom::sphere(float& x, float& y, float& z) {
    m_index = (m_index + 1) & m_modMask;
    x = m_sphereBits[m_index].sphereX;
    y = m_sphereBits[m_index].sphereY;
    z = m_sphereBits[m_index].sphereZ;
}

}
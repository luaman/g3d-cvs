/**
 @file CoherentAllocator.cpp
  
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2009-01-20
 @edited  2009-01-20

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/CoherentAllocator.h"
#include "G3D/System.h"

namespace G3D {

CoherentAllocator::Buffer::Buffer(size_t size) : m_size(size), m_used(0) {
    // Allocate space for a lot of buffers.
    m_first = (uint8*)::malloc(m_size);
}


CoherentAllocator::Buffer::~Buffer() {
    ::free(m_first);
}


void* CoherentAllocator::Buffer::malloc(size_t s) {
    if (s + m_used > m_size) {
        return NULL;
    } else {
        void* old = m_first + m_used;
        m_used += s;
        return old;
    }
}


CoherentAllocator::CoherentAllocator(size_t sizeHint) : m_sizeHint(sizeHint) {
    debugAssert(sizeHint > 0);
}


CoherentAllocator::~CoherentAllocator() {
    deallocateAll();
}


size_t CoherentAllocator::sizeHint() const {
    return m_sizeHint;
}


size_t CoherentAllocator::bytesAllocated() const {
    return m_sizeHint * m_bufferArray.size();
}


void* CoherentAllocator::malloc(size_t s) {
    debugAssert(s <= m_sizeHint);

    void* n = (m_bufferArray.size() > 0) ? m_bufferArray.last()->malloc(s) : NULL;
    if (n == NULL) {
        // This buffer is full
        m_bufferArray.append(new Buffer(m_sizeHint));
        return m_bufferArray.last()->malloc(s);
    } else {
        return n;
    }
}


void CoherentAllocator::free(void* x) {
    // Intentionally empty; we block deallocate
}


void CoherentAllocator::deallocateAll() {
    m_bufferArray.deleteAll();
    m_bufferArray.clear();
}

}
/**
 @file CoherentAllocator.h
  
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2009-01-20
 @edited  2009-01-20

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */


#ifndef G3D_CoherentAllocator_h
#define G3D_CoherentAllocator_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Array.h"

namespace G3D {

/** 
  @brief Allocates memory in large blocks and then frees it as an area.

  Useful for ensuring cache coherence and for reducing the time cost of 
  multiple allocations and deallocations. 

  <b>Not threadsafe</b>
 */
class CoherentAllocator {
private:

    class Buffer {
    private:
        uint8*              m_first;
        size_t              m_size;
        size_t              m_used;

    public:

        Buffer(size_t size);

        ~Buffer();

        /** Returns NULL if out of space */
        void* malloc(size_t s);
    };
    
    size_t                  m_sizeHint;
    Array<Buffer*>          m_bufferArray;

public:

    /** 
    @param sizeHint
    */
    CoherentAllocator(size_t sizeHint = 100 * 100);

    /** Invokes deallocateAll. */
    ~CoherentAllocator();

    /** Maximum block size.  Normally set to much larger than the desired 
        allocation block size. */
    size_t sizeHint() const;

    size_t bytesAllocated() const;

    /** Allocates memory out of the buffer pool. 
        @param s must be no larger than sizeHint */
    void* malloc(size_t s);

    /** Ignored. */
    void free(void* x);

    /** Deletes all previously allocated memory. */
    void deallocateAll();
};

}

#endif

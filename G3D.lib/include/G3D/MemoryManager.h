/**
  @file MemoryManager.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2009-04-20
  @edited  2009-04-20

  Copyright 2000-2009, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_MemoryManager_h
#define G3D_MemoryManager_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"

namespace G3D {

/** 
   Abstraction of memory management.
   Default implementation uses G3D::System::malloc. 

   Threadsafe.
   \sa CRTMemoryManager. */
class MemoryManager : public ReferenceCountedObject {
protected:
    MemoryManager();

public:
    typedef ReferenceCountedPointer<class MemoryManager> Ref;
    virtual void* malloc(size_t s);
    virtual void free(void* ptr);

    /** There's only one instance of this memory manager; it is 
        cached after the first creation. */
    static MemoryManager::Ref create();
};


/** MemoryManager implemented using the C runtime. */
class CRTMemoryManager : public MemoryManager {
protected:
    CRTMemoryManager();

public:
    typedef ReferenceCountedPointer<class MemoryManager> Ref;
    virtual void* malloc(size_t s);
    virtual void free(void* ptr);

    /** There's only one instance of this memory manager; it is 
        cached after the first creation. */
    static CRTMemoryManager::Ref create();
};

}

#endif

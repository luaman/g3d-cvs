/**
  @file MemoryManager.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2009-04-20
  @edited  2009-04-20

  Copyright 2000-2009, Morgan McGuire.
  All rights reserved.
 */

#include "G3D/MemoryManager.h"
#include "G3D/System.h"

namespace G3D {

MemoryManager::MemoryManager() {}


void* MemoryManager::malloc(size_t s) {
    return System::malloc(s);
}


void MemoryManager::free(void* ptr) {
    System::free(ptr);
}


MemoryManager::Ref MemoryManager::create() {
    static MemoryManager::Ref m = new MemoryManager();
    return m;
}


///////////////////////////////////////////////////

CRTMemoryManager::CRTMemoryManager() {}


void* CRTMemoryManager::malloc(size_t s) {
    return ::malloc(s);
}

void CRTMemoryManager::free(void* ptr) {
    return ::free(ptr);
}


CRTMemoryManager::Ref CRTMemoryManager::create() {
    static CRTMemoryManager::Ref m = new CRTMemoryManager();
    return m;
}
}

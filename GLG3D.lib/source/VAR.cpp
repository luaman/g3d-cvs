/**
 @file VAR.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2003-04-08
 @edited  2008-12-24
 */

#include "GLG3D/VAR.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VARArea.h"
#include "G3D/Log.h"
#include "GLG3D/getOpenGLState.h"

namespace G3D {

VAR::VAR() : m_area(NULL), _pointer(NULL), elementSize(0), 
             numElements(0), m_stride(0), generation(0), 
             underlyingRepresentation(GL_NONE), _maxSize(0) {
}


VAR::VAR(size_t numBytes, VARAreaRef _area) : m_area(NULL), _pointer(NULL), elementSize(0), 
             numElements(0), m_stride(0), generation(0), 
             underlyingRepresentation(GL_NONE), _maxSize(0) {
    init(NULL, numBytes, _area, GL_NONE, 1);
}


bool VAR::valid() const {
    return
        (! m_area.isNull()) && 
        (m_area->currentGeneration() == generation) &&
        // If we're in VBO_MEMORY mode, the pointer can be null.  Otherwise
        // it shouldn't be
        (m_area->mode == VARArea::VBO_MEMORY || _pointer);
}


void VAR::init(VAR& dstPtr, size_t dstOffset, GLenum glformat, size_t eltSize, 
               int _numElements, size_t dstStride) {
    m_area = dstPtr.m_area;
    alwaysAssertM(m_area.notNull(), "Bad VARArea");
    numElements              = _numElements;
    underlyingRepresentation = glformat;
    elementSize              = eltSize;
    m_stride                 = dstStride;
    _maxSize                 = dstPtr._maxSize / dstStride;

    generation = m_area->currentGeneration();
    _pointer = (uint8*)dstPtr._pointer + dstOffset;
    debugAssertGLOk();
}


void VAR::init(const void* srcPtr,
               int     _numElements, 
               int     srcStride,      
               GLenum  glformat, 
               size_t  eltSize,
               VAR     dstPtr,
               size_t  dstOffset, 
               size_t  dstStride) {
    debugAssertGLOk();

    m_area = dstPtr.m_area;
    alwaysAssertM(m_area.notNull(), "Bad VARArea");

    numElements              = _numElements;
    underlyingRepresentation = glformat;
    elementSize              = eltSize;
    m_stride                 = dstStride;
    _maxSize                 = dstPtr._maxSize / dstStride;

    debugAssertM(
        (elementSize % sizeOfGLFormat(underlyingRepresentation)) == 0,
        "Sanity check failed on OpenGL data format; you may"
        " be using an unsupported type in a vertex array.");

    generation = m_area->currentGeneration();

    _pointer = (uint8*)dstPtr._pointer + dstOffset;

    // Upload the data
    if (_numElements > 0) {
        uploadToCardStride(srcPtr, _numElements, eltSize, srcStride, 0, dstStride);
    }
    debugAssertGLOk();
}


void VAR::init(
    const void*         sourcePtr,
    int                 _numElements,
    VARAreaRef          _area,
    GLenum              glformat,
    size_t              eltSize) {

    alwaysAssertM(! _area.isNull(), "Bad VARArea");

    numElements              = _numElements;
    m_area                   = _area;
    underlyingRepresentation = glformat;
    elementSize              = eltSize;
    m_stride                 = eltSize;

    size_t size              = elementSize * numElements;
    _maxSize                 = size;

    debugAssertM(
                 (sourcePtr == NULL) ||
                 (elementSize % sizeOfGLFormat(underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");

    generation = m_area->currentGeneration();

    _pointer = (uint8*)m_area->openGLBasePointer() + m_area->allocatedSize();

    // Align to the nearest multiple of this many bytes.
    const size_t alignment = 4;

    // Ensure that the next memory address is aligned.  This has 
    // a significant (up to 25%!) performance impact on some GPUs
    size_t pointerOffset = ((alignment - (size_t)_pointer % alignment) % alignment);

    if (_numElements == 0) {
        pointerOffset = 0;
    }

    // Adjust pointer to new alignment
    _pointer = (uint8*)_pointer + pointerOffset;
    
    size_t newAlignedSize = size + pointerOffset;

    alwaysAssertM(newAlignedSize <= m_area->freeSize(),
                  "VARArea too small to hold new VAR (possibly due to rounding"
                  " to the nearest dword boundary).");

    // Upload the data
    if (size > 0 && sourcePtr != NULL) {
        // Update VARArea values
        m_area->updateAllocation(newAlignedSize);

        uploadToCard(sourcePtr, 0, size);
    }

    debugAssertGLOk();
}


void VAR::update(
    const void*         sourcePtr,
    int                 _numElements,
    GLenum              glformat,
    size_t              eltSize) {

    size_t size = eltSize * _numElements;

    debugAssert(m_stride == 0 || m_stride == elementSize);
    alwaysAssertM(size <= _maxSize,
        "A VAR can only be updated with an array that is smaller "
        "or equal size (in bytes) to the original array.");

    alwaysAssertM(generation == m_area->currentGeneration(),
        "The VARArea has been reset since this VAR was created.");

    numElements              = _numElements;
    underlyingRepresentation = glformat;
    elementSize              = eltSize;

    debugAssertM(
                 (elementSize % sizeOfGLFormat(underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");
	
    // Upload the data
    if (size > 0) {
        uploadToCard(sourcePtr, 0, size);
    }
    debugAssertGLOk();
}


void VAR::set(int index, const void* value, GLenum glformat, size_t eltSize) {
    debugAssert(m_stride == 0 || m_stride == elementSize);
    (void)glformat;
    debugAssertM(index < numElements && index >= 0, 
        "Cannot call VAR::set with out of bounds index");
    
    debugAssertM(glformat == underlyingRepresentation, 
        "Value argument to VAR::set must match the intialization type.");

    debugAssertM(eltSize == elementSize, 
        "Value argument to VAR::set must match the intialization type's memory footprint.");

    uploadToCard(value, index * eltSize, eltSize);
}


void* VAR::mapBuffer(GLenum permissions) {
    // Map buffer
    switch (m_area->mode) {
    case VARArea::VBO_MEMORY:
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        glBindBufferARB(m_area->openGLTarget(), m_area->glbuffer);
        return (uint8*)glMapBufferARB(m_area->openGLTarget(), permissions) + 
            (size_t)_pointer;
        break;

    case VARArea::MAIN_MEMORY:
        return (uint8*)_pointer;
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
        return NULL;
    }
}


void VAR::unmapBuffer() {
    switch (m_area->mode) {
    case VARArea::VBO_MEMORY:
        glUnmapBufferARB(m_area->openGLTarget());
        glBindBufferARB(m_area->openGLTarget(), GL_NONE);
        glPopClientAttrib();
        break;

    case VARArea::MAIN_MEMORY:
        // Empty
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
    }
    debugAssertGLOk();
}

void VAR::uploadToCardStride(const void* srcPointer, int srcElements, size_t srcSize, int srcStride, 
                        size_t dstPtrOffsetBytes, size_t dstStrideBytes) {
    
    if (srcStride == 0) {
        srcStride = srcSize;
    }

    if (dstStrideBytes == 0) {
        dstStrideBytes = srcSize;
    }

    uint8* dstPointer = (uint8*)mapBuffer(GL_WRITE_ONLY) + (size_t)dstPtrOffsetBytes;

    // Copy elements
    for (int i = 0; i < srcElements; ++i) {
        System::memcpy(dstPointer, srcPointer, srcSize);
        srcPointer = (uint8*)srcPointer + srcStride;
        dstPointer = (uint8*)dstPointer + dstStrideBytes;
    }
    
    // Unmap buffer
    unmapBuffer();
    dstPointer = NULL;
    debugAssertGLOk();
}


void VAR::uploadToCard(const void* sourcePtr, int dstPtrOffset, size_t size) {
    debugAssertGLOk();
    debugAssert(m_stride == 0 || m_stride == elementSize);

    void* ptr = (void*)(reinterpret_cast<intptr_t>(_pointer) + dstPtrOffset);

    switch (m_area->mode) {
    case VARArea::VBO_MEMORY:
        // Don't destroy any existing bindings; this call can
        // be made at any time and the program might also
        // use VBO on its own.
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            glBindBufferARB(m_area->openGLTarget(), m_area->glbuffer);
            glBufferSubDataARB(m_area->openGLTarget(), (GLintptrARB)ptr, size, sourcePtr);
            glBindBufferARB(m_area->openGLTarget(), 0);
        }
        glPopClientAttrib();
        break;

    case VARArea::MAIN_MEMORY:
        System::memcpy(ptr, sourcePtr, size);
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
    }
    debugAssertGLOk();
}


// The following are called by the VARSystem.
void VAR::vertexPointer() const {
    debugAssert(valid());
    glEnableClientState(GL_VERTEX_ARRAY);
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_INT, 
                 "OpenGL does not support GL_UNSIGNED_INT as a vertex format.");
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_SHORT, 
                 "OpenGL does not support GL_UNSIGNED_SHORT as a vertex format.");
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_BYTE, 
                 "OpenGL does not support GL_UNSIGNED_BYTE as a vertex format.");
    glVertexPointer(elementSize / sizeOfGLFormat(underlyingRepresentation), 
                    underlyingRepresentation, m_stride, _pointer);
}


void VAR::normalPointer() const {
    debugAssert(valid());
    debugAssert((numElements == 0) || (double)elementSize / sizeOfGLFormat(underlyingRepresentation) == 3.0);
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_INT, 
                 "OpenGL does not support GL_UNSIGNED_INT as a normal format.");
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_SHORT, 
              "OpenGL does not support GL_UNSIGNED_SHORT as a normal format.");
    debugAssertM(underlyingRepresentation != GL_UNSIGNED_BYTE,
              "OpenGL does not support GL_UNSIGNED_BYTE as a normal format.");
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(underlyingRepresentation, m_stride, _pointer); 
}


void VAR::colorPointer() const {
    debugAssert(valid());
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(elementSize / sizeOfGLFormat(underlyingRepresentation),
                   underlyingRepresentation, m_stride, _pointer); 
}


void VAR::texCoordPointer(uint32 unit) const {
    debugAssert(valid());
    debugAssertM(GLCaps::supports_GL_ARB_multitexture() || (unit == 0),
        "Graphics card does not support multitexture");

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(elementSize / sizeOfGLFormat(underlyingRepresentation),
                      underlyingRepresentation, m_stride, _pointer);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
    }
}


void VAR::vertexAttribPointer(uint32 attribNum, bool normalize) const {
    debugAssert(valid());
    if (GLCaps::supports_GL_ARB_vertex_program()) {
        glEnableVertexAttribArrayARB(attribNum);
        glVertexAttribPointerARB(attribNum, elementSize / sizeOfGLFormat(underlyingRepresentation),
                                 underlyingRepresentation, normalize, m_stride, _pointer);
    }
}

}

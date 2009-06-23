/**
 @file VertexRange.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2003-04-08
 @edited  2008-12-24
 */

#include "GLG3D/VertexRange.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VertexBuffer.h"
#include "G3D/Log.h"
#include "GLG3D/getOpenGLState.h"

namespace G3D {

VertexRange::VertexRange() : m_area(NULL), m_pointer(NULL), m_elementSize(0), 
             m_numElements(0), m_stride(0), m_generation(0), 
             m_underlyingRepresentation(GL_NONE), m_maxSize(0) {
}


VertexRange::VertexRange(int numBytes, VertexBufferRef area) : m_area(NULL), m_pointer(NULL), m_elementSize(0), 
             m_numElements(0), m_stride(0), m_generation(0), 
             m_underlyingRepresentation(GL_NONE), m_maxSize(0) {

    init(NULL, numBytes, area, GL_NONE, 1);
}


bool VertexRange::valid() const {
    return
        (! m_area.isNull()) && 
        (m_area->currentGeneration() == m_generation) &&
        // If we're in VBO_MEMORY mode, the pointer can be null.  Otherwise
        // it shouldn't be
        (m_area->m_mode == VertexBuffer::VBO_MEMORY || m_pointer);
}


void VertexRange::init(VertexRange& dstPtr, int dstOffset, GLenum glformat, int eltSize, 
                int numElements, int dstStride) {
    m_area = dstPtr.m_area;
    alwaysAssertM(m_area.notNull(), "Bad VertexBuffer");
    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = dstStride;
    m_maxSize                  = dstPtr.m_maxSize / dstStride;

    m_generation = m_area->currentGeneration();
    m_pointer = (uint8*)dstPtr.m_pointer + dstOffset;
    debugAssertGLOk();
}


void VertexRange::init(const void* srcPtr,
               int          numElements, 
               int          srcStride,      
               GLenum       glformat, 
               int          eltSize,
               VertexRange  dstPtr,
               int          dstOffset, 
               int          dstStride) {
    debugAssertGLOk();

    m_area = dstPtr.m_area;
    alwaysAssertM(m_area.notNull(), "Bad VertexBuffer");

    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = dstStride;
    m_maxSize                  = dstPtr.m_maxSize / dstStride;

    debugAssertM(
        (m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
        "Sanity check failed on OpenGL data format; you may"
        " be using an unsupported type in a vertex array.");

    m_generation = m_area->currentGeneration();

    m_pointer = (uint8*)dstPtr.m_pointer + dstOffset;

    // Upload the data
    if (numElements > 0) {
        uploadToCardStride(srcPtr, numElements, eltSize, srcStride, 0, dstStride);
    }
    debugAssertGLOk();
}


void VertexRange::init(
    const void*         sourcePtr,
    int                 numElements,
    VertexBufferRef     area,
    GLenum              glformat,
    int                 eltSize) {

    alwaysAssertM(area.notNull(), "Bad VertexBuffer");

    m_numElements              = numElements;
    m_area                     = area;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = eltSize;

    int size                   = m_elementSize * m_numElements;
    m_maxSize                  = size;

    debugAssertM(
                 (sourcePtr == NULL) ||
                 (m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");

    m_generation = m_area->currentGeneration();

    m_pointer = (uint8*)m_area->openGLBasePointer() + m_area->allocatedSize();

    // Align to the nearest multiple of this many bytes.
    const int alignment = 4;

    // Ensure that the next memory address is aligned.  This has 
    // a significant (up to 25%!) performance impact on some GPUs
    int pointerOffset = ((alignment - (int)m_pointer % alignment) % alignment);

    if (numElements == 0) {
        pointerOffset = 0;
    }

    // Adjust pointer to new alignment
    m_pointer = (uint8*)m_pointer + pointerOffset;
    
    int newAlignedSize = size + pointerOffset;

    alwaysAssertM(newAlignedSize <= m_area->freeSize(),
                  "VertexBuffer too small to hold new VertexRange (possibly due to rounding"
                  " to the nearest dword boundary).");

    // Upload the data
    if (size > 0 && sourcePtr != NULL) {
        // Update VertexBuffer values
        m_area->updateAllocation(newAlignedSize);

        uploadToCard(sourcePtr, 0, size);
    }

    debugAssertGLOk();
}


void VertexRange::update(
    const void*         sourcePtr,
    int                 numElements,
    GLenum              glformat,
    int                 eltSize) {

    int size = eltSize * numElements;

    debugAssert(m_stride == 0 || m_stride == m_elementSize);
    alwaysAssertM(size <= m_maxSize,
        "A VertexRange can only be updated with an array that is smaller "
        "or equal size (in bytes) to the original array.");

    alwaysAssertM(m_generation == m_area->currentGeneration(),
        "The VertexBuffer has been reset since this VertexRange was created.");

    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;

    debugAssertM(
                 (m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");
	
    // Upload the data
    if (size > 0) {
        uploadToCard(sourcePtr, 0, size);
    }
    debugAssertGLOk();
}


void VertexRange::set(int index, const void* value, GLenum glformat, int eltSize) {
    debugAssert(m_stride == 0 || m_stride == m_elementSize);
    (void)glformat;
    debugAssertM(index < m_numElements && index >= 0, 
        "Cannot call VertexRange::set with out of bounds index");
    
    debugAssertM(glformat == m_underlyingRepresentation, 
        "Value argument to VertexRange::set must match the intialization type.");

    debugAssertM(eltSize == m_elementSize, 
        "Value argument to VertexRange::set must match the intialization type's memory footprint.");

    uploadToCard(value, index * eltSize, eltSize);
}


void* VertexRange::mapBuffer(GLenum permissions) {
    // Map buffer
    switch (m_area->m_mode) {
    case VertexBuffer::VBO_MEMORY:
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        glBindBufferARB(m_area->openGLTarget(), m_area->m_glbuffer);
        return (uint8*)glMapBufferARB(m_area->openGLTarget(), permissions) + 
            (int)m_pointer;
        break;

    case VertexBuffer::MAIN_MEMORY:
        return (uint8*)m_pointer;
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
        return NULL;
    }
}


void VertexRange::unmapBuffer() {
    switch (m_area->m_mode) {
    case VertexBuffer::VBO_MEMORY:
        glUnmapBufferARB(m_area->openGLTarget());
        glBindBufferARB(m_area->openGLTarget(), GL_NONE);
        glPopClientAttrib();
        break;

    case VertexBuffer::MAIN_MEMORY:
        // Empty
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
    }
    debugAssertGLOk();
}


void VertexRange::uploadToCardStride(const void* srcPointer, int srcElements, int srcSize, int srcStride, 
                        int dstPtrOffsetBytes, int dstStrideBytes) {
    
    if (srcStride == 0) {
        srcStride = srcSize;
    }

    if (dstStrideBytes == 0) {
        dstStrideBytes = srcSize;
    }

    uint8* dstPointer = (uint8*)mapBuffer(GL_WRITE_ONLY) + (int)dstPtrOffsetBytes;

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


void VertexRange::uploadToCard(const void* sourcePtr, int dstPtrOffset, int size) {
    debugAssertGLOk();
    debugAssert(m_stride == 0 || m_stride == m_elementSize);

    void* ptr = (void*)(reinterpret_cast<intptr_t>(m_pointer) + dstPtrOffset);

    switch (m_area->m_mode) {
    case VertexBuffer::VBO_MEMORY:
        // Don't destroy any existing bindings; this call can
        // be made at any time and the program might also
        // use VBO on its own.
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            glBindBufferARB(m_area->openGLTarget(), m_area->m_glbuffer);
            glBufferSubDataARB(m_area->openGLTarget(), (GLintptrARB)ptr, size, sourcePtr);
            glBindBufferARB(m_area->openGLTarget(), 0);
        }
        glPopClientAttrib();
        break;

    case VertexBuffer::MAIN_MEMORY:
        System::memcpy(ptr, sourcePtr, size);
        break;

    default:
        alwaysAssertM(false, "Fell through switch");
    }
    debugAssertGLOk();
}


// The following are called by the VARSystem.
void VertexRange::vertexPointer() const {
    debugAssert(valid());
    glEnableClientState(GL_VERTEX_ARRAY);
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_INT, 
                 "OpenGL does not support GL_UNSIGNED_INT as a vertex format.");
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_SHORT, 
                 "OpenGL does not support GL_UNSIGNED_SHORT as a vertex format.");
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_BYTE, 
                 "OpenGL does not support GL_UNSIGNED_BYTE as a vertex format.");
    alwaysAssertM(sizeOfGLFormat(m_underlyingRepresentation) > 0, "Unsupported vertex format");
    glVertexPointer(m_elementSize / sizeOfGLFormat(m_underlyingRepresentation), 
                    m_underlyingRepresentation, m_stride, m_pointer);
}


void VertexRange::normalPointer() const {
    debugAssert(valid());
    alwaysAssertM(sizeOfGLFormat(m_underlyingRepresentation) > 0, "Unsupported vertex format");
    debugAssert((m_numElements == 0) || (double)m_elementSize / sizeOfGLFormat(m_underlyingRepresentation) == 3.0);
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_INT, 
                 "OpenGL does not support GL_UNSIGNED_INT as a normal format.");
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_SHORT, 
              "OpenGL does not support GL_UNSIGNED_SHORT as a normal format.");
    debugAssertM(m_underlyingRepresentation != GL_UNSIGNED_BYTE,
              "OpenGL does not support GL_UNSIGNED_BYTE as a normal format.");
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(m_underlyingRepresentation, m_stride, m_pointer); 
}


void VertexRange::colorPointer() const {
    debugAssert(valid());
    glEnableClientState(GL_COLOR_ARRAY);
    alwaysAssertM(sizeOfGLFormat(m_underlyingRepresentation) > 0, "Unsupported vertex format");
    glColorPointer(m_elementSize / sizeOfGLFormat(m_underlyingRepresentation),
                   m_underlyingRepresentation, m_stride, m_pointer); 
}


void VertexRange::texCoordPointer(uint32 unit) const {
    debugAssert(valid());
    debugAssertM(GLCaps::supports_GL_ARB_multitexture() || (unit == 0),
        "Graphics card does not support multitexture");

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    alwaysAssertM(sizeOfGLFormat(m_underlyingRepresentation) > 0, "Unsupported vertex format");
    glTexCoordPointer(m_elementSize / sizeOfGLFormat(m_underlyingRepresentation),
                      m_underlyingRepresentation, m_stride, m_pointer);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
    }
}


void VertexRange::vertexAttribPointer(uint32 attribNum, bool normalize) const {
    debugAssert(valid());
    if (GLCaps::supports_GL_ARB_vertex_program()) {
        glEnableVertexAttribArrayARB(attribNum);
        glVertexAttribPointerARB(attribNum, m_elementSize / sizeOfGLFormat(m_underlyingRepresentation),
                                 m_underlyingRepresentation, normalize, m_stride, m_pointer);
    }
}

}

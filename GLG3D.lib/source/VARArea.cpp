/**
 @file VertexBuffer.cpp
 
 Implementation of the vertex array system used by RenderDevice.

 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2003-01-08
 @edited  2008-12-24
 */

#include "G3D/Log.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/VAR.h"
#include "GLG3D/VARArea.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

Array<VertexBufferRef>    VertexBuffer::m_allVARAreas;

VertexBuffer::Mode VertexBuffer::m_mode = VertexBuffer::UNINITIALIZED;

int VertexBuffer::m_sizeOfAllVARAreasInMemory = 0;

VertexBufferRef VertexBuffer::create(int s, UsageHint h, Type t) {
    cleanCache();
    VertexBufferRef x = new VertexBuffer(s, h, t);
    m_allVARAreas.push(x);
    return x;
}


VertexBuffer::VertexBuffer(int _size, UsageHint hint, Type t) :  m_type(t), m_size(_size) {
    m_renderDevice = NULL;
    debugAssertGLOk();

    // See if we've determined the m_mode yet.
    if (m_mode == UNINITIALIZED) {
        if (GLCaps::supports_GL_ARB_vertex_buffer_object() &&
            (glGenBuffersARB != NULL) && 
            (glBufferDataARB != NULL) &&
            (glDeleteBuffersARB != NULL) &&			
            ! GLCaps::hasBug_slowVBO()) {
            m_mode = VBO_MEMORY;
        } else {
            m_mode = MAIN_MEMORY;
        }
    }

    m_sizeOfAllVARAreasInMemory += m_size;

    switch (m_mode) {
    case VBO_MEMORY:
        {
            //glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            glGenBuffersARB(1, &m_glbuffer);
            glBindBufferARB(openGLTarget(), m_glbuffer);
            
            GLenum usage;
            
            switch (hint) {
            case WRITE_EVERY_FRAME:
                usage = GL_STREAM_DRAW_ARB;
                break;
                
            case WRITE_ONCE:
                usage = GL_STATIC_DRAW_ARB;
                break;
                
            case WRITE_EVERY_FEW_FRAMES:
                usage = GL_DYNAMIC_DRAW_ARB;
                break;
                
            default:
                usage = GL_STREAM_DRAW_ARB;
                debugAssertM(false, "Fell through switch");
            }
            
            // Load some (undefined) data to initialize the buffer
            glBufferDataARB(openGLTarget(), m_size, NULL, usage);
            debugAssertGLOk();    
            
            // The m_basePointer is always NULL for a VBO
            m_basePointer = NULL;
            
            glBindBufferARB(openGLTarget(), 0);
            //glPopClientAttrib();
            debugAssertGLOk();
        }
        break;

    case MAIN_MEMORY:
        // Use the base pointer
        m_glbuffer = 0;
        m_basePointer = malloc(m_size);
        debugAssert(m_basePointer);
        break;

    default:
        alwaysAssertM(false, "Fell through switch.");
        m_glbuffer = 0;
        m_basePointer = NULL;
    }

    milestone     = NULL;
    m_allocated     = 0;
    m_generation    = 1;
    m_peakAllocated = 0;
}


VertexBuffer::~VertexBuffer() {
    m_sizeOfAllVARAreasInMemory -= m_size;

    if (m_size == 0) {
        // Already freed
        return;
    }

    switch (m_mode) {
    case VBO_MEMORY:
        // Delete the vertex buffer
        glDeleteBuffersARB(1, &m_glbuffer);
        m_glbuffer = 0;
        break;

    case MAIN_MEMORY:
        // Free the buffer
        free(m_basePointer);
        m_basePointer = NULL;
        break;

    default:
        alwaysAssertM(false, "Fell through switch.");
    }

    m_size = 0;
}


void VertexBuffer::finish() {
    if (milestone.notNull()) {
        m_renderDevice->waitForMilestone(milestone);
        milestone = NULL;
    }
}


void VertexBuffer::reset() {
    finish();
    ++m_generation;
    m_allocated = 0;
}


void VertexBuffer::cleanCache() {
    int i = 0;
    while (i < m_allVARAreas.size()) {
        if (m_allVARAreas[i].isLastReference()) {
            m_allVARAreas.fastRemove(i);
        } else {
            ++i;
        }
    }
}


void VertexBuffer::cleanupAllVARAreas() {
    // Intentionally empty
    for (int i = 0; i < m_allVARAreas.size(); ++i) {
        debugAssert((void*)m_allVARAreas[i]->m_renderDevice != (void*)0xcdcdcdcd);

        m_allVARAreas[i]->reset();

        // Invoke the destructor, freeing the resources even if there are
        // more pointers (but not actually deleting the object)
        m_allVARAreas[i]->~VertexBuffer();
    }
    m_allVARAreas.clear();
}

} // namespace

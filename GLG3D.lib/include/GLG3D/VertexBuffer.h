/**
  @file VertexBuffer.h
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2003-08-09
  @edited  2008-12-06
*/

#ifndef GLG3D_VARArea_h
#define GLG3D_VARArea_h

#include "G3D/ReferenceCount.h"
#include "G3D/Table.h"
#include "GLG3D/Milestone.h"
#include "GLG3D/glheaders.h"

namespace G3D {

// Forward-declaration because including RenderDevice.h would include VertexBuffer.h also
class RenderDevice;

/** @deprecated */
typedef ReferenceCountedPointer<class VertexBuffer> VertexBufferRef;

/**
 @brief A block of GPU memory within which G3D::VertexRanges can be allocated.

 Wrapper for OpenGL Vertex Buffer Object
 http://oss.sgi.com/projects/ogl-sample/registry/ARB/vertex_buffer_object.txt
 http://developer.nvidia.com/docs/IO/8230/GDC2003_OGL_BufferObjects.ppt

 Allocate a VertexBuffer, then allocate VertexRanges within it.  VertexBuffer are garbage
 collected: when no pointers remain to VertexRanges inside it or the VertexBuffer itself,
 it will automatically be reclaimed by the system.

 You cannot mix pointers from different VertexBuffers when rendering.  For
 example, if the vertex VertexRange is in one VertexBuffer, the normal VertexRange and color
 VertexRange must come from the same area.

 You can't find out how much space is left for VARAreas in video memory,
 except by checking the VertexBuffer::create value and seeing if it is NULL.
 */
class VertexBuffer : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<class VertexBuffer> Ref;


    /**
     These values are <B>hints</B>. Your program will work correctly
     regardless of which you use, but using the appropriate value
     lets the renderer optimize for your useage patterns and can
     increase performance.

     Use WRITE_EVERY_FRAME if you write <I>at least</I> once per frame
     (e.g. software animation).

     Use WRITE_EVERY_FEW_FRAMES if you write to the area as part of
     the rendering loop, but not every frame (e.g. impostors, deformable
     data).

     Use WRITE_ONCE if you do not write to the area inside the rendering
     loop (e.g. rigid bodies loaded once at the beginning of a game level).  
     This does not mean you can't write multiple times
     to the area, just that writing might be very slow compared to rendering.

     Correspond to OpenGL hints: 
      WRITE_ONCE : GL_STATIC_DRAW_ARB
      WRITE_EVERY_FRAME : GL_STREAM_DRAW_ARB
      WRITE_EVERY_FEW_FRAMEs : DYNAMIC_DRAW_ARB
     */
    enum UsageHint {
        WRITE_ONCE,
        WRITE_EVERY_FEW_FRAMES,
        WRITE_EVERY_FRAME};

    /**
       Each area can old either vertex data or vertex indices.
     */
    enum Type {
        DATA,
        INDEX
    };
    
private:

    friend class VertexRange;
    friend class RenderDevice;

    /**
     The milestone is used for finish().  It is created
     by RenderDevice::setVARAreaMilestones.  If NULL, there
     is no milestone pending.
     */
    MilestoneRef                        milestone;

    /** Number of bytes currently m_allocated out of m_size total. */
    int				                    m_allocated;
    
    Type                                m_type;

    /**
       This count prevents vertex arrays that have been freed from
       accidentally being used-- it is incremented every time
       the VertexBuffer is reset.
    */
    uint64				                m_generation;
    
    /** The maximum m_size of this area that was ever used. */
    int				                    m_peakAllocated;
    
    /** Set by RenderDevice */
    RenderDevice*                       m_renderDevice;
    
    /** Total  number of bytes in this area.  May be zero if resources have been freed.*/
    int				                    m_size;
    
    /**
       The OpenGL buffer object associated with this area
       (only used when m_mode == VBO_MEMORY)
    */
    uint32                              m_glbuffer;
    
    /** Pointer to the memory (NULL when
        the VBO extension is not present). */
    void*				                m_basePointer;
    
    enum Mode {UNINITIALIZED, VBO_MEMORY, MAIN_MEMORY};
    static Mode                         m_mode;
    
    /** Updates allocation and peakAllocation based off of new allocation. */
    inline void updateAllocation(int newAllocation) {
        m_allocated += newAllocation;
        m_peakAllocated = iMax(m_peakAllocated, m_allocated);
    }

    static int                          m_sizeOfAllVARAreasInMemory;

    VertexBuffer(int _size, UsageHint h, Type t);

    static Array<VertexBufferRef>       m_allVARAreas;
    
    /** Removes elements of m_allVARAreas that are not externally referenced.
        Called whenever a new VertexBuffer is created.*/
    static void cleanCache();
    
public:

    /**
       You should always create your VARAreas at least 8 bytes larger
       than needed for each individual VertexRange because VertexBuffer tries to 
       align VertexRange starts in memory with dword boundaries.
     */
    static VertexBufferRef create(int s, UsageHint h = WRITE_EVERY_FRAME, Type = DATA);

    ~VertexBuffer();

    inline Type type() const {
        return m_type;
    }

    inline int totalSize() const {
        return m_size;
    }

    inline int freeSize() const {
        return m_size - m_allocated;
    }

    inline int allocatedSize() const {
        return m_allocated;
    }

    inline int peakAllocatedSize() const {
        return m_peakAllocated;
    }

    inline uint64 currentGeneration() const {
        return m_generation;
    }

    /**
     Provided for breaking the VertexBuffer abstraction; use G3D::VertexRange and 
     G3D::RenderDevice in general.

     When using the OpenGL vertex buffer API, this is the underlying 
     OpenGL vertex buffer object.  It is zero when using system memory.
     The caller cannot control whether VBO is used or not; G3D selects
     the best method automatically.
     */
    inline uint32 openGLVertexBufferObject() const {
        return m_glbuffer;
    }

    /**
     Provided for breaking the VertexBuffer abstraction; use G3D::VertexRange and 
     G3D::RenderDevice in general.

     When using system memory, this is a pointer to the beginning of 
     the system memory block in which data is stored.  Null when using VBO.
     */
    inline void* openGLBasePointer() const {
        return m_basePointer;
    }

    /**
     Blocks the CPU until all rendering calls referencing 
     this area have completed.
     */
    void finish();

    /** Finishes, then frees all VertexRange memory inside this area.*/ 
    void reset();

    /** Returns the total m_size of all VARAreas m_allocated.  Note that not all
        will be in video memory, and some will be backed by main memory 
        even if nominally stored in video memory, so the total m_size may
        exceed the video memory m_size.*/
    static int sizeOfAllVARAreasInMemory() {
        return m_sizeOfAllVARAreasInMemory;
    }

    /** Releases all VARAreas. Called before shutdown by RenderDevice. */
    static void cleanupAllVARAreas();

    /** Target argument for glBufferData and other raw OpenGL routines. */
    inline GLenum openGLTarget() const {
        if (m_type == DATA) {
            return GL_ARRAY_BUFFER_ARB;
        } else {
            return GL_ELEMENT_ARRAY_BUFFER;
        }
    }

};


/** @deprecated Use VertexBuffer */
typedef VertexBuffer VARArea;

} // namespace G3D

template <> struct HashTrait<G3D::VertexBuffer*> {
    static size_t hashCode(const G3D::VertexBuffer* key) { return reinterpret_cast<size_t>(key); }
};

#endif

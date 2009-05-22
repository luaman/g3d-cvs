/**
  @file GLG3D/VAR.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2001-05-29
  @edited  2009-02-03
*/

#ifndef GLG3D_VAR_h
#define GLG3D_VAR_h

#include "GLG3D/getOpenGLState.h"
#include "GLG3D/glFormat.h"
#include "GLG3D/VARArea.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;


/**
 @brief A pointer to a "Vertex ARrray" of data (e.g., vertices,
 colors, or normals) in video memory.

 A VAR is just a pointer, so it is safe to copy these (the pointer
 will be copied, not the video memory).
 
 There is no destructor because the referenced memory is freed when
 the parent VARArea is reset or freed.

 A VAR is normally a statically typed fixed-length array of a Vector
 or Color class, however it is possible to make a "void" array with the 
 constructor that does not take an array,
 and then fill it with data to create interleaved or structure arrays.
 Interleaved arrays are 2x - 3x as fast as non-interleaved ones for
 vertex-limited programs.
 */
class VAR {
private:

    friend class RenderDevice;

    VARAreaRef           m_area;
    
    /** For VBO_MEMORY, this is the offset.  For MAIN_MEMORY, this is
        a pointer to the block of uploaded memory.
        
        When there was a dstOffset as an init() argument, it has already
        been applied here.
    */
    void*               _pointer;
    
    /** Size of one element. For a void array, this is 1.*/
    size_t              elementSize;
    
    /** For a void array, this is _maxSize. */
    int                 numElements;
   
    /** Space between subsequent elements, must be zero or >= elementSize */
    size_t              m_stride;
 
    uint64		generation;
    
    /** GL_NONE for a "void" array */
    GLenum              underlyingRepresentation;
    
    /** The initial size this VAR was allocated with, in bytes. */
    size_t              _maxSize;

    /** For uploading interleaved arrays */
    void init(VAR& dstPtr, size_t dstOffset, GLenum glformat, 
        size_t eltSize, int _numElements, size_t stride);

    void init(const void* sourcePtr, 
              int _numElements, 
              VARAreaRef _area,
              GLenum glformat, 
              size_t eltSize);

    void init(const void* srcPtr,
              int     _numElements, 
              int     srcStride,      
              GLenum  glformat, 
              size_t  eltSize,
              VAR     dstPtr,
              size_t  dstOffset, 
              size_t  dstStride);
    
    void update(const void* sourcePtr, int _numElements,
                GLenum glformat, size_t eltSize);

    /** Performs the actual memory transfer (like memcpy).  The
        dstPtrOffset is the number of <B>bytes</B> to add to _pointer
        when performing the transfer. */
    void uploadToCard(const void* sourcePtr, int dstPtrOffsetElements, size_t size);

    /** Used for creating interleaved arrays. */
    void uploadToCardStride(const void* sourcePtr, int srcElements, size_t srcSizeBytes,
                            int srcStrideBytes, size_t dstPtrOffsetBytes, size_t dstStrideBytes);

    void set(int index, const void* value, GLenum glformat, size_t eltSize);

    // The following are called by RenderDevice
    /** May be an OpenGL video memory offset or a real memory pointer.  For use by RenderDevice only.*/
    const void* pointer() const {
        return _pointer;
    }

    void vertexPointer() const;
    
    void normalPointer() const;
    
    void colorPointer() const;
    
    void texCoordPointer(unsigned int unit) const;
    
    void vertexAttribPointer(unsigned int attribNum, bool normalize) const;

public:

    /** The G3D::VARArea containing this VAR. */
    inline VARAreaRef area() {
        return m_area;
    }

    /** The G3D::VARArea containing this VAR. */
    inline VARAreaRef area() const {
        return m_area;
    }

    inline VARArea::Type type() const {
        return m_area->type();
    }

    /** @brief Number of elements in this array (not byte size!) */
    inline int size() const {
        return numElements;
    }

    /** Creates an invalid VAR. */
    VAR();

    /** @brief Creates a VAR that acts as a pointer to a block of memory.

        This block of memory can then be used with VAR::VAR(const T* srcPtr, 
        int      _numElements,
        int      srcStride,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride) or        
        VAR::VAR(int      _numElements,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride)
        to upload interleaved data.        
     */
    VAR(size_t numBytes, VARAreaRef _area);
    
    /**
       Uploads memory from the CPU to the GPU.  The element type is
       inferred from the pointer type by the preprocessor.  Sample
       usage:       
       
       <PRE>
       // Once at the beginning of the program
       VARAreaRef dataArea  = VARArea::create(5 * 1024 * 1024);
       VARAreaRef indexArea = VARArea::create(1024 * 1024, VARArea::WRITE_EVERY_FRAME, VARArea::INDEX);
       
       //----------
       // Store data in main memory
       Array<Vector3> vertexArrayCPU;
       Array<int>     indexArrayCPU;
       
       //... fill out vertex & index arrays
       
       //------------
       // Upload to graphics card whenever CPU data changes
       area.reset();
       VAR vertexVARGPU(vertexArrayCPU, dataArea);
       VAR indexVARGPU(indexArrayCPU, indexArea);

       //------------
       // Render 
       renderDevice->beginIndexedPrimitives();
       {
           renderDevice->setVertexArray(varray);
           renderDevice->sendIndices(PrimitiveType::TRIANGLES, indexVARGPU);
       }
       renderDevice->endIndexedPrimitives();
       </PRE>
    */
    template<class T>
    VAR(const T* sourcePtr, int _numElements, VARAreaRef _area) {
        alwaysAssertM((_area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        init(sourcePtr, _numElements, _area, glFormatOf(T), sizeof(T));
    }		

  
    template<class T>
    VAR(const Array<T>& source, VARAreaRef _area) {

        alwaysAssertM((_area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        init(source.getCArray(), source.size(), _area, glFormatOf(T), sizeof(T));
    }


    /** Return a pointer to CPU-addressable memory for this VAR.  The buffer must be unmapped
        later before any rendering calls are made.  This contains a glPushClientAttrib call
        that must be matched by unmapBuffer.

        Works for both CPU memory and VBO memory VAR.

        This method of moving data is not typesafe and is not recommended.
        
        @param permissions Same as the argument to 
        <a href="http://www.opengl.org/sdk/docs/man/xhtml/glMapBuffer.xml">glMapBufferARB</a>:
        <code>GL_READ_ONLY</code>, <code>GL_WRITE_ONLY</code>, or <code>GL_READ_WRITE</code>.                
        */
    void* mapBuffer(GLenum permissions);

    /** Release CPU addressable memory previously returned by mapBuffer.
        This method of moving data is not typesafe and is not recommended. 
      */
    void unmapBuffer();

    /** @brief Update a set of interleaved arrays.  
    
        Update a set of interleaved arrays.  None may change size from the original. */
    template<class T1, class T2, class T3, class T4>
    static void updateInterleaved
        (const Array<T1>& src1,
         VAR&             var1,
         const Array<T2>& src2,
         VAR&             var2,
         const Array<T3>& src3,
         VAR&             var3,
         const Array<T4>& src4,
         VAR&             var4) {

        int N = iMax(iMax(src1.size(), src2.size()),
                     iMax(src3.size(), src4.size()));

        // Pack arguments into arrays to avoid repeated code below.
        uint8* src[4]   = {(uint8*)src1.getCArray(), (uint8*)src2.getCArray(),
                           (uint8*)src3.getCArray(), (uint8*)src4.getCArray()};
        int    count[4] = {src1.size(), src2.size(), src3.size(), src4.size()};
        size_t size[4]  = {sizeof(T1), sizeof(T2), sizeof(T3), sizeof(T4)};
        VAR*   var[4]   = {&var1, &var2, &var3, &var4};
        (void)var;

        // Zero out the size of unused arrays
        for (int a = 0; a < 4; ++a) {
            if (count[a] == 0) {
                // If an array is unused, it occupies no space in the interleaved array.
                size[a] = 0;
            }

            debugAssertM(count[a] == var[a]->numElements, 
                "Updated arrays must have the same size they were created with.");
            if (a > 1) {
                debugAssertM(var[a]->_pointer == (uint8*)var[a - 1]->_pointer + size[a - 1],
                             "Updated interleaved arrays must be the same set and"
                             " order as original interleaved arrays.");
            }
        }

        uint8* dstPtr = (uint8*)var1.mapBuffer(GL_WRITE_ONLY);

        for (int i = 0; i < N; ++i) {
            for (int a = 0; a < 4; ++a) {
                if (count[a] > 0) {
                    System::memcpy(dstPtr, src[a] + size[a] * i, size[a]);
                    dstPtr += size[a];
                }
            }
        }

        var1.unmapBuffer();
    }

    /** @brief Creates four interleaved VAR arrays simultaneously. 

        Creates four interleaved VAR arrays simultaneously.  This is
        convenient for uploading vertex, normal, texcoord, and tangent
        arrays although it can be used for any four arrays.  This is
        substantially faster than creating a single "void VAR" and
        uploading arrays within it using a stride.

        The var<i>n</i> arguments are outputs only; they should not be
        initialized values.

        All src arrays must have the same length or be empty.  Empty arrays
        will return an uninitialized var.

        @sa updateInterleaved
    */
    template<class T1, class T2, class T3, class T4>
    static void createInterleaved(
                       const Array<T1>& src1,
                       VAR&             var1,
                       const Array<T2>& src2,
                       VAR&             var2,
                       const Array<T3>& src3,
                       VAR&             var3,
                       const Array<T4>& src4,
                       VAR&             var4,
                       VARAreaRef       area) {

        int N = iMax(iMax(src1.size(), src2.size()),
                     iMax(src3.size(), src4.size()));

        debugAssert(area->type() == VARArea::DATA);
        debugAssert(src1.size() == N || src1.size() == 0);
        debugAssert(src2.size() == N || src2.size() == 0);
        debugAssert(src3.size() == N || src3.size() == 0);
        debugAssert(src4.size() == N || src4.size() == 0);

        // Treat sizes as zero if the corresponding array is not used
        size_t size1 = (src1.size() == N) ? sizeof(T1) : 0;
        size_t size2 = (src2.size() == N) ? sizeof(T2) : 0;
        size_t size3 = (src3.size() == N) ? sizeof(T3) : 0;
        size_t size4 = (src4.size() == N) ? sizeof(T4) : 0;

        size_t stride = size1 + size2 + size3 + size4;
        size_t totalMemory = stride * N;
        
        VAR masterVAR(totalMemory, area);
        var1.init(masterVAR, 0, glFormatOf(T1), size1, src1.size(), stride);
        var2.init(masterVAR, size1, glFormatOf(T2), size2, src2.size(), stride);
        var3.init(masterVAR, size1 + size2, glFormatOf(T3), size3, src3.size(), stride);
        var4.init(masterVAR, size1 + size2 + size3, glFormatOf(T4), size4, src4.size(), stride);

        updateInterleaved(src1, var1, src2, var2, src3, var3, src4, var4);
    }


    /**
       @brief Create an interleaved array within an existing VAR
       and upload data to it.
              
       Upload @a _numElements values from @a sourcePtr on the 
       CPU to @a dstPtr on the GPU.

       @param srcStride If non-zero, this is the spacing between
       sequential elements <i>in bytes</i>. It may be negative. 

       @param dstOffset Offset in bytes from the head of dstPtr.
       e.g., to upload starting after the 2nd byte, set
       <code>dstOffset = 2</code>.

       @param dstStride If non-zero, this is the spacing between
       sequential elements of T in dstPtr.  e.g., to upload every
       other Vector3, use <code>dstStride = sizeof(Vector3) *
       2</code>.  May not be negative.
     */
    template<class T>
    VAR(const T* srcPtr, 
        int      _numElements,
        int      srcStride,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride) {
        init(srcPtr, _numElements, srcStride, glFormatOf(T), sizeof(T), dstPtr, dstOffset, dstStride);
    }

    /** @brief Create an interleaved array within an existing VAR, but do
        not upload data to it.

        Data can later be uploaded by update() or mapBuffer().

        Example:
        <PRE>
           G3D_BEGIN_PACKED_CLASS
           struct Packed {
               Vector3   vertex;
               Vector2   texcoord;
           }
           G3D_END_PACKED_CLASS

           ...
            
           size_t stride = sizeof(Vector3) + sizeof(Vector2);
           size_t totalSize = stride * N;

           VAR interleavedBlock(totalSize, area);

           VAR vertex(Vector3() N, interleavedBlock, 0, stride);
           VAR texcoord(Vector2(), N, interleavedBlock, sizeof(Vector3), stride);

           Packed* ptr = (Packed*)interleavedBlock.mapBuffer(GL_WRITE_ONLY);
            // ... write to elements of ptr ...
           interleavedBlock.unmapBuffer();
        </PRE>

       @param dstStride If non-zero, this is the spacing between
       sequential elements of T in dstPtr.  e.g., to upload every
       other Vector3, use <code>dstStride = sizeof(Vector3) *
       2</code>.  May not be negative.
       */
    template<class T>
    VAR(const T& ignored,
        int      _numElements,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride) {
        (void)ignored;
        init(dstPtr, dstOffset, glFormatOf(T), sizeof(T), _numElements, dstStride);
    }

    template<class T>
    VAR(const Array<T>& source,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride) {
        init(source.getCArray(), source.size(), 0, glFormatOf(T), sizeof(T), dstPtr, dstOffset, dstStride);
    }

    
    template<class T>
    void update(const T* sourcePtr, int _numElements) {
        debugAssertM((m_area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        update(sourcePtr, _numElements, glFormatOf(T), sizeof(T));
    }
   
    /**
       Overwrites existing data with data of the same size or smaller.
       Convenient for changing part of a G3D::VARArea without reseting
       the area (and thereby deallocating the other G3D::VAR arrays in
       it).
    */
    template<class T>
    void update(const Array<T>& source) {
        debugAssertM((m_area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        update(source.getCArray(), source.size(), glFormatOf(T), sizeof(T));
    }
    
    /** Overwrites a single element of an existing array without
       changing the number of elements.  This is faster than calling
       update for large arrays, but slow if many set calls are made.
       Typically used to change a few key vertices, e.g., the single
       dark cap point of a directional light's shadow volume.
     */
    // This is intentionally not the overloaded [] operator because
    // direct access to VAR memory is generally slow and discouraged.
    template<class T>
    void set(int index, const T& value) {
        debugAssertM((m_area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        set(index, &value, glFormatOf(T), sizeof(T));
    }
    
    /** @brief Returns true if this VAR can be used for rendering
        (i.e., contains data and the parent VARArea has not been reset).  */
    bool valid() const;

    /** @brief Maximum size that can be loaded via update into this VAR. */
    inline size_t maxSize() const {
        if (valid()) {
            return _maxSize;
        } else {
            return 0;
        }
    }
};

}

#endif

/**
  @file GLG3D/VAR.h

  @maintainer Morgan McGuire, morgan@graphics3d.com
  @created 2001-05-29
  @edited  2008-12-24
*/

#ifndef GLG3D_VAR_H
#define GLG3D_VAR_H

#include "GLG3D/RenderDevice.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/glFormat.h"
#include "GLG3D/VARArea.h"

namespace G3D {

/**
 @brief A pointer to a "Vertex ARrray" of data (e.g., vertices,
 colors, or normals) in video memory.

 A VAR is just a pointer, so it is safe to copy these (the pointer
 will be copied, not the video memory).
 
 There is no destructor because the referenced memory is freed when
 the parent VARArea is reset or freed.

 A VAR is normally a statically typed fixed-length array of a Vector
 or Color class, however it is possible to make a "void" byte array
 and then fill it with data to create interleaved or structure arrays.
 */
class VAR {
private:

    friend class RenderDevice;

    VARAreaRef	                        area;
    
    /** For VBO_MEMORY, this is the offset.  For MAIN_MEMORY, this is
        a pointer to the block of uploaded memory.
        
        When there was a dstOffset as an init() argument, it has already
        been applied here.
    */
    void*				_pointer;
    
    /** Size of one element. For a void array, this is 1.*/
    size_t				elementSize;
    
    /** For a void array, this is _maxSize. */
    int					numElements;
   
    /** Space between subsequent elements, must be zero or >= elementSize */
    size_t                              m_stride;
 
    uint64				generation;
    
    /** GL_NONE for a "void" array */
    GLenum                              underlyingRepresentation;
    
    /** The initial size this VAR was allocated with, in bytes. */
    size_t                              _maxSize;

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
    void vertexPointer() const;
    
    void normalPointer() const;
    
    void colorPointer() const;
    
    void texCoordPointer(unsigned int unit) const;
    
    void vertexAttribPointer(unsigned int attribNum, bool normalize) const;

public:

    /** Creates an invalid VAR */
    VAR();

    /** 
        @brief Creates a VAR that acts as a pointer to a block of memory.

        Creates a VAR that acts as a pointer to a block of memory.
        Interleaved data can then be loaded into this using another 
        VAR constructor.
     */
    VAR(size_t numBytes, VARAreaRef _area);
    
    /**
       Uploads memory from the CPU to the GPU.  The element type is
       inferred from the pointer type by the preprocessor.  Sample
       usage:       
       
       <PRE>
       // Once at the beginning of the program
       VARAreaRef area = VARArea::create(1024 * 1024);
       
       //----------
       // Store data in main memory
       Array<Vector3> vertex;
       Array<int>     index;
       
       //... fill out vertex & index arrays
       
       //------------
       // Upload to graphics card every frame
       area.reset();
       VAR varray(vertex, area);
       renderDevice->beginIndexedPrimitives();
       renderDevice->setVertexArray(varray);
       renderDevice->sendIndices(RenderDevice::TRIANGLES, index);
       renderDevice->endIndexedPrimitives();
       </PRE>
       See GLG3D_Demo for examples.
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
    */
    template<class T1, class T2, class T3, class T4>
    static void create(
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

        debugAssert(src1.size() == N || src1.size() == 0);
        debugAssert(src2.size() == N || src2.size() == 0);
        debugAssert(src3.size() == N || src3.size() == 0);
        debugAssert(src4.size() == N || src4.size() == 0);

        // TODO
    }

    /**
       Upload @a _numElements values from @a sourcePtr on the CPU to @a dstPtr on the GPU.

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

    template<class T>
    VAR(const Array<T>& source,
        VAR      dstPtr,
        size_t   dstOffset, 
        size_t   dstStride) {
        init(source.getCArray(), source.size(), 0, glFormatOf(T), sizeof(T), dstPtr, dstOffset, dstStride);
    }

    
    template<class T>
    void update(const T* sourcePtr, int _numElements) {
        debugAssertM((area->type() == VARArea::DATA) || isIntType(T),
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
        debugAssertM((area->type() == VARArea::DATA) || isIntType(T),
                      "Cannot create an index VAR in a non-index VARArea");
        update(source.getCArray(), source.size(), glFormatOf(T), sizeof(T));
    }
    
    /**
       Overwrites a single element of an existing array without
       changing the number of elements.  This is faster than calling
       update for large arrays, but slow if many set calls are made.
       Typically used to change a few key vertices, e.g., the single
       dark cap point of a directional light's shadow volume.
    */
    // This is intentionally not the overloaded [] operator because
    // direct access to VAR memory is generally slow and discouraged.
    template<class T>
    void set(int index, const T& value) {
        debugAssertM((area->type() == VARArea::DATA) || isIntType(T),
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

/**
 @file Map2D.h

 More flexible support than provided by G3D::GImage.

 @maintainer Morgan McGuire, morgan@cs.brown.edu
 @created 2004-10-10
 @edited  2007-01-31
 */
#ifndef G3D_MAP2D_H
#define G3D_MAP2D_H

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Array.h"
#include "G3D/Vector2int16.h"
#include "G3D/ReferenceCount.h"
#include "G3D/AtomicInt32.h"
#include "G3D/GThread.h"
#include <string>

namespace G3D {
    /**
     WRAP_IGNORE silently discards attempts to write to out 
     of bounds locations and returns an undefined value for reading
     from out of bounds locations.

     WRAP_ERROR generates an error when the 
     pixel indices are out of bounds
     */
    enum WrapMode {WRAP_CLAMP, WRAP_TILE, WRAP_IGNORE, WRAP_ERROR};

}


namespace G3D {
namespace _internal {

/** The default compute type for a type is the type itself. */
template<typename Storage> class _GetComputeType {
public:
    typedef Storage Type;
};

} // _internal
} // G3D

// This weird syntax is needed to support VC6, which doesn't
// properly implement template overloading.
#define DECLARE_COMPUTE_TYPE(StorageType, ComputeType)            \
namespace G3D {                                                   \
    namespace _internal {                                         \
        template<> class _GetComputeType < StorageType > {        \
        public:                                                   \
            typedef ComputeType Type;                             \
        };                                                        \
    }                                                             \
}

DECLARE_COMPUTE_TYPE( float32,  float64)
DECLARE_COMPUTE_TYPE( float64,  float64)

DECLARE_COMPUTE_TYPE( int8,     float32)
DECLARE_COMPUTE_TYPE( int16,    float32)
DECLARE_COMPUTE_TYPE( int32,    float64)
DECLARE_COMPUTE_TYPE( int64,    float64)

DECLARE_COMPUTE_TYPE( uint8,     float32)
DECLARE_COMPUTE_TYPE( uint16,    float32)
DECLARE_COMPUTE_TYPE( uint32,    float64)
DECLARE_COMPUTE_TYPE( uint64,    float64)

DECLARE_COMPUTE_TYPE( Vector2,  Vector2)
DECLARE_COMPUTE_TYPE( Vector2int16,  Vector2)

DECLARE_COMPUTE_TYPE( Vector3,  Vector3)
DECLARE_COMPUTE_TYPE( Vector3int16,  Vector3)

DECLARE_COMPUTE_TYPE( Vector4,  Vector4)

DECLARE_COMPUTE_TYPE( Color3,  Color3)
DECLARE_COMPUTE_TYPE( Color3uint8,  Color3)

DECLARE_COMPUTE_TYPE( Color4,  Color4)
DECLARE_COMPUTE_TYPE( Color4uint8,  Color4)
#undef DECLARE_COMPUTE_TYPE

namespace G3D {

/**
  Map of values across a discrete 2D plane.  Can be thought of as a generic class for 2D images, 
  allowing flexibility as to pixel format and convenient methods.
  In fact, the "pixels" can be any values
  on a grid that can be sensibly interpolated--RGB colors, scalars, 4D vectors, and so on.

  Other "image" classes in G3D:

  G3D::GImage - Supports file formats, fast, Color3uint8 and Color4uint8 formats.  No interpolation.

  G3D::TextureRef - Represents image on the graphics card (not directly readable on the CPU).  Supports 2D, 3D, and a variety of interpolation methods, loads file formats.

  G3D::Image3 - A subclass of Map2D<Color3> that supports image loading and saving and conversion to Texture.

  G3D::Image4 - A subclass of Map2D<Color4> that supports image loading and saving and conversion to Texture.

  G3D::Image3uint8 - A subclass of Map2D<Color3uint8> that supports image loading and saving and conversion to Texture.

  G3D::Image4uint8 -  A subclass of Map2D<Color4uint8> that supports image loading and saving and conversion to Texture.

  
  There are two type parameters-- the first (@ Storage) is the type 
  used to store the "pixel" values efficiently and 
  the second (@a Compute) is
  the type operated on by computation.  The Compute::Compute(Storage&) constructor
  is used to convert between storage and computation types.
  @a Storage is often an integer version of @a Compute, for example 
  <code>Map2D<double, uint8></code>.  By default, the computation type is:

  <pre>
     Storage       Computation

     uint8          float32
     uint16         float32
     uint32         float64
     uint64         float64

     int8           float32
     int16          float32
     int32          float64
     int64          float64

     float32        float64
     float64        float64

     Vector2        Vector2
     Vector2int16   Vector2

     Vector3        Vector3
     Vector3int16   Vector3

     Vector4        Vector4

     Color3         Color3
     Color3uint8    Color3

     Color4         Color4
     Color4uint8    Color4
    </pre>
  Any other storage type defaults to itself as the computation type.

  The computation type can be any that 
  supports lerp, +, -, *, /, and an empty constructor.

  Assign value:

    <code>im->set(x, y, 7);</code> or 
    <code>im->get(x, y) = 7;</code>

  Read value:

    <code>int c = im(x, y);</code>

  Can also sample with nearest neighbor, bilinear, and bicubic
  interpolation.  
  
  Sampling follows OpenGL conventions, where 
  pixel values represent grid points and (0.5, 0.5) is half-way
  between two vertical and two horizontal grid points.  
  To draw an image of dimensions w x h with nearest neighbor
  sampling, render pixels from [0, 0] to [w - 1, h - 1].

  Under the WRAP_CLAMP wrap mode, the value of bilinear interpolation
  becomes constant outside [1, w - 2] horizontally.  Nearest neighbor
  interpolation is constant outside [0, w - 1] and bicubic outside
  [3, w - 4].  The class does not offer quadratic interpolation because
  the interpolation filter could not center over a pixel.
  
  @author Morgan McGuire, morgan@cs.williams.edu
 */
template< typename Storage, typename Compute = typename G3D::_internal::_GetComputeType<Storage>::Type > 
class Map2D : public ReferenceCountedObject {

//
// It doesn't make sense to automatically convert from Compute back to Storage
// because the rounding rule (and scaling) is application dependent.
// Thus the interpolation methods all return type Compute.
//

public:

    typedef Storage StorageType;
    typedef Compute ComputeType;
    typedef Map2D<Storage, Compute> Type;
    typedef ReferenceCountedPointer<Map2D> Ref;

protected:

    /** Width, in pixels. */
    uint32              w;

    /** Height, in pixels. */
    uint32              h;

    /** The zero value.  Set by the constructor. */
    Compute             ZERO;

    WrapMode            _wrapMode;

    /** 0 if no mutating method has been invoked 
        since the last call to setChanged(); */
    AtomicInt32         m_changed;

    Array<Storage>      data;

    /** Handles the exceptional cases from get */
    Storage& slowGet(int x, int y) {
        switch (_wrapMode) {
        case WRAP_CLAMP:
            return get(iClamp(x, 0, w - 1), iClamp(y, 0, h - 1));

        case WRAP_TILE:
            return get(iWrap(x, w), iWrap(y, h));

        case WRAP_ERROR:
            alwaysAssertM(((uint32)x < w) && ((uint32)y < h), 
                format("Index out of bounds: (%d, %d), w = %d, h = %d",
                x, y, w, h));

            // intentionally fall through
        case WRAP_IGNORE:
            // intentionally fall through
        default:
            {
                static Storage temp;
                return temp;
            }
        }
    }

    inline const Storage& fastGet(int x, int y) const {
        return data[x + y * w];
    }

    inline void fastSet(int x, int y, const Storage& v) {
        data[x + y * w] = v;
    }

    /** Given four control points and a value on the range [0, 1)
        evaluates the Catmull-rom spline between the times of the
        middle two control points */
    Compute bicubic(const Compute* ctrl, double s) const {

        // f = B * S * ctrl'

        // B matrix: Catmull-Rom spline basis
        static const double B[4][4] = {
            { 0.0, -0.5,  1.0, -0.5},
            { 1.0,  0.0, -2.5,  1.5},
            { 0.0,  0.5,  2.0, -1.5},
            { 0.0,  0.0, -0.5,  0.5}}; 

        // S: Powers of the fraction
        double S[4];
        double s2 = s * s;
        S[0] = 1.0;
        S[1] = s;
        S[2] = s2;
        S[3] = s2 * s;

        Compute sum = ZERO;
        for (int c = 0; c < 4; ++c) {
            double coeff = 0.0;
            for (int power = 0; power < 4; ++power) {
                coeff += B[c][power] * S[power];
            }
            sum += ctrl[c] * coeff;
        }

        return sum;
    }

    Map2D(int w, int h, WrapMode wrap) : _wrapMode(wrap) {
        resize(w, h);

        // Ensure that the zero value is actually zero.
        ZERO = ZERO * 0.0; 
    }

public:

    /**
     Although Map2D is not threadsafe (except for the setChanged() method),
     you can use this mutex to create your own threadsafe access to a Map2D.
     Not used by the default implementation.
    */
    GMutex mutex;

    static Ref create(int w = 0, int h = 0, WrapMode wrap = WRAP_ERROR) {
        return new Map2D(w, h, wrap);
    }

    /** Resizes without clearing, leaving garbage.
      */
    void resize(uint32 newW, uint32 newH) {
        if ((newW != w) || (newH != h)) {
            w = newW;
            h = newH;
            data.resize(w * h);
            setChanged(true);
        }
    }

    /** 
     Returns true if this map has been written to since the last call to setChanged(false).
     This is useful if you are caching a texture map other value that must be recomputed
     whenever this changes.
     */
    bool changed() {
        return m_changed.value() != 0;
    }

    /** Set/unset the changed flag. */
    void setChanged(bool c) {
        m_changed = c ? 1 : 0;
    }

    /** Returns a pointer to the underlying row-major data. There is no padding at the end of the row.
        Be careful--this will be reallocated during a resize.  You should call setChanged(true) if you mutate the array.*/
    Storage* getCArray() {
        return data.getCArray();
    }


    const Storage* getCArray() const {
        return data.getCArray();
    }


    /** Row-major array.  You should call setChanged(true) if you mutate the array. */
    Array<Storage>& getArray() {
        return data;
    }


    const Array<Storage>& getArray() const {
        return data;
    }

    /** Get the value at (x, y).
    
        Note that the type of image->get(x, y) is 
        the storage type, not the computation
        type.  If the constructor promoting Storage to Compute rescales values
        (as, for example Color3(Color3uint8&) does), this will not match the value
        returned by Map2D::nearest.
      */
    inline const Storage& get(int x, int y) const {
        if (((uint32)x < w) && ((uint32)y < h)) {
            return data[x + y * w];
        } else {
            // Remove the const to allow a slowGet on this object
            // (we're returning a const reference so this is ok)
            return const_cast<Type*>(this)->slowGet(x, y);
        }
    }


    void set(int x, int y, const Storage& v) {
        setChanged(true);
        if (((uint32)x < w) && ((uint32)y < h)) {
            // In bounds, wrapping isn't an issue.
            data[x + y * w] = v;
        } else {
            slowGet(x, y) = v;
        }
    }


    void setAll(const Storage& v) {
        for(int i = 0; i < data.size(); ++i) {
            data[i] = v;
        }
        setChanged(true);
    }


    /** Returns the nearest neighbor.  Pixel values are considered
        to be at the upper left corner, so <code>image->nearest(x, y) == image(x, y)</code>
      */
    Compute nearest(double x, double y) const {
        return Compute(get(iRound(x), iRound(y)));
    }


    /** 
      Needs to access elements from (floor(x), floor(y))
      to (floor(x) + 1, floor(y) + 1) and will use
      the wrap mode appropriately (possibly generating 
      out of bounds errors).

      Guaranteed to match nearest(x, y) at integers. */ 
    Compute bilinear(double x, double y) const {
        int i = iFloor(x);
        int j = iFloor(y);
    
        double fX = x - i;
        double fY = y - j;

        // Horizontal interpolation, first row
        Compute t0(get(i, j));
        Compute t1(get(i + 1, j));
        Compute A = lerp(t0, t1, fX);

        // Horizontal interpolation, second row
        Compute t2(get(i, j + 1));
        Compute t3(get(i + 1, j + 1));
        Compute B = lerp(t2, t3, fX);

        // Vertical interpolation
        return lerp(A, B, fY);
    }


    /**
     Uses Catmull-Rom splines to interpolate between grid
     values.  Guaranteed to match nearest(x, y) at integers.
     */
    Compute bicubic(double x, double y) const {
        int i = iFloor(x);
        int j = iFloor(y);
        double fX = x - i;
        double fY = y - j;

        // 'static' prevents constructors from being called
        // every time through this loop.
        static Compute vsample[4];
        for (int v = 0; v < 4; ++v) {

            // Horizontal interpolation
            static Compute hsample[4];
            for (int u = 0; u < 4; ++u) {
                hsample[u] = Compute(get(i + u - 1, j + v - 1));
            }
    
            vsample[v] = bicubic(hsample, fX);
        }

        //  Vertical interpolation
        return bicubic(vsample, fY);
    }


    /** Pixel width */
    inline uint32 width() const {
        return w;
    }


    /** Pixel height */
    inline uint32 height() const {
        return h;
    }


    /** Dimensions in pixels */
    Vector2int16 size() const {
        return Vector2int16(w, h);
    }


    /** Number of bytes occupied by the image data and this structure */
    size_t sizeInMemory() const {
        return data.size() * sizeof(Storage) + sizeof(*this);
    }


    WrapMode wrapMode() const {
        return _wrapMode;
    }


    void setWrapMode(WrapMode m) {
        _wrapMode = m;
    }
};



}

#endif // G3D_IMAGE_H

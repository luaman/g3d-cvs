/**
 @file   Component.h
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2009-02-19
*/
#ifndef G3D_Component_h
#define G3D_Component_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/Image1.h"
#include "G3D/Image3.h"
#include "G3D/Image4.h"
#include "GLG3D/Texture.h"

namespace G3D {

/** Used by Component */
enum ImageStorage {
    /** Ensure that all image data is stored exclusively on the CPU. */
    MOVE_TO_CPU, 

    /** Ensure that all image data is stored exclusively on the GPU. */
    MOVE_TO_GPU, 

    /** Ensure that all image data is stored at least on the CPU. */
    COPY_TO_CPU, 

    /** Ensure that all image data is stored at least on the GPU. */
    COPY_TO_GPU
};


class ImageUtils {
public:
    /** Returns the equivalent 8-bit version of a float format */
    static const ImageFormat* to8(const ImageFormat* f);
};

#define ReferenceCountedPointer ReferenceCountedPointer

/** Manages CPU and GPU versions of image data and performs
    conversions as needed.

    Primarily used by Component. */
template<class Image>
class MapComponent : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<MapComponent<Image> > Ref;

private:
            
#   define MyType class MapComponent<Image>

    ReferenceCountedPointer<Image> m_cpuImage;
    Texture::Ref                   m_gpuImage;
    typename Image::Storage        m_min;
    typename Image::Storage        m_max;
    typename Image::Compute        m_mean;

    static void getTexture(const ReferenceCountedPointer<Image>& im, Texture::Ref& tex) {
            
        Texture::Dimension dim;
        if (isPow2(im->width()) && isPow2(im->height())) {
            dim = Texture::DIM_2D;
        } else {
            dim = Texture::DIM_2D_NPOT;
        }
            
        Texture::Settings settings;
        settings.wrapMode = im->wrapMode();

        tex = Texture::fromMemory
            ("Converted", im->getCArray(), im->format(),
             im->width(), im->height(), 1, ImageUtils::to8(im->format()),
             dim, settings);
    }

    inline MapComponent(const class ReferenceCountedPointer<Image>& im, const Texture::Ref& tex) : 
        m_cpuImage(im), m_gpuImage(tex), m_min(Image::Storage::one()), m_max(Image::Storage::zero()),
        m_mean(Image::Compute::zero()) {

        bool cpuWasNull = m_cpuImage.isNull();

        // Compute min, max, mean
        if (m_cpuImage.isNull() && m_gpuImage.notNull()) {
            m_gpuImage->getImage(m_cpuImage);
        }

        if (m_cpuImage.notNull()) {
            const typename Image::Storage* ptr = m_cpuImage->getCArray();
            typename Image::Compute sum = Image::Compute::zero();
            const int N = m_cpuImage->width() * m_cpuImage->height();
            for (int i = 0; i < N; ++i) {
                m_min  = m_min.min(ptr[i]);
                m_max  = m_max.min(ptr[i]);
                sum   += typename Image::Compute(ptr[i]);
            }
            m_mean = sum / (float)N;
        }

        if (cpuWasNull) {
            // Throw away the CPU image to conserve memory
            m_cpuImage = NULL;
        }
    }

public:

    /** Returns NULL if both are NULL */
    static inline Ref create(const class ReferenceCountedPointer<Image>& im, const Texture::Ref& tex) {
        if (im.isNull() && tex.isNull()) {
            return NULL;
        } else {
            return new MapComponent(im, tex);
        }
    }


    /** Largest value in each channel of the image */
    const typename Image::Storage& max() const {
        return m_max;
    }

    /** Smallest value in each channel of the image */
    const typename Image::Storage& min() const {
        return m_min;
    }

    /** Average value in each channel of the image */
    const typename Image::Compute& mean() const {
        return m_mean;
    }
            
    /** Returns the CPU image portion of this component, synthesizing
        it if necessary.  Returns NULL if there is no GPU data to 
        synthesize from.*/
    const class ReferenceCountedPointer<Image>& image() const {
        MyType* me = const_cast<MyType*>(this);
        if (me->m_cpuImage.isNull()) {
            debugAssert(me->m_gpuImage.notNull());
            // Download from GPU
            m_gpuImage->getImage(me->m_cpuImage);
        }
                
        return m_cpuImage;
    }
    

    /** Returns the GPU image portion of this component, synthesizing
        it if necessary.  */
    const Texture::Ref& texture() const {
        MyType* me = const_cast<MyType*>(this);
        if (me->m_gpuImage.isNull()) {
            debugAssert(me->m_cpuImage.notNull());
                    
            // Upload from CPU
            getTexture(m_cpuImage, me->m_gpuImage);
        }
                
        return m_gpuImage;
    }

    void setStorage(ImageStorage s) const {
        MyType* me = const_cast<MyType*>(this);
        switch (s) {
        case MOVE_TO_CPU:
            image();
            me->m_gpuImage = NULL;
            break;

        case MOVE_TO_GPU:
            texture();
            me->m_cpuImage = NULL;
            break;

        case COPY_TO_GPU:
            texture();
            break;

        case COPY_TO_CPU:
            image();
            break;
        }
    }
            
#   undef MyType
};


/** @brief Common code for G3D::Component1, G3D::Component3, and G3D::Component4.

    Product of a constant and an image. 

    The image may be stored on either the GPU (G3D::Texture) or
    CPU (G3D::Map2D subclass), and both factors are optional. The
    details of this class are rarely needed to use Material, since
    it provides constructors from all combinations of data
    types.
    
    Supports only floating point image formats because bilinear 
    sampling of them is about 9x faster than sampling int formats.
    */
template<class Color, class Image>
class Component {
public:

    enum Factors {
        /** rgb() will always be zero (says nothing about the alpha value) */
        BLACK,

        /** There is no map, but there is a non-black constant specified. */
        CONSTANT,

        /** There is no constant, but there is a map specified that is assumed to be not all black. */
        MAP,

        /** There is a map and a non-black constant. */
        MAP_TIMES_CONSTANT
    };

private:

    Factors                   m_factors;
    Color                     m_constant;

    Color                     m_max;
    Color                     m_min;
    Color                     m_mean;

    /** NULL if there is no map. This is a reference so that
        multiple Components may share a texture and jointly
        move it to and from the GPU.*/
    ReferenceCountedPointer<MapComponent<Image> >  m_map;

    void init(const Color& constant) {
        m_constant = constant;

        if (constant.rgb() == Color3::zero()) {
            m_factors = BLACK;
        } else if (m_map.isNull()) {
            m_factors = CONSTANT;
        } else if (constant.rgb() == Color3::one()) {
            m_factors = MAP;
        } else {
            m_factors = MAP_TIMES_CONSTANT;
        }

        if (m_map.notNull()) {
            m_max  = m_constant * m_map->max();
            m_min  = m_constant * m_map->min();
            m_mean = m_constant * m_map->mean();
        } else {
            m_max  = m_constant;
            m_min  = m_constant;
            m_mean = m_constant;
        }
    }

public:

    /** All zero */
    Component() : m_map(NULL) {
        init(Color::zero());
    }
        
    /** Assumes a map of NULL (all white) if not specified */
    Component
    (const Color&                    constant, 
     const ReferenceCountedPointer<MapComponent<Image> >& map = NULL) :
        m_map(map) {

        init(constant);
    }

    Component
    (const Color&                    constant, 
     const ReferenceCountedPointer<Image>& map) :
        m_map(MapComponent<Image>::create(map, NULL)) {

        init(constant);
    }

    Component
    (const Color&                    constant, 
     const Texture::Ref&             map) :
        m_map(MapComponent<Image>::create(NULL, map)) {

        init(constant);
    }

    /** Assumes a constant of 1 */
    Component(const ReferenceCountedPointer<MapComponent<Image> >& map) : m_map(map) {
        init(Color::one());
    }
    
    inline bool operator==(const Component<Color, Image>& other) const {
        return 
            (m_factors == other.m_factors) &&
            (m_constant == other.m_constant) &&
            (m_map == other.m_map);
    }
    
    inline Factors factors() const {
        return m_factors;
    }
        
    /** Return constant * map.  Optimized to only perform as many
        operations as needed.

        If the component contains a texture map that has not been
        converted to a CPU image, that conversion is
        performed. Because that process is not threadsafe, when using
        sample() in a multithreaded environment, first invoke setStorage(COPY_TO_CPU)
        on every Component from a single thread to prime the CPU data
        structures.

        Coordinates are normalized; will be scaled by the image width and height
        automatically.
    */
    inline Color sample(const Vector2& pos) const {
        switch (m_factors) {
        case BLACK:
        case CONSTANT:
            return m_constant;

        case MAP:
            {
                const typename Image::Ref& im = m_map->image();
                return im->bilinear(pos * Vector2(im->width(), im->height()));
            }
        case MAP_TIMES_CONSTANT:
            {
                const typename Image::Ref& im = m_map->image();
                return im->bilinear(pos * Vector2(im->width(), im->height())) * m_constant;
            }

        default:
            alwaysAssertM(false, "fell through switch");
            return Color::zero();
        }
    }
        
    inline const Color& constant() const {
        return m_constant;
    }

    /** Largest value per color channel */
    inline const Color& max() const {
        return m_max;
    }

    /** Smallest value per color channel */
    inline const Color& min() const {
        return m_min;
    }

    /** Average value per color channel */
    inline const Color& mean() const {
        return m_mean;
    }

    /** Causes the image to be created by downloading from GPU if necessary. 
        Returns NULL if the underlying map is NULL.*/
    inline const ReferenceCountedPointer<Image>& image() const {
        if (m_map.isNull()) {
            static const ReferenceCountedPointer<Image> n;
            return n;
        } else {
            return m_map->image();
        }
    }

    /** Causes the texture to be created by uploading from CPU if necessary. 
        Returns NULL if the underlying map is NULL.*/
    inline const Texture::Ref& texture() const {
        if (m_map.isNull()) {
            static const Texture::Ref n;
            return n;
        } else {
            return m_map->texture();
        }
    }

    /** Does not change storage if the map is NULL */
    inline void setStorage(ImageStorage s) const {
        if (m_map.notNull()) {
            m_map->setStorage(s);
        }
    }

    /** Says nothing about the alpha channel */
    inline bool notBlack() const {
        return factors() != BLACK;
    }

    /** Says nothing about the alpha channel */
    inline bool isBlack() const {
        return factors() == BLACK;
    }
};

typedef Component<Color1, Image1> Component1;

typedef Component<Color3, Image3> Component3;

typedef Component<Color4, Image4> Component4;

}

#endif

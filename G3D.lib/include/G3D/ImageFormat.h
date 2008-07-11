/**
  @file ImageFormat.h

  @maintainer Morgan McGuire, matrix@graphics3d.com

  @created 2003-05-23
  @edited  2008-07-01
*/

#ifndef GLG3D_ImageFormat_H
#define GLG3D_ImageFormat_H

#include "G3D/platform.h"
#include "G3D/Table.h"

namespace G3D {

/** Information about common image formats.
    Don't construct these; use the methods provided. 
    
    For most formats, the number indicates the number of bits per channel and a suffix of "F" indicates
    floating point.  This does not hold for the YUV and DXT formats.*/
class ImageFormat {
public:

    // Must update ImageFormat::name() when this enum changes.
    enum Code {
        CODE_L8,
        CODE_L16,
        CODE_L16F,
        CODE_L32F,

        CODE_A8,
        CODE_A16,
        CODE_A16F,
        CODE_A32F,

        CODE_LA4,
        CODE_LA8,
        CODE_LA16,
        CODE_LA16F,
        CODE_LA32F,

        CODE_RGB5,
        CODE_RGB5A1,
        CODE_RGB8,
        CODE_RGB10,
        CODE_RGB10A2,
        CODE_RGB16,
        CODE_RGB16F,
        CODE_RGB32F,

        CODE_ARGB8,
        CODE_BGR8,

        CODE_RGBA8,
        CODE_RGBA16,
        CODE_RGBA16F,
        CODE_RGBA32F,

        CODE_BAYER_RGGB8,
        CODE_BAYER_GRBG8,
        CODE_BAYER_GBRG8,
        CODE_BAYER_BGGR8,
        CODE_BAYER_RGGB32F,
        CODE_BAYER_GRBG32F,
        CODE_BAYER_GBRG32F,
        CODE_BAYER_BGGR32F,

        CODE_HSV8,
        CODE_HSV32F,

        CODE_YUV8,
        CODE_YUV32F,
        CODE_YUV411,
        CODE_YUV420,
        CODE_YUV444,

        CODE_RGB_DXT1,
        CODE_RGBA_DXT1,
        CODE_RGBA_DXT3,
        CODE_RGBA_DXT5,

        CODE_DEPTH16,
        CODE_DEPTH24,
        CODE_DEPTH32,
        CODE_DEPTH32F,
        
        CODE_STENCIL1,
        CODE_STENCIL4,
        CODE_STENCIL8,
        CODE_STENCIL16,

        CODE_DEPTH24_STENCIL8,

        CODE_NUM
        };

    enum ColorSpace {
        COLOR_SPACE_NONE,
        COLOR_SPACE_RGB,
        COLOR_SPACE_HSV,
        COLOR_SPACE_YUV
    };

    enum BayerPattern {
        BAYER_PATTERN_NONE,
        BAYER_PATTERN_RGGB,
        BAYER_PATTERN_GRBG,
        BAYER_PATTERN_GBRG,
        BAYER_PATTERN_BGGR
    };

    /**  Number of channels (1 for a depth texture). */
    int                 numComponents;
    bool                compressed;

    /** Useful for serializing. */
    Code                code;

    ColorSpace          colorSpace;

    /** If this is a Bayer format, what is the pattern. */
    BayerPattern        bayerPattern;

    /** The OpenGL format equivalent to this one, e.g, GL_RGB8  Zero if there is no equivalent. This is actually a GLenum */
    int                 openGLFormat;

    /** The OpenGL base format equivalent to this one (e.g., GL_RGB, GL_ALPHA).  Zero if there is no equivalent.  */
    int                 openGLBaseFormat;

    int                 luminanceBits;

    /** Number of bits per pixel storage for alpha values; Zero for compressed textures and non-RGB. */
    int                 alphaBits;
    
    /** Number of bits per pixel storage for red values; Zero for compressed textures and non-RGB. */
    int                 redBits;

    /** Number of bits per pixel storage for green values; Zero for compressed textures and non-RGB. */
    int                 greenBits;
    
    /** Number of bits per pixel storage for blue values; Zero for compressed textures  and non-RGB. */
    int                 blueBits;

    /** Number of bits per pixel */
    int                 stencilBits;

    /** Number of depth bits (for depth textures; e.g. shadow maps) */
    int                 depthBits;

    /** Amount of CPU memory per pixel when packed into an array, discounting any end-of-row padding. */
    int                 cpuBitsPerPixel;

    /** Amount of CPU memory per pixel when packed into an array, discounting any end-of-row padding. @deprecated Use cpuBitsPerPixel*/
    int                 packedBitsPerTexel;
    
    /**
      Amount of GPU memory per pixel on most graphics cards, for formats supported by OpenGL. This is
      only an estimate--the actual amount of memory may be different on your actual card.

     This may be greater than the sum of the per-channel bits
     because graphics cards need to pad to the nearest 1, 2, or
     4 bytes.
     */
    int                 openGLBitsPerPixel;

    /** @deprecated Use openGLBitsPerPixel */
    int                 hardwareBitsPerTexel;

    /** The OpenGL bytes format of the data buffer used with this texture format, e.g., GL_UNSIGNED_BYTE */
    int                 openGLDataFormat;

    /** True if there is no alpha channel for this texture. */
    bool                opaque;

    /** True if the bit depths specified are for float formats. */
    bool                floatingPoint;

    /** Human readable name of this texture.*/
    std::string name() const;

private:

    ImageFormat(
        int             numComponents,
        bool            compressed,
        int             glFormat,
        int             glBaseFormat,
        int             luminanceBits,
        int             alphaBits,
        int             redBits,
        int             greenBits,
        int             blueBits,
        int             depthBits,
        int             stencilBits,
        int             hardwareBitsPerTexel,
        int             packedBitsPerTexel,
        int             glDataFormat,
        bool            opaque,
        bool            floatingPoint,
        Code            code,
        ColorSpace      colorSpace,
        BayerPattern    bayerPattern = BAYER_PATTERN_NONE);

public:

    static const ImageFormat* L8();

    static const ImageFormat* L16();

    static const ImageFormat* L16F();
    
    static const ImageFormat* L32F();

    static const ImageFormat* A8();

    static const ImageFormat* A16();

    static const ImageFormat* A16F();
    
    static const ImageFormat* A32F();

    static const ImageFormat* LA4();

    static const ImageFormat* LA8();

    static const ImageFormat* LA16();

    static const ImageFormat* LA16F();
    
    static const ImageFormat* LA32F();

    static const ImageFormat* BGR8();

    static const ImageFormat* RGB5();

    static const ImageFormat* RGB5A1();

    static const ImageFormat* RGB8();

    static const ImageFormat* RGB10();

    static const ImageFormat* RGB10A2();

    static const ImageFormat* RGB16();

    static const ImageFormat* RGB16F();

    static const ImageFormat* RGB32F();

    static const ImageFormat* RGBA8();

    static const ImageFormat* RGBA16();

    static const ImageFormat* RGBA16F();
    
    static const ImageFormat* RGBA32F();
    
    static const ImageFormat* RGB_DXT1();

    static const ImageFormat* RGBA_DXT1();

    static const ImageFormat* RGBA_DXT3();

    static const ImageFormat* RGBA_DXT5();

    static const ImageFormat* DEPTH16();

    static const ImageFormat* DEPTH24();

    static const ImageFormat* DEPTH32();

    static const ImageFormat* DEPTH32F();

    static const ImageFormat* STENCIL1();

    static const ImageFormat* STENCIL4();

    static const ImageFormat* STENCIL8();

    static const ImageFormat* STENCIL16();

    static const ImageFormat* DEPTH24_STENCIL8();

	/**
     NULL pointer; indicates that the G3D::Texture class should choose
     either RGBA8 or RGB8 depending on the presence of an alpha channel
     in the input.
     */
    static const ImageFormat* AUTO()  { return NULL; }

    /** Returns DEPTH16, DEPTH24, or DEPTH32 according to the bits
     specified. You can use "glGetInteger(GL_DEPTH_BITS)" to match 
     the screen's format.*/
    static const ImageFormat* depth(int depthBits = 24);

    /** Returns STENCIL1, STENCIL4, STENCIL8 or STENCIL16 according to the bits
      specified. You can use "glGetInteger(GL_STENCIL_BITS)" to match 
     the screen's format.*/
    static const ImageFormat* stencil(int bits = 8);

    /** Returns the matching ImageFormat* identified by the Code.  May return NULL
      if this format's code is reserved but not yet implemented by G3D. */
    static const ImageFormat* fromCode(ImageFormat::Code code);
};

typedef ImageFormat TextureFormat;

}

template <>
struct HashTrait<const G3D::ImageFormat*> {
    static size_t hashCode(const G3D::ImageFormat* key) { return reinterpret_cast<size_t>(key); }
};



#endif
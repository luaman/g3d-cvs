/**
  @file Texture.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2001-02-28
  @edited  2010-01-27
*/

#ifndef GLG3D_Texture_h
#define GLG3D_Texture_h

#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "G3D/WrapMode.h"
#include "G3D/ImageFormat.h"
#include "G3D/Image1.h"
#include "G3D/Image1uint8.h"
#include "G3D/Image3.h"
#include "G3D/Image3uint8.h"
#include "G3D/Image4.h"
#include "G3D/Image4uint8.h"
#include "G3D/BumpMapPreprocess.h"
#include "GLG3D/glheaders.h"

namespace G3D {

class GImage;
class Rect2D;
class Matrix3;
class Texture;

/** @deprecated Use Texture::Ref */
typedef ReferenceCountedPointer<Texture> TextureRef;

/**
 @brief A 1D, 2D, or 3D array (e.g., an image) stored on the GPU, commonly used for
  mapping reflectance values (colors) over meshes.

 Abstraction of OpenGL textures.  This class can be used with raw OpenGL, 
 without RenderDevice.  G3D::Texture supports all of the image formats
 that G3D::GImage can load, and DDS (DirectX textures), and Quake-style cube 
 maps.

 If you enable texture compression, textures will be compressed on the fly.
 This can be slow (up to a second).

 The special filename "<white>" generates an all-white Color4 texture.  You can use Preprocess::modulate
 to create other colors from this.

 Unless DIM_2D_RECT, DIM_2D_NPOT, DIM_CUBE_MAP_NPOT are used, the texture is automatically
 scaled to the next power of 2 along each dimension to meet hardware requirements, if not
 already a power of 2.  However, DIM_2D_NPOT and DIM_CUBE_MAP_NPOT will safely fallback to
 POT requirements if the ARB_non_power_of_two extension is not supported. Develoeprs can 
 check if this will happen by calling GLCaps::supports_GL_ARB_texture_non_power_of_two().
 Note that the texture does not have to be a rectangle; the dimensions can be different powers of two.
 DIM_2D_RECT is provided primarily for older cards only and does not interact well with shaders.

 Textures are loaded so that (0, 0) is the upper-left corner of the image. 

 DIM_2D_RECT requires the GL_EXT_texture_rectangle extension.
 Texture compression requires the EXT_texture_compression_s3tc extions.
 You can either query OpenGL for whether these are supported or
 use the G3D::GLCaps facility for doing so.

 G3D::Texture can be used with straight OpenGL, without G3D::RenderDevice, as
 follows:

 <PRE>
  Texture::Ref texture = new Texture("logo.jpg");

  ...
    
  GLint u = texture->getOpenGLTextureTarget();
  glEnable(u);
  glBindTexture(u, texture->getOpenGLID());
 </PRE>

 To use Texture with RenderDevice:

  <PRE>
  Texture::Ref texture = new Texture("logo.jpg");
  ...
  renderDevice->setTexture(0, texture);
  // (to disable: renderDevice->setTexture(0, NULL);)
  </PRE>

  3D MIP Maps are not supported because gluBuild3DMipMaps is not in all GLU implementations.

  \sa G3D::RenderDevice::setBlendFunc for important information about turning on 
  alpha blending when using textures with alpha.
 */
class Texture : public ReferenceCountedObject {
public:

    enum CubeFace {
        CUBE_POS_X = 0,
        CUBE_NEG_X = 1,
        CUBE_POS_Y = 2,
        CUBE_NEG_Y = 3,
        CUBE_POS_Z = 4,
        CUBE_NEG_Z = 5,

		NUM_CUBEFACES
	};

    /** Image alignment conventions specified by different APIs. 
        G3D loads cube maps so that they act like reflection maps.
        That is, it assumes you are <i>inside</i> the cube map.
     */
    enum CubeMapConvention {
        /** Uses "up", "lf", etc. */
        CUBE_QUAKE, 
        
        /** Uses "up", "west", etc. */
        CUBE_UNREAL, 

        /** Uses "+y", "-x", etc. */
        CUBE_G3D,

        /** Uses "PY", "NX", etc. */
        CUBE_DIRECTX,

		NUM_CUBECONVENTIONS
	};

    struct CubeMapInfo {
        struct Face {
            /** True if the face is horizontally flipped */
            bool            flipX;

            /** True if the face is vertically flipped */
            bool            flipY;

            /** Number of CW 90-degree rotations to perform after flipping */
            int             rotations;

            /** Filename suffix */
			std::string     suffix;

            Face() : flipX(true), flipY(false), rotations(0) {}
        };

		std::string         name;

        /** Index using CubeFace */
        Face                face[6];
    };

    /**
     Returns the rotation matrix that should be used for rendering the
     given cube map face.  The orientations will seem to have the
     camera "upside down" compared to what you might expect because
     OpenGL's cube map convention and texture convention are both
     inverted from how we usually visualize the data.
     
     The resulting cube maps can be saved to disk by:

     <pre>
     const Texture::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
     GImage temp;
     renderTarget->getImage(temp, ImageFormat::RGB8(), true);

     temp.rotate90CW(-faceInfo.rotations);
     if (faceInfo.flipY) {
         temp.flipVertical();
     }
     if (faceInfo.flipX) {
         temp.flipHorizontal();
     }
     temp.save(format("out-%s.png", faceInfo.suffix));
     </pre>

    */
    static void getCubeMapRotation(CubeFace face, Matrix3& outMatrix);

    /** \a filename should contain a * wildcard */
    static CubeMapConvention determineCubeConvention(const std::string& filename);
    
    /** Returns the mapping from [0, 5] to cube map faces and filename
        suffixes. There are multiple filename conventions, so the
        suffixes specify each of the options. */
    static const CubeMapInfo& cubeMapInfo(CubeMapConvention convention);

    /** Reference counted pointer to a Texture.*/
    typedef ReferenceCountedPointer<class Texture> Ref;

    /** DIM_2D_NPOT and DIM_CUBE_MAP_NPOT attempt to use
        ARB_non_power_of_two texture support with POT fallback.

        \sa defaultDimension */
    enum Dimension {
		DIM_2D = 2, 
         DIM_3D = 3, 
         DIM_2D_RECT = 4, 
         DIM_CUBE_MAP = 5,
         DIM_2D_NPOT = 6, 
         DIM_CUBE_MAP_NPOT = 7,
         DIM_3D_NPOT = 8
	};

    /** 
      Returns true if this is a legal wrap mode for a G3D::Texture.
     */
    static bool supportsWrapMode(WrapMode m) {
        return (m == WrapMode::TILE) || (m == WrapMode::CLAMP) || (m == WrapMode::ZERO);
    }

    /**
     Trilinear mipmap is the best quality (and frequently fastest)
     mode.  The no-mipmap modes conserve memory.  Non-interpolating
     ("Nearest") modes are generally useful only when packing lookup
     tables into textures for shaders.

     3D textures do not support mipmap interpolation modes.
     */
    enum InterpolateMode {
        TRILINEAR_MIPMAP = 3, 
        BILINEAR_MIPMAP = 4,
        NEAREST_MIPMAP = 5,

        BILINEAR_NO_MIPMAP = 2,
        NEAREST_NO_MIPMAP = 6
	};

    static const char* toString(InterpolateMode m);
    static InterpolateMode toInterpolateMode(const std::string& s);

    /** A m_depth texture can automatically perform the m_depth
        comparison used for shadow mapping on a texture lookup.  The
        result of a texture lookup is thus the shadowed amount (which
        will be percentage closer filtered on newer hardware) and
        <I>not</I> the actual m_depth from the light's point of view.
       
        This combines GL_TEXTURE_COMPARE_MODE_ARB and GL_TEXTURE_COMPARE_FUNC_ARB from
        http://www.nvidia.com/dev_content/nvopenglspecs/GL_ARB_shadow.txt

        For best results on percentage closer hardware (GeForceFX and Radeon9xxx or better), 
        create shadow maps as m_depth textures with BILINEAR_NO_MIPMAP sampling.

        See also G3D::RenderDevice::configureShadowMap and the Collision_Demo.
     */
    enum DepthReadMode {DEPTH_NORMAL = 0, DEPTH_LEQUAL = 1, DEPTH_GEQUAL = 2};
    
    static const char* toString(DepthReadMode m);
    static DepthReadMode toDepthReadMode(const std::string& s);
    
    static const char* toString(Dimension m);
    static Dimension toDimension(const std::string& s);

    /**
     Splits a filename around the '*' character-- used by cube maps to generate all filenames.
     */
    static void splitFilenameAtWildCard(
        const std::string&  filename,
        std::string&        filenameBeforeWildCard,
        std::string&        filenameAfterWildCard);

    /**
       Returns true if the specified filename exists and is an image that can be loaded as a Texture.
    */
    static bool isSupportedImage(const std::string& filename);
    
    /** @brief Returns a small all-white (1,1,1,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static Texture::Ref white();

    /** @brief Returns a small opaque all-black (0,0,0,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static Texture::Ref opaqueBlack();

    /** @brief Returns a small, all zero Color4(0,0,0,0) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static Texture::Ref zero();

    /** @brief Returns a small all-gray (0.5,0.5,0.5,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static Texture::Ref opaqueGray();

    /** Creates a new 1x1 texture in this color. Colors are not cached.*/
    static Texture::Ref createColor(const Color3uint8& c);

    /** Creates a new 1x1 texture in this color. Colors are not cached.*/
    static Texture::Ref createColor(const Color4uint8& c);

    /** @copydoc white(). */
    inline static Texture::Ref one() {
        return white();
    }

    /** Returns \a t if it is non-NULL, or white() if \a t is NULL */
    inline static Texture::Ref whiteIfNull(const Texture::Ref& t) {
        if (t.isNull()) {
            return white();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-NULL, or opaqueBlack() if \a t is NULL */
    inline static Texture::Ref opaqueBlackIfNull(const Texture::Ref& t) {
        if (t.isNull()) {
            return opaqueBlack();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-NULL, or zero() if \a t is NULL */
    inline static Texture::Ref zeroIfNull(const Texture::Ref& t) {
        if (t.isNull()) {
            return zero();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-NULL, or gray() if \a t is NULL */
    inline static Texture::Ref opaqueGrayIfNull(const Texture::Ref& t) {
        if (t.isNull()) {
            return opaqueGray();
        } else {
            return t;
        }
    }

    /** All parameters of a texture that are independent of the
        underlying image data.  */
    class Settings {
    public:
        /** Default is TRILINEAR_MIPMAP */
        InterpolateMode             interpolateMode;

        /** Default is TILE */
        WrapMode                    wrapMode;

        /** Default is DEPTH_NORMAL */
        DepthReadMode               depthReadMode;

        /** Default is 2.0 */
        float                       maxAnisotropy;

        /** Default is true */
        bool                        autoMipMap;

        /**
         Highest MIP-map level that will be used during rendering.
         The highest level that actually exists will be L =
         log(max(m_width, m_height), 2)), although it is fine to set
         maxMipMap higher than this.  Must be larger than minMipMap.
         Default is 1000.

         Setting the max mipmap level is useful for preventing
         adjacent areas of a texture from being blurred together when
         viewed at a distance.  It may decrease performance, however,
         by forcing a larger texture into cache than would otherwise
         be required.
         */
        int                         maxMipMap;

        /**
         Lowest MIP-map level that will be used during rendering.
         Level 0 is the full-size image.  Default is -1000, matching
         the OpenGL spec.  @cite
         http://oss.sgi.com/projects/ogl-sample/registry/SGIS/texture_lod.txt
         */
        int                         minMipMap;

        Settings();

        /** \param any Must be in the form of a table of the fields or appear as
            a call to a static factory method, e.g.,:

            - Texture::Settings{  interpolateMode = "TRILINEAR_MIPMAP", wrapMode = "TILE", ... }
            - Texture::Settings::video()
        */
        Settings(const Any& any);

        static const Settings& defaults();

        /** 
          Useful defaults for video/image processing.
          BILINEAR_NO_MIPMAP / CLAMP / DEPTH_NORMAL / 1.0 / automipmap =false
        */
        static const Settings& video();

        /** 
          Useful defaults for general purpose computing.
          NEAREST_NO_MIPMAP / CLAMP / DEPTH_NORMAL / 1.0 / false
        */
        static const Settings& buffer();

        /** 
          Useful defaults for shadow maps.
          BILINEAR_NO_MIPMAP / CLAMP / DEPTH_LEQUAL / 1.0 / false
        */
        static const Settings& shadow();

        /**
           Useful defaults for cube maps
           TRILINEAR_MIPMAP / CLAMP, DEPTH_NORMAL / 1.0 / true 
         */
        static const Settings& cubeMap();

        /*
         Coming in a future version...
        void serialize(class BinaryOutput& b);
        void deserialize(class BinaryInput& b);
        void serialize(class TextOutput& t);
        void deserialize(class TextInput& t);
        */

        bool operator==(const Settings& other) const;

        /** True if both Settings are identical, ignoring mipmap settings.*/
        bool equalsIgnoringMipMap(const Settings& other) const;
        size_t hashCode() const;
    };


    class Preprocess {
    public:

        /** Multiplies color channels.  Useful for rescaling to make
            textures brighter (e.g., for Quake textures, which are
            dark) or to tint textures as they are loaded.  Modulation
            happens first of all preprocessing.
         */
        Color4                      modulate;

        /**
           After brightening, each (unit-scale) pixel is raised to
           this power. Many textures are drawn to look good when
           displayed on the screen in PhotoShop, which means that they
           are drawn with a document gamma of about 2.2.

           If the document gamma is 2.2, set \a gammaAdjust to:
           <ul>
             <li> 2.2 for reflectivity, emissive, and environment maps (e.g., lambertian, glossy, etc.)
             <li> 1.0 for 2D elements, like fonts and full-screen images
             <li> 1.0 for computed data (e.g., normal maps, bump maps, GPGPU data)
           </ul>

           To maintain maximum precision, author and store the
           original image files in a 1.0 gamma space, at which point
           no gamma correction is necessary.
         */
        float                       gammaAdjust;

        /** Amount to resize images by before loading onto the
            graphics card to save memory; typically a negative power
            of 2 (e.g., 1.0, 0.5, 0.25). Scaling happens last of all
            preprocessing.*/
        float                       scaleFactor;

        /** If true (default), constructors automatically compute the min, max, and mean
            value of the texture. This is necessary, for example, for use with SuperBSDF. */
        bool                        computeMinMaxMean;

        /** If true, treat the input as a monochrome bump map and compute a normal map from
            it where the RGB channels are XYZ and the A channel is the input bump height.*/
        bool                        computeNormalMap;

        BumpMapPreprocess           bumpMapPreprocess;


        Preprocess() : modulate(Color4::one()), gammaAdjust(1.0f), scaleFactor(1.0f), computeMinMaxMean(true),
                       computeNormalMap(false) {}

        /** \param any Must be in the form of a table of the fields or appear as
            a call to a static factory method, e.g.,:

            - Texture::Preprocess{  modulate = Color4(...), ... }
            - Texture::Preprocess::gamma(2.2)
            - Texture::Preprocess::none()
        */
        Preprocess(const Any& a);

        /** Defaults + gamma adjust set to g*/
        static Preprocess gamma(float g);

        static const Preprocess& defaults();

        /** Default settings + computeMinMaxMean = false */
        static const Preprocess& none();

        /** Brighten by 2 and adjust gamma by 1.6, the default values
            expected for Quake versions 1 - 3 textures.*/
        static const Preprocess& quake();

        static const Preprocess& normalMap();

        bool operator==(const Preprocess& other) const;
        
        bool operator!=(const Preprocess& other) const {
            return !(*this == other);
        }

    };

private:

    /** OpenGL texture ID */
    GLuint                          m_textureID;

    /** Set in the base constructor. */
    Settings                        m_settings;

    std::string                     m_name;
    Dimension                       m_dimension;
    bool                            m_opaque;

    const ImageFormat*              m_format;
    int                             m_width;
    int                             m_height;
    int                             m_depth;

    Color4                          m_min;
    Color4                          m_max;
    Color4                          m_mean;

    static int64                    m_sizeOfAllTexturesInMemory;

    Texture
    (const std::string&          name,
     GLuint                      textureID,
     Dimension                   dimension,
     const ImageFormat*          format,
     bool                        opaque,
     const Settings&             settings);
    
public:

    class Specification {
    public:
        std::string               filename;

        /** Defaults to ImageFormat::AUTO() */
        const class ImageFormat*  desiredFormat;

        /** Defaults to ImageFormat::DIM_2D_NPOT on cards that support
            it, DIM_2D otherwise. */
        Dimension                 dimension;

        Settings                  settings;

        Preprocess                preprocess;

        Specification() : desiredFormat(ImageFormat::AUTO()), 
                          dimension(defaultDimension()) {}

        Specification(const Any& any);

        bool operator==(const Specification& s) const;

        bool operator!=(const Specification& s) const {
            return !(*this == s);
        }
    };

    static Ref create(const Specification& s);

    /** Call glGetTexImage with appropriate target. 
    
        This will normally perform a synchronous read, which causes
        the CPU to stall while the GPU catches up, and then stalls the
        GPU while data is being read.  For higher performance, use an
        OpenGL PixelBufferObject to perform an asynchronous read.  PBO
        is not abstracted by G3D.  The basic operation is:

        <pre>
            GLuint pbo;
            glGenBuffersARB(1, &pbo);
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
            glBufferData(GL_PIXEL_PACK_BUFFER_ARB, dataByteSize, NULL, GL_STREAM_READ);
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
            debugAssertGLOk();

            // Issue the read (does not stall the GPU):
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
            texture->getTexImage((void*)0, dataFormat);
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

            // [[ Do some CPU processing while waiting for the data to transfer asynchronously ]]
            ...

            // Bind the buffer.  This will block if the data is not yet available.
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
            void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    
            // [[ Process the data ]]
            ...

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
            glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
         </pre>
    */
    void getTexImage(void* data, const ImageFormat* desiredFormat) const;

    /** Reads back a single texel.  This is faster than reading an entire image, but 
        still stalls the pipeline because it is synchronous.
        \beta 

        \a rd If NULL, set to RenderDevice::lastRenderDeviceCreated;
    */
    Color4 readTexel(int ix, int iy, class RenderDevice* rd = NULL) const;

    /** Returns the default Dimension for this machine, which is
        DIM_2D_NPOT if supported and DIM_2D if not.*/
    static Dimension defaultDimension();

    /**
     Creates an empty texture (useful for later reading from the screen).
     
     \sa clear()
     */
    static Texture::Ref createEmpty(
        const std::string&              name,
        int                             width,
        int                             height,
        const ImageFormat*              desiredFormat  = ImageFormat::RGBA8(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        int                             depth          = 1);

    /** Clear the texture to empty (typically after creation, so that it does
        not contain unitialized data).
        Requires the Framebuffer Object extension.

        \a rd If NULL, set to RenderDevice::lastRenderDeviceCreated

        \beta
    */
    void clear(CubeFace face = CUBE_POS_X, int mipLevel = 0, class RenderDevice* rd = NULL);

    /** Resize the underlying OpenGL texture memory buffer, without reallocating the OpenGL texture ID.  This does not scale the contents; 
        the contents are undefined after resizing.  This is only useful for textures that are render targets. */
    void resize(int w, int h);

    /**
     Wrap and interpolate will override the existing parameters on the
     GL texture.

     @param name Arbitrary name for this texture to identify it
     @param textureID Set to newGLTextureID() to create an empty texture.
     */
    static Texture::Ref fromGLTexture(
        const std::string&              name,
        GLuint                          textureID,
        const ImageFormat*              textureFormat,
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults());

    /**
     Creates a texture from a single image.  The image must have a format understood
     by G3D::GImage or a DirectDraw Surface (DDS).  If dimension is DIM_CUBE_MAP, this loads the 6 files with names
     _ft, _bk, ... following the G3D::Sky documentation.
     */    
    static Texture::Ref fromFile(
        const std::string&              filename,
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        const Preprocess&               process        = Preprocess());

    /**
     Creates a cube map from six independently named files.  The first
     becomes the name of the texture.
     */
    static Texture::Ref fromFile(
        const std::string               filename[6],
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        const Preprocess&               process        = Preprocess());

    /**
     Creates a texture from the colors of filename and takes the alpha values
     from the red channel of alpha filename. See G3D::RenderDevice::setBlendFunc
	 for important information about turning on alpha blending. 
     */
    static Texture::Ref fromTwoFiles(
        const std::string&              filename,
        const std::string&              alphaFilename,
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        const Preprocess&               process        = Preprocess());

    /**
    Construct from an explicit set of (optional) mipmaps and (optional) cubemap faces.

    bytes[miplevel][cubeface] is a pointer to the bytes
    for that miplevel and cube
    face. If the outer array has only one element and the
    interpolation mode is
    TRILINEAR_MIPMAP, mip-maps are automatically generated from
	the level 0 mip-map.

    There must be exactly
    6 cube faces per mip-level if the dimensions are
    DIM_CUBE and and 1 per
    mip-level otherwise. You may specify compressed and
    uncompressed formats for
    both the bytesformat and the desiredformat.
   
    3D Textures map not use mip-maps.

    Data is converted between normalized fixed point and floating point as described in section 2.1.5 of the OpenGL 3.2 specification.
    Specifically, uint8 values are converted to floating point by <code>v' = v / 255.0f</code>.  Note that this differs from
    how G3D::Color1uint8 converts to G3D::Color3.

    Note: OpenGL stores values at texel centers.  Thus element at integer position (x, y) in the input "image" is stored at
    texture coordinate ((x + 0.5) / width, (x + 0.5) / height).
    */
    static Texture::Ref fromMemory(
        const std::string&                  name,
        const Array< Array<const void*> >&  bytes,
        const ImageFormat*                  bytesFormat,
        int                                 m_width,
        int                                 m_height,
        int                                 m_depth,
        const ImageFormat*                  desiredFormat  = ImageFormat::AUTO(),
        Dimension                           dimension      = defaultDimension(),
        const Settings&                     settings       = Settings::defaults(),
        const Preprocess&                   preprocess     = Preprocess::defaults());


	 /** Construct from a single packed 2D or 3D data set.  For 3D
         textures, the interpolation mode must be one that does not
         use MipMaps. */
    static Texture::Ref fromMemory(
        const std::string&              name,
        const void*                     bytes,
        const ImageFormat*              bytesFormat,
        int                             m_width,
        int                             m_height,
        int	                        m_depth,
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        const Preprocess&               preprocess     = Preprocess::defaults());

    static Texture::Ref fromGImage(
        const std::string&              name,
        const GImage&                   image,
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = defaultDimension(),
        const Settings&                 settings       = Settings::defaults(),
        const Preprocess&               preprocess     = Preprocess::defaults());

    /** Creates another texture that is the same as this one but contains only
        an alpha channel.  Alpha-only textures are useful as mattes.  
        
        If the current texture is opaque(), returns NULL (since it is not useful
        to construct an alpha-only version of a texture without an alpha channel).
        
        Like all texture construction methods, this is fairly
        slow and should not be called every frame during interactive rendering.*/
    Texture::Ref alphaOnlyVersion() const;

    /**
     Helper method. Returns a new OpenGL texture ID that is not yet managed by a G3D Texture.
     */
    static unsigned int newGLTextureID();

    /**
     Copies data from screen into an existing texture (replacing whatever was
     previously there).  The dimensions must be powers of two or a texture 
     rectangle will be created (not supported on some cards).

     <i>This call is provided for backwards compatibility on old cards. It is 
      substantially slower than simply rendering to a
     G3D::Texture using a G3D::Framebuffer.</i>

     The (x, y) coordinates are in OpenGL coordinates.  If a FrameBuffer is bound then (0, 0) is the top left
     of the screen.  When rendering directly to a window, (0,0) is the lower left.
     Use RenderDevice::copyTextureFromScreen to obtain consistent coordinates.

     The texture dimensions will be updated but all other properties will be preserved:
     The previous wrap mode will be preserved.
     The interpolation mode will be preserved (unless it required a mipmap,
     in which case it will be set to BILINEAR_NO_MIPMAP).  The previous color m_depth
     and alpha m_depth will be preserved.  Texture compression is not supported for
     textures copied from the screen.

     To copy a depth texture, first create an empty depth texture then copy into it.

     If you invoke this method on a texture that is currently set on RenderDevice,
     the texture will immediately be updated (there is no need to rebind).

     \param rect The rectangle to copy (relative to the viewport)

     \param fmt If NULL, uses the existing texture format, otherwise 
     forces this texture to use the specified format.

     @sa RenderDevice::screenShotPic
     @sa RenderDevice::setReadBuffer

     \deprecated
     */
    void copyFromScreen(const Rect2D& rect, const ImageFormat* fmt = NULL);


    /**
     Copies into the specified face of a cube map.  Because cube maps can't have
     the Y direction inverted (and still do anything useful), you should render
     the cube map faces <B>upside-down</B> before copying them into the map.  This
     is an unfortunate side-effect of OpenGL's cube map convention.  
     
     Use G3D::Texture::getCubeMapRotation to generate the (upside-down) camera
     orientations.
     */
    void copyFromScreen(const Rect2D& rect, CubeFace face);

    /**
     How much (texture) memory this texture occupies.  OpenGL backs
     video memory textures with main memory, so the total memory 
     is actually twice this number.
     */
    int sizeInMemory() const;

    /**
     Video memory occupied by all OpenGL textures allocated using Texture
     or maintained by pointers to a Texture.
     */
    inline static int64 sizeOfAllTexturesInMemory() {
        return m_sizeOfAllTexturesInMemory;
    }

    /**
     True if this texture was created with an alpha channel.  Note that
     a texture may have a format that is not opaque (e.g. RGBA8) yet still
     have a completely opaque alpha channel, causing texture->opaque to
     be true.  This is just a flag set for the user's convenience-- it does
     not affect rendering in any way.
	 See G3D::RenderDevice::setBlendFunc
	 for important information about turning on alpha blending. 
     */
    inline bool opaque() const {
        return m_opaque;
    }

    /**
     Returns the level 0 mip-map data in the format that most closely matches
     outFormat.
     @param outFormat Must be one of: ImageFormat::AUTO, ImageFormat::RGB8, ImageFormat::RGBA8, ImageFormat::L8, ImageFormat::A8
     */
    void getImage(GImage& dst, const ImageFormat* outFormat = ImageFormat::AUTO()) const;

    /** Extracts the data as ImageFormat::RGBA32F. */
    Image4::Ref toImage4() const;
    
    /** Extracts the data as ImageFormat::RGBA8.*/
    Image4uint8::Ref toImage4uint8() const;
    
    /** Extracts the data as ImageFormat::RGB32F. */
    Image3::Ref toImage3() const;
    
    /** Extracts the data as ImageFormat::RGB8. */
    Image3uint8::Ref toImage3uint8() const;
    
    /** Extracts the data as ImageFormat::L32F.   */
    Image1::Ref toImage1() const;
    
    /** Extracts the data as ImageFormat::L8. */
    Image1uint8::Ref toImage1uint8() const;
    
    /** Extracts the data as ImageFormat::DEPTH32F. */
    Image1::Ref toDepthImage1() const;
    
    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image4::Ref& im) const {
        im = toImage4();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image3::Ref& im) const {
        im = toImage3();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image1::Ref& im) const {
        im = toImage1();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image4uint8::Ref& im) const {
        im = toImage4uint8();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image3uint8::Ref& im) const {
        im = toImage3uint8();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. */
    inline void getImage(Image1uint8::Ref& im) const {
        im = toImage1uint8();
    }

    /** If this texture was loaded from an uncompressed format in memory or disk (and not rendered to), 
       this is the smallest value in the texture. */
    inline Color4 min() const {
        return m_min;
    }

    /** If this texture was loaded from an uncompressed format in memory or disk (and not rendered to),
       this is the largest value in the texture. */
    inline Color4 max() const {
        return m_max;
    }

    /** If this texture was loaded from an uncompressed format in memory or disk (and not rendered to), 
       this is the average value in the texture. */
    inline Color4 mean() const {
        return m_mean;
    }

    /** Extracts the data as ImageFormat::DEPTH32F */
    Map2D<float>::Ref toDepthMap() const;
    
    /** Extracts the data as ImageFormat::DEPTH32F and converts to 8-bit. Note that you may want to call 
        Image1uint8::flipVertical if Texture::invertY is true.*/
    Image1uint8Ref toDepthImage1uint8() const;
    
    inline unsigned int openGLID() const {
        return m_textureID;
    }

    /** Number of horizontal texels in the level 0 mipmap */
    inline int width() const {
        return m_width;
    }

    /** Number of horizontal texels in the level 0 mipmap */
    inline int height() const {
        return m_height;
    }

    inline int depth() const {
        return m_depth;
    }


    inline Vector2 vector2Bounds() const {
        return Vector2((float)m_width, (float)m_height);
    }

    /** Returns a rectangle whose m_width and m_height match the dimensions of the texture. */
    Rect2D rect2DBounds() const;

    /**
     For 3D textures.
     @deprecated use m_depth()
     */
    inline int texelDepth() const {
        return m_depth;
    }

    inline const std::string& name() const {
        return m_name;
    }

    inline const ImageFormat* format() const {
        return m_format;
    }
    
    inline Dimension dimension() const {
        return m_dimension;
    }

    /**
     Deallocates the OpenGL texture.
     */
    virtual ~Texture();

    /**
     The OpenGL texture target this binds (e.g. GL_TEXTURE_2D)
     */
    unsigned int openGLTextureTarget() const;

    const Settings& settings() const;

    /** Set the autoMipMap value, which only affects textures when they are rendered 
        to or copied from the screen.

        You can read the automipmap value from <code>settings().autoMipMap</code>. 
        */
    void setAutoMipMap(bool b);

    /** For a texture with automipmap off that supports the FrameBufferObject extension, 
       generate mipmaps from the level 0 mipmap immediately.  For other textures, does nothing.*/
    void generateMipMaps();

    /** Allows forcing a change to the depthReadMode of the texture currently bound to the target. */
    void setDepthReadMode(Texture::DepthReadMode depthReadMode);

    /**
       Upload new data from the CPU to this texture.  Corresponds to <a
       href="http://www.opengl.org/sdk/docs/man/xhtml/glTexSubImage2D.xml">glTexSubImage2D</a>.
       If \a src is smaller than the current dimensions of \a this, only
       part of \a this is updated.

       This routine does not provide the same protections as creating a
       new Texture from memory: you must handle scaling and ensure
       compatible formats yourself.

       \param src Must be Image1::Ref, Image1uint8::Ref, Image3::Ref
       Image3::uint8Ref, Image4::Ref, or Image4uint8::Ref
    */
    template<class ImageRef>
    void update(const ImageRef& src, int mipLevel = 0) {
        alwaysAssertM(format()->openGLBaseFormat == src->format()->openGLBaseFormat,
            "Data must have the same number of channels as the texture: this = " + format()->name() + 
            "  src = " + src->format()->name());
        glPushAttrib(GL_TEXTURE_BIT);
        {
            glBindTexture(openGLTextureTarget(), openGLID());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            
            const GLint xoffset = 0;
            const GLint yoffset = 0;
            
            glTexSubImage2D
                (openGLTextureTarget(), 
                 mipLevel,
                 xoffset,
                 yoffset,
                 src->width(), 
                 src->height(), 
                 src->format()->openGLBaseFormat,
                 src->format()->openGLDataFormat, 
                 src->getCArray());
        }
        glPopAttrib();
    }

private:

    /** Allows forcing a change to the depthReadMode of the texture currently bound to the target. */
    static void setDepthTexParameters(GLenum target, Texture::DepthReadMode depthReadMode);
    static void setTexParameters(GLenum target,  const Texture::Settings& settings);

    class DDSTexture {
    private:
                                    
        uint8*                      m_bytes;
        const ImageFormat*          m_bytesFormat;
        int                         m_width;
        int                         m_height;
        int                         m_numMipMaps;
        int                         m_numFaces;

    public:

        DDSTexture(const std::string& filename);

        ~DDSTexture();

        int getWidth() {
            return m_width;
        }

        int getHeight() {
            return m_height;
        }

        const ImageFormat* getBytesFormat() {
            return m_bytesFormat;
        }

        int getNumMipMaps() {
            return m_numMipMaps;
        }

        int getNumFaces() {
            return m_numFaces;
        }

        uint8* getBytes() {
            return m_bytes;
        }
    };
};


} // namespace

template <> struct HashTrait<G3D::Texture::Settings> {
    static size_t hashCode(const G3D::Texture::Settings& key) { return key.hashCode(); }
};


#endif

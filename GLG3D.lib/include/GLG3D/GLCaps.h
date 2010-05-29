/**
 @file GLCaps.h

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2004-03-28
 @edited  2009-12-31

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_GLCaps_h
#define G3D_GLCaps_h

#include "G3D/platform.h"
#include "G3D/Set.h"
#include "GLG3D/glheaders.h"
#include "G3D/NetworkDevice.h"
#include <string>

namespace G3D {

/**
 Low-level wrapper for OpenGL extension management.
 Can be used without G3D::RenderDevice to load and
 manage extensions.

 OpenGL has a base API and an extension API.  All OpenGL drivers
 must support the base API.  The latest features may not 
 be supported by some drivers, so they are in the extension API
 and are dynamically loaded at runtime using GLCaps::loadExtensions.  
 Before using a specific extension you must test for its presence
 using the GLCaps::supports method.
 
 For convenience, frequently used extensions have fast tests, e.g.,
 GLCaps::supports_GL_EXT_texture_rectangle.

 Note that GL_NV_texture_rectangle and GL_EXT_texture_rectangle
 have exactly the same constants, so supports_GL_EXT_texture_rectangle
 returns true if either is supported.

 GLCaps assumes all OpenGL contexts have the same capabilities.

  The following extensions are shortcutted with a method named
  similarly to GLCaps::supports_GL_EXT_texture_rectangle():

  <UL>
    <LI>GL_ARB_texture_float
    <LI>GL_ARB_texture_non_power_of_two
    <LI>GL_EXT_texture_rectangle
    <LI>GL_ARB_vertex_program
    <LI>GL_NV_vertex_program2
    <LI>GL_ARB_vertex_buffer_object
    <LI>GL_ARB_fragment_program
    <LI>GL_ARB_multitexture
    <LI>GL_EXT_texture_edge_clamp
    <LI>GL_ARB_texture_border_clamp
    <LI>GL_EXT_texture_r
    <LI>GL_EXT_stencil_wrap
    <LI>GL_EXT_stencil_two_side
    <LI>GL_ATI_separate_stencil
    <LI>GL_EXT_texture_compression_s3tc
    <LI>GL_EXT_texture_cube_map, GL_ARB_texture_cube_map
    <LI>GL_ARB_shadow
    <LI>GL_ARB_shader_objects
    <LI>GL_ARB_shading_language_100
    <LI>GL_ARB_fragment_shader
    <LI>GL_ARB_vertex_shader
    <LI>GL_EXT_geometry_shader4
    <LI>GL_EXT_framebuffer_object
    <LI>GL_ARB_framebuffer_object
    <LI>GL_ARB_frambuffer_sRGB
    <LI>GL_SGIS_generate_mipmap
    <LI>GL_EXT_texture_mirror_clamp
	</UL>

  These methods do not appear in the documentation because they
  are generated using macros.

  The <code>hasBug_</code> methods detect specific common bugs on
  graphics cards.  They can be used to switch to fallback rendering
  paths.
 */
class GLCaps {
public:

    enum Vendor {ATI, NVIDIA, MESA, ARB};

private:

    /** True when init has been called */
    static bool         m_initialized;

    /** True when loadExtensions has already been called */
    static bool         m_loadedExtensions;

    /** True if this is GL 2.0 or greater, which mandates certain extensions.*/
    static bool         m_hasGLMajorVersion2;

    /** True if this is GL 3.0 or greater, which mandates certain extensions.*/
    static bool         m_hasGLMajorVersion3;

    /** True when checkAllBugs has been called. */
    static bool         m_checkedForBugs;

    static float        m_glslVersion;

    static int          m_numTextureCoords;
    static int          m_numTextures;
    static int          m_numTextureUnits;

    static int          m_maxTextureSize;
    static int          m_maxCubeMapSize;

    static Vendor computeVendor();

    /**
       Returns the version string for the video driver
       for MESA or Windows drivers.
    */
    static std::string getDriverVersion();
  
 // We're going to need exactly the same code for each of 
 // several extensions, so we abstract the boilerplate into
 // a macro that declares a private variable and public accessor.
#define DECLARE_EXT(extname)                    \
private:                                        \
    static bool     _supports_##extname;        \
public:                                         \
    static bool inline supports_##extname() {   \
        return _supports_##extname;             \
    }                                           \
private:

    // New extensions must be added in 3 places: 1. here.
    // 2. at the top of GLCaps.cpp.  3. underneath the LOAD_EXTENSION code.
    DECLARE_EXT(GL_ARB_texture_float);
    DECLARE_EXT(GL_ARB_texture_non_power_of_two);
    DECLARE_EXT(GL_EXT_texture_rectangle);
    DECLARE_EXT(GL_ARB_vertex_program);
    DECLARE_EXT(GL_NV_vertex_program2);
    DECLARE_EXT(GL_ARB_vertex_buffer_object);
    DECLARE_EXT(GL_ARB_fragment_program);
    DECLARE_EXT(GL_ARB_multitexture);
    DECLARE_EXT(GL_EXT_texture_edge_clamp);
    DECLARE_EXT(GL_ARB_texture_border_clamp);
    DECLARE_EXT(GL_EXT_texture3D);
    DECLARE_EXT(GL_EXT_stencil_wrap);
    DECLARE_EXT(GL_EXT_separate_specular_color);
    DECLARE_EXT(GL_EXT_stencil_two_side);
    DECLARE_EXT(GL_ATI_separate_stencil);    
    DECLARE_EXT(GL_EXT_texture_compression_s3tc);
    DECLARE_EXT(GL_EXT_texture_cube_map);
    DECLARE_EXT(GL_ARB_shadow);
    DECLARE_EXT(GL_ARB_shader_objects);
    DECLARE_EXT(GL_ARB_shading_language_100);
    DECLARE_EXT(GL_ARB_fragment_shader);
    DECLARE_EXT(GL_ARB_vertex_shader);
    DECLARE_EXT(GL_EXT_geometry_shader4);
    DECLARE_EXT(GL_EXT_framebuffer_object);
    DECLARE_EXT(GL_ARB_framebuffer_object);
    DECLARE_EXT(GL_ARB_framebuffer_sRGB);
    DECLARE_EXT(GL_SGIS_generate_mipmap);
    DECLARE_EXT(GL_EXT_texture_mirror_clamp);
    
#undef DECLARE_EXT

    static Set<std::string>         extensionSet;

    static bool bug_glMultiTexCoord3fvARB;
    static bool bug_normalMapTexGen;
    static bool bug_redBlueMipmapSwap;
    static bool bug_mipmapGeneration;
    static bool bug_slowVBO;

    /** Tests for hasBug_glMultiTexCoord3fvARB and hasBug_glNormalMapTexGenARB */
    static void checkBug_cubeMapBugs();
    static void checkBug_redBlueMipmapSwap();
    static void checkBug_mipmapGeneration();
    static void checkBug_slowVBO();

    /** Runs all of the checkBug_ methods. Called from loadExtensions(). */
    static void checkAllBugs();

    /** Loads OpenGL extensions (e.g. glBindBufferARB).
    Call this once at the beginning of the program,
    after a video device is created.  This is called
    for you if you use G3D::RenderDevice.    
    */
    static void loadExtensions(class Log* debugLog = NULL);

public:
    /** Loads OpenGL extensions (e.g. glBindBufferARB).
        Call this once at the beginning of the program,
        after a video device is created.  This is called
        for you if you use G3D::RenderDevice.*/
    static void init();

    static bool supports(const std::string& extName);

    /** @deprecated.  Call GLCaps::supportsTexture instead. */
    static bool G3D_DEPRECATED supports(const class ImageFormat* fmt);

    /** Returns true if the given texture format is supported on this device for Textures.*/
    static bool supportsTexture(const class ImageFormat* fmt);

    /** Returns the first element of \a prefs for which supportsTexture() returns true. Returns NULL if none are supported.*/
    static const class ImageFormat* firstSupportedTexture(const Array<const class ImageFormat*>& prefs);

    /** Returns true if the given texture format is supported on this device for RenderBuffers.*/
    static bool supportsRenderBuffer(const class ImageFormat* fmt);

    static const std::string& glVersion();

    static const std::string& driverVersion();

    /** e.g., 1.50 or 4.00 */
    static const float glslVersion() {
        return m_glslVersion;
    }

    static const std::string& vendor();

    static Vendor enumVendor();

    /** 
        Returns true if this GPU/driver supports the features needed
        for the future G3D 9.00 release, which raises the minimum
        standards for GPUs.  This call is intended to give developers
        some guidance in what to expect from the new API, however, it
        is not guaranteed to match the G3D 9.00 specification and
        requirements because that API is still under design.

        \param explanation Detailed explanation of which extensions
        are needed.
     */
    static bool supportsG3D9(std::string& explanation);

    static const std::string& renderer();

    /** Returns true if either GL_EXT_stencil_two_side or GL_ATI_separate_stencil is supported.
        Convenient becaused G3D::RenderDevice unifies those extensions. */
    static bool supports_two_sided_stencil();
 
    /** Between 8 and 16 on most cards.  Can be more than number of textures. */
    static int numTextureCoords() {
        return m_numTextureCoords;
    }

    /** Between 16 and 32 on most cards. Can be more than number of fixed-function texture units. */
    static int numTextures() {
        return m_numTextures;
    }

    /** 4 on most cards. Only affects fixed function. See http://developer.nvidia.com/object/General_FAQ.html#t6  for a discussion of the number of texture units.*/
    static int numTextureUnits() {
        return m_numTextureUnits;
    }

    static int maxTextureSize() {
        return m_maxTextureSize;
    }

    static int maxCubeMapSize() {
        return m_maxCubeMapSize;
    }

    static inline bool supports_GL_ARB_texture_cube_map() {
        return supports_GL_EXT_texture_cube_map();
    }

    /**
     Returns true if cube map support has a specific known bug on this card.
     Returns false if cube maps are not supported at all on this card.

     Call after OpenGL is intialized.  Will render on the backbuffer but not make
     the backbuffer visible.  Safe to call multiple times; the result is memoized.

     On some Radeon Mobility cards (up to Mobility 9200), glMultiTexCoord3fvARB and glVertex4fv together
     create incorrect texture lookups from cube maps.  Using glVertex3fv or glTexCoord
     with glActiveTextureARB avoids this problem, as does using normal map generation.
     */
    static bool hasBug_glMultiTexCoord3fvARB();

    /** Some ATI cards claim to support ImageFormat::R11G10B10F but render to it incorrectly. */
    static bool hasBug_R11G10B10F();

    /**
     Returns true if cube map support has a specific known bug on this card that
     prevents correct normal map coordinate generation, i.e.,
     <code>glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB)</code> 
     does not function as specified by OpenGL.

     Returns false if cube maps are not supported at all on this card.

     Call after OpenGL is intialized.  Will render on the backbuffer but not make
     the backbuffer visible.  Safe to call multiple times; the result is memoized.

     Radeon Mobility 7500 has been shown to have a bug where not only does
     hasBug_glMultiTexCoord3fvARB() exist, but normal maps can't work around 
     the problem.  Certain NVIDIA 280 Linux drivers may also have this bug.

     If detected, G3D::Sky will revert to non-cube map textures.
     */
    static bool hasBug_normalMapTexGen();

    /**
    Radeon mobility 7500 occasionally flips the red and blue channels
    when auto-generating mipmaps.  This has proven to be a reliable test
    for this bug. 

    If this bug is detected, G3D::Texture switches to RGBA8 formats for
    RGB8 data.
    */
    static bool hasBug_redBlueMipmapSwap();

    /**
      Returns true if SGIS auto mip-map generation occasionally
      produces buggy results (usually, pieces of other textures in
      the low-level mipmaps).

      Radeon Mobility 9200 has this bug for some drivers.

      If this bug is detected, G3D::Texture reverts to software mipmap generation.
     */
    static bool hasBug_mipmapGeneration();

    /**
	 Some graphics cards (e.g. Radeon Mobility 7500) support the VBO extension
	 but it is slower than main memory in most cases due to poor cache behavior.
	 This method performs a speed test the first time it is invoked and identifies
	 those cards.
	*/
	static bool hasBug_slowVBO();

};


/**
 Prints a human-readable description of this machine
 to the text output stream.  Either argument may be NULL.
 */
void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    class TextOutput& t);

void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    std::string&        s);

} // namespace

#endif

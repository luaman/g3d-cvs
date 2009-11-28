/**
  @file GLCaps.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2004-03-28
  @edited  2009-11-10
*/

#include "G3D/TextOutput.h"
#include "G3D/RegistryUtil.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/glcalls.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/NetworkDevice.h"
#include "G3D/Log.h"

#include <sstream>

namespace G3D {

// Global init flags for GLCaps.  Because this is an integer constant (equal to zero),
// we can safely assume that it will be initialized before this translation unit is
// entered.
bool GLCaps::_loadedExtensions = false;
bool GLCaps::_initialized = false;
bool GLCaps::_checkedForBugs = false;
bool GLCaps::_hasGLMajorVersion2 = false;

int GLCaps::_numTextureCoords = 0;
int GLCaps::_numTextures = 0;
int GLCaps::_numTextureUnits = 0;

bool GLCaps::bug_glMultiTexCoord3fvARB = false;
bool GLCaps::bug_normalMapTexGen = false;
bool GLCaps::bug_redBlueMipmapSwap = false;
bool GLCaps::bug_mipmapGeneration = false;
bool GLCaps::bug_slowVBO = false;

int GLCaps::_maxTextureSize = 0;
int GLCaps::_maxCubeMapSize = 0;

/**
 Dummy function to which unloaded extensions can be set.
 */
static void __stdcall glIgnore(GLenum e) {
    (void)e;
}

/** Cache of values supplied to supportsImageFormat.
    Works on pointers since there is no way for users
    to construct their own ImageFormats.
 */
static Table<const ImageFormat*, bool>& _supportedImageFormat() {
    static Table<const ImageFormat*, bool> cache;
    return cache;
}


static Table<const ImageFormat*, bool>& _supportedRenderBufferFormat() {
    static Table<const ImageFormat*, bool> cache;
    return cache;
}

Set<std::string> GLCaps::extensionSet;

GLCaps::Vendor GLCaps::computeVendor() {
    std::string s = vendor();

    if (s == "ATI Technologies Inc.") {
        return ATI;
    } else if (s == "NVIDIA Corporation") {
        return NVIDIA;
    } else if ((s == "Brian Paul") || (s == "Mesa project: www.mesa3d.org")) {
        return MESA;
    } else {
        return ARB;
    }
}

GLCaps::Vendor GLCaps::enumVendor() {
    return computeVendor();
}


#ifdef G3D_WIN32
/**
 Used by the Windows version of getDriverVersion().
 @cite Based on code by Ted Peck tpeck@roundwave.com http://www.codeproject.com/dll/ShowVer.asp
 */
struct VS_VERSIONINFO { 
    WORD                wLength; 
    WORD                wValueLength; 
    WORD                wType; 
    WCHAR               szKey[1]; 
    WORD                Padding1[1]; 
    VS_FIXEDFILEINFO    Value; 
    WORD                Padding2[1]; 
    WORD                Children[1]; 
};
#endif

std::string GLCaps::getDriverVersion() {
    if (computeVendor() == MESA) {
        // Mesa includes the driver version in the renderer version
        // e.g., "1.5 Mesa 6.4.2"
        
        static std::string _glVersion = (char*)glGetString(GL_VERSION);
        int i = _glVersion.rfind(' ');
        if (i == (int)std::string::npos) {
            return "Unknown (bad MESA driver string)";
        } else {
            return _glVersion.substr(i + 1, _glVersion.length() - i);
        }
    }
    
#ifdef G3D_WIN32
 
    // locate the driver on Windows and get the version
    // this systems expects Windows 2000/XP/Vista
    std::string videoDriverKey;

    bool canCheckVideoDevice = RegistryUtil::keyExists("HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\VIDEO");

    if (canCheckVideoDevice) {

        // find the driver expected to load
        std::string videoDeviceKey = "HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\VIDEO";
        std::string videoDeviceValue = "\\Device\\Video";
        int videoDeviceNum = 0;

        while (RegistryUtil::valueExists(videoDeviceKey, format("%s%d", videoDeviceValue.c_str(), videoDeviceNum))) {
            ++videoDeviceNum;
        }

        // find the key where the installed driver lives
        std::string installedDriversKey;
        RegistryUtil::readString(videoDeviceKey, format("%s%d", videoDeviceValue.c_str(), videoDeviceNum - 1), installedDriversKey);

        // find and remove the "\Registry\Machine\" part of the key
        int subKeyIndex = installedDriversKey.find('\\', 1);
        subKeyIndex = installedDriversKey.find('\\', subKeyIndex + 1);

        installedDriversKey.erase(0, subKeyIndex);

        // read the list of driver files this display driver installed/loads
        // this is a multi-string value, but we only care about the first entry so reading one string is fine
        
        std::string videoDrivers;
        RegistryUtil::readString("HKEY_LOCAL_MACHINE" + installedDriversKey, "InstalledDisplayDrivers", videoDrivers);

        if (videoDrivers.find(',', 0) != std::string::npos) {
            videoDrivers = videoDrivers.substr(0, videoDrivers.find(',', 0));
        }

        char systemDirectory[512] = "";
        GetSystemDirectoryA(systemDirectory, sizeof(systemDirectory));

        std::string driverFileName = format("%s\\%s.dll", systemDirectory, videoDrivers.c_str());

        DWORD dummy;
        int size = GetFileVersionInfoSizeA((LPCSTR)driverFileName.c_str(), &dummy);
        if (size == 0) {
            return "Unknown (Can't find driver)";
        }

        void* buffer = new uint8[size];

        if (GetFileVersionInfoA((LPCSTR)driverFileName.c_str(), 0, size, buffer) == 0) {
            delete[] (uint8*)buffer;
            return "Unknown (Can't find driver)";
        }

	    // Interpret the VS_VERSIONINFO header pseudo-struct
	    VS_VERSIONINFO* pVS = (VS_VERSIONINFO*)buffer;
        debugAssert(! wcscmp(pVS->szKey, L"VS_VERSION_INFO"));

	    uint8* pVt = (uint8*) &pVS->szKey[wcslen(pVS->szKey) + 1];

        #define roundoffs(a,b,r)	(((uint8*)(b) - (uint8*)(a) + ((r) - 1)) & ~((r) - 1))
        #define roundpos(b, a, r)	(((uint8*)(a)) + roundoffs(a, b, r))

	    VS_FIXEDFILEINFO* pValue = (VS_FIXEDFILEINFO*) roundpos(pVt, pVS, 4);

        #undef roundoffs
        #undef roundpos

        std::string result = "Unknown (Can't find driver)";

	    if (pVS->wValueLength) {
	        result = format("%d.%d.%d.%d",
                pValue->dwProductVersionMS >> 16,
                pValue->dwProductVersionMS & 0xFFFF,
	            pValue->dwProductVersionLS >> 16,
                pValue->dwProductVersionLS & 0xFFFF);
        }

        delete[] (uint8*)buffer;

        return result;

    } else {
        return "Unknown (Can't find driver)";
    }

    #else
        return "Unknown";
    #endif
}

void GLCaps::init() {
    loadExtensions(Log::common());

    checkAllBugs();
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
}

// We're going to need exactly the same code for each of 
// several extensions.
#define DECLARE_EXT(extname)    bool GLCaps::_supports_##extname = false; 
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
    DECLARE_EXT(GL_EXT_stencil_two_side);
    DECLARE_EXT(GL_ATI_separate_stencil);    
    DECLARE_EXT(GL_EXT_texture_compression_s3tc);
    DECLARE_EXT(GL_EXT_texture_cube_map);
    DECLARE_EXT(GL_EXT_separate_specular_color);
    DECLARE_EXT(GL_ARB_shadow);
    DECLARE_EXT(GL_ARB_shader_objects);
    DECLARE_EXT(GL_ARB_shading_language_100);
    DECLARE_EXT(GL_ARB_fragment_shader);
    DECLARE_EXT(GL_ARB_vertex_shader);
    DECLARE_EXT(GL_EXT_geometry_shader4);
    DECLARE_EXT(GL_EXT_framebuffer_object);
    DECLARE_EXT(GL_SGIS_generate_mipmap);
#undef DECLARE_EXT

void GLCaps::loadExtensions(Log* debugLog) {
    // This is here to prevent a spurrious warning under gcc
    glIgnore(0);

    debugAssert(glGetString(GL_VENDOR) != NULL);

    if (_loadedExtensions) {
        return;
    } else {
        _loadedExtensions = true;
    }

    alwaysAssertM(! _initialized, "Internal error.");

    // Require an OpenGL context to continue
    alwaysAssertM(glGetCurrentContext(), "Unable to load OpenGL extensions without a current context.");

    // Initialize statically cached strings
	vendor();
    renderer();
    glVersion();
    driverVersion();

    // Initialize cached GL major version pulled from glVersion() for extensions made into 2.0 core
    const std::string glver = glVersion();
    _hasGLMajorVersion2 = beginsWith(glver, "2.");

    // Turn on OpenGL 3.0
    glewExperimental = GL_TRUE;

    GLenum err = glewInit();
    alwaysAssertM(err == GLEW_OK, format("Error Initializing OpenGL Extensions (GLEW): %s\n", glewGetErrorString(err)));

    std::istringstream extensions;
    std::string extStringCopy = (char*)glGetString(GL_EXTENSIONS);
    extensions.str(extStringCopy.c_str());
    {
        // Parse the extensions into the supported set
        std::string s;
        while (extensions >> s) {
            extensionSet.insert(s);
        }

        // We're going to need exactly the same code for each of 
        // several extensions.
#       define DECLARE_EXT(extname) _supports_##extname = supports(#extname)
#       define DECLARE_EXT_GL2(extname) _supports_##extname = (supports(#extname) || _hasGLMajorVersion2)
            DECLARE_EXT(GL_ARB_texture_float);
            DECLARE_EXT_GL2(GL_ARB_texture_non_power_of_two);
            DECLARE_EXT(GL_EXT_texture_rectangle);
            DECLARE_EXT(GL_ARB_vertex_program);
            DECLARE_EXT(GL_NV_vertex_program2);
            DECLARE_EXT(GL_ARB_vertex_buffer_object);
            DECLARE_EXT(GL_EXT_texture_edge_clamp);
            DECLARE_EXT_GL2(GL_ARB_texture_border_clamp);
            DECLARE_EXT(GL_EXT_texture3D);
            DECLARE_EXT_GL2(GL_ARB_fragment_program);
            DECLARE_EXT_GL2(GL_ARB_multitexture);
            DECLARE_EXT_GL2(GL_EXT_separate_specular_color);
            DECLARE_EXT(GL_EXT_stencil_wrap);
            DECLARE_EXT(GL_EXT_stencil_two_side);
            DECLARE_EXT(GL_ATI_separate_stencil);            
            DECLARE_EXT(GL_EXT_texture_compression_s3tc);
            DECLARE_EXT(GL_EXT_texture_cube_map);
            DECLARE_EXT_GL2(GL_ARB_shadow);
            DECLARE_EXT_GL2(GL_ARB_shader_objects);
            DECLARE_EXT_GL2(GL_ARB_shading_language_100);
            DECLARE_EXT(GL_ARB_fragment_shader);
            DECLARE_EXT(GL_ARB_vertex_shader);
            DECLARE_EXT(GL_EXT_geometry_shader4);
            DECLARE_EXT_GL2(GL_EXT_framebuffer_object);
            DECLARE_EXT(GL_SGIS_generate_mipmap);
#       undef DECLARE_EXT

        // Some extensions have aliases
         _supports_GL_EXT_texture_cube_map = _supports_GL_EXT_texture_cube_map || supports("GL_ARB_texture_cube_map");
         _supports_GL_EXT_texture_edge_clamp = _supports_GL_EXT_texture_edge_clamp || supports("GL_SGIS_texture_edge_clamp");


        // Verify that multitexture loaded correctly
        if (supports_GL_ARB_multitexture() &&
            ((glActiveTextureARB == NULL) ||
            (glMultiTexCoord4fvARB == NULL))) {
            _supports_GL_ARB_multitexture = false;
            #ifdef G3D_WIN32
                *((void**)&glActiveTextureARB) = (void*)glIgnore;
            #endif
        }

        _supports_GL_EXT_texture_rectangle = 
            _supports_GL_EXT_texture_rectangle ||
            supports("GL_NV_texture_rectangle");


        // GL_ARB_texture_cube_map doesn't work on Radeon Mobility
        // GL Renderer:    MOBILITY RADEON 9000 DDR x86/SSE2
        // GL Version:     1.3.4204 WinXP Release
        // Driver version: 6.14.10.6430

        // GL Vendor:      ATI Technologies Inc.
        // GL Renderer:    MOBILITY RADEON 7500 DDR x86/SSE2
        // GL Version:     1.3.3842 WinXP Release
        // Driver version: 6.14.10.6371

        if ((beginsWith(renderer(), "MOBILITY RADEON") || beginsWith(renderer(), "ATI MOBILITY RADEON")) &&
            beginsWith(driverVersion(), "6.14.10.6")) {
            logPrintf("WARNING: This ATI Radeon Mobility card has a known bug with cube maps.\n"
                      "   Put cube map texture coordinates in the normals and use ARB_NORMAL_MAP to work around.\n\n");
        }
    }

    // Don't use more texture units than allowed at compile time.
    if (GLCaps::supports_GL_ARB_multitexture()) {
        _numTextureUnits = iMin(G3D_MAX_TEXTURE_UNITS, 
                                glGetInteger(GL_MAX_TEXTURE_UNITS_ARB));
    } else {
        _numTextureUnits = 1;
    }

    // NVIDIA cards with GL_NV_fragment_program have different 
    // numbers of texture coords, units, and textures
    _numTextureCoords = glGetInteger(GL_MAX_TEXTURE_COORDS_ARB);
    _numTextures = glGetInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB);

    if (! GLCaps::supports_GL_ARB_multitexture()) {
        // No multitexture
        if (debugLog) {
            debugLog->println("No GL_ARB_multitexture support: "
                              "forcing number of texture units "
                              "to no more than 1");
        }
        _numTextureCoords = iMax(1, _numTextureCoords);
        _numTextures      = iMax(1, _numTextures);
        _numTextureUnits  = iMax(1, _numTextureUnits);
    }
    debugAssertGLOk();

    _maxTextureSize = glGetInteger(GL_MAX_TEXTURE_SIZE);
    _maxCubeMapSize = glGetInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT);

    _initialized = true;
}


void GLCaps::checkAllBugs() {
    if (_checkedForBugs) {
        return;
    } else {
        _checkedForBugs = true;
    }

    alwaysAssertM(_loadedExtensions, "Cannot check for OpenGL bugs before extensions are loaded.");

    checkBug_cubeMapBugs();
    checkBug_redBlueMipmapSwap();
    checkBug_mipmapGeneration();
    checkBug_slowVBO();
}


bool GLCaps::hasBug_glMultiTexCoord3fvARB() {
    alwaysAssertM(_initialized, "GLCaps has not been initialized.");
    return bug_glMultiTexCoord3fvARB;
}

bool GLCaps::hasBug_normalMapTexGen() {
    alwaysAssertM(_initialized, "GLCaps has not been initialized.");
    return bug_normalMapTexGen;
}


bool GLCaps::hasBug_redBlueMipmapSwap() {
    alwaysAssertM(_initialized, "GLCaps has not been initialized.");
    return bug_redBlueMipmapSwap;
}


bool GLCaps::hasBug_mipmapGeneration() {
    alwaysAssertM(_initialized, "GLCaps has not been initialized.");
    return bug_mipmapGeneration;
}


bool GLCaps::hasBug_slowVBO() {
    alwaysAssertM(_initialized, "GLCaps has not been initialized.");
    return bug_slowVBO;
}


bool GLCaps::supports(const std::string& extension) {
    return extensionSet.contains(extension);
}


bool GLCaps::supports(const ImageFormat* fmt) {
    return supportsTexture(fmt);
}


bool GLCaps::supportsTexture(const ImageFormat* fmt) {

    // First, check if we've already tested this format
    if (! _supportedImageFormat().containsKey(fmt)) {

        bool supportsFormat = false;

        if (fmt->floatingPoint && ! supports_GL_ARB_texture_float()) {
            supportsFormat = false;
        } else {
            // Allocate some space for making a dummy texture
            uint8 bytes[8 * 8 * 4];

            glPushAttrib(GL_TEXTURE_BIT);
            {
                // Clear the error bit
                glGetError();

                // See if we can create a texture in this format
                unsigned int id;
                glGenTextures(1, &id);
                glBindTexture(GL_TEXTURE_2D, id);

                // Clear the old error flag
                glGetError();
                // 2D texture, level of detail 0 (normal), internal format, x size from image, y size from image, 
                // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
                glTexImage2D(GL_TEXTURE_2D, 0, fmt->openGLFormat, 8, 8, 0, 
                             fmt->openGLBaseFormat, GL_UNSIGNED_BYTE, bytes);

                supportsFormat = (glGetError() == GL_NO_ERROR);

                glBindTexture(GL_TEXTURE_2D, 0);
                glDeleteTextures(1, &id);
            }
            // Restore old texture state
            glPopAttrib();
        }

        _supportedImageFormat().set(fmt, supportsFormat);
    }

    return _supportedImageFormat()[fmt];
}


bool GLCaps::supportsRenderBuffer(const ImageFormat* fmt) {

    // First, check if we've already tested this format
    if (! _supportedRenderBufferFormat().containsKey(fmt)) {

        bool supportsFormat = false;

        if (! supports_GL_EXT_framebuffer_object()) {
            // No frame buffers
            supportsFormat = false;

        } else if (fmt->floatingPoint && ! supports_GL_ARB_texture_float()) {
            supportsFormat = false;
        } else {
            glPushAttrib(GL_COLOR_BUFFER_BIT);
            {
                // Clear the error bit
                glGetError();

                // See if we can create a render buffer in this format
                unsigned int id;
                glGenRenderbuffersEXT (1, &id);

                // Clear the old error flag
                glGetError();

                glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, id);
                glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, fmt->openGLFormat, 8, 8);

                supportsFormat = (glGetError() == GL_NO_ERROR);

                glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, 0);
                glDeleteRenderbuffersEXT(1, &id);
            }
            // Restore old  state
            glPopAttrib();
        }

        _supportedRenderBufferFormat().set(fmt, supportsFormat);
    }

    return _supportedRenderBufferFormat()[fmt];
}


const std::string& GLCaps::glVersion() {
    alwaysAssertM(_loadedExtensions, "Cannot call GLCaps::glVersion before GLCaps::init().");
    static std::string _glVersion = (char*)glGetString(GL_VERSION);
    return _glVersion;
}


const std::string& GLCaps::driverVersion() {
    alwaysAssertM(_loadedExtensions, "Cannot call GLCaps::driverVersion before GLCaps::init().");
    static std::string _driverVersion = getDriverVersion().c_str();
	return _driverVersion;
}


const std::string& GLCaps::vendor() {
    alwaysAssertM(_loadedExtensions, "Cannot call GLCaps::vendor before GLCaps::init().");
    static std::string _driverVendor = (char*)glGetString(GL_VENDOR);
	return _driverVendor;
}


const std::string& GLCaps::renderer() {
    alwaysAssertM(_loadedExtensions, "Cannot call GLCaps::renderer before GLCaps::init().");
    static std::string _glRenderer = (char*)glGetString(GL_RENDERER);
	return _glRenderer;
}

 
bool GLCaps::supports_two_sided_stencil() {
    return 
        GLCaps::supports_GL_ATI_separate_stencil() ||
        GLCaps::supports_GL_EXT_stencil_two_side();
}


void GLCaps::checkBug_cubeMapBugs() {
    bool hasCubeMap = (strstr((char*)glGetString(GL_EXTENSIONS), "GL_EXT_texture_cube_map") != NULL) ||
                      (strstr((char*)glGetString(GL_EXTENSIONS), "GL_ARB_texture_cube_map") != NULL);

    if (! hasCubeMap) {
        bug_glMultiTexCoord3fvARB = false;
        bug_normalMapTexGen = false;
        // No cube map == no bug.
        return;
    }

    // Save current GL state
    unsigned int id;
    glGenTextures(1, &id);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
        glClearColor(0,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        GLenum target[] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB};


        // Face colors
        unsigned char color[6];

        // Create a cube map
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, id);
        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    
        {
            const int N = 16;
            unsigned int image[N * N];
            for (int f = 0; f < 6; ++f) {

                color[f] = f * 40;

                // Fill each face with a different color
                memset(image, color[f], N * N * sizeof(unsigned int));

                // 2D texture, level of detail 0 (normal), internal format, x size from image, y size from image, 
                // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
                glTexImage2D(target[f], 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                debugAssertGLOk();
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP);
        }
    

        // Set orthogonal projection
        float viewport[4];
        glGetFloatv(GL_VIEWPORT, viewport);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(viewport[0], viewport[0] + viewport[2], viewport[1] + viewport[3], viewport[1], -1, 10);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glActiveTextureARB(GL_TEXTURE0_ARB + 0);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);

        // Render one sample from each cube map face
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glColor3f(1, 1, 1);

        static const float corner[] = {
            1.000000, -1.000000, 1.000000,
            1.000000, -1.000000, -1.000000,
            1.000000, 1.000000, -1.000000,
            1.000000, 1.000000, 1.000000,

            -1.000000, 1.000000, 1.000000,
            -1.000000, 1.000000, -1.000000,
            -1.000000, -1.000000, -1.000000,
            -1.000000, -1.000000, 1.000000,

            1.000000, 1.000000, 1.000000,
            1.000000, 1.000000, -1.000000,
            -1.000000, 1.000000, -1.000000,
            -1.000000, 1.000000, 1.000000,

            1.000000, -1.000000, 1.000000,
            -1.000000, -1.000000, 1.000000,
            -1.000000, -1.000000, -1.000000,
            1.000000, -1.000000, -1.000000,

            -1.000000, -1.000000, 1.000000,
            1.000000, -1.000000, 1.000000,
            1.000000, 1.000000, 1.000000,
            -1.000000, 1.000000, 1.000000,

            -1.000000, 1.000000, -1.000000,
            1.000000, 1.000000, -1.000000,
            1.000000, -1.000000, -1.000000,
            -1.000000, -1.000000, -1.000000};

        for (int i = 0; i < 2; ++i) {
            // First time through, use multitex coord
            if (i == 1) {
                // Second time through, use normal map generation
                glActiveTextureARB(GL_TEXTURE0_ARB + 0);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
                glEnable(GL_TEXTURE_GEN_R);
            }

            glBegin(GL_QUADS);

                for (int f = 0; f < 6; ++f) {
                    const float s = 10.0f;

                    glMultiTexCoord3fvARB(GL_TEXTURE0_ARB, corner + 12 * f + 0);
                    glNormal3fv(corner + 12 * f + 0);
                    glVertex4f(f * s, 0, -1, 1);

                    glMultiTexCoord3fvARB(GL_TEXTURE0_ARB, corner + 12 * f + 3);
                    glNormal3fv(corner + 12 * f + 3);
                    glVertex4f(f * s, s, -1, 1);

                    glMultiTexCoord3fvARB(GL_TEXTURE0_ARB, corner + 12 * f + 6);
                    glNormal3fv(corner + 12 * f + 6);
                    glVertex4f((f + 1) * s, s, -1, 1);

                    glMultiTexCoord3fvARB(GL_TEXTURE0_ARB, corner + 12 * f + 9);
                    glNormal3fv(corner + 12 * f + 9);
                    glVertex4f((f + 1) * s, 0, -1, 1);
                }
            glEnd();

            // Read back results
            unsigned int readback[60];
            glReadPixels(0, (int)(viewport[3] - 5), 60, 1, GL_RGBA, GL_UNSIGNED_BYTE, readback);

            // Test result for errors
            bool texbug = false;
            for (int f = 0; f < 6; ++f) {
                if ((readback[f * 10 + 5] & 0xFF) != color[f]) {
                    texbug = true;
                    break;
                }
            }

            if (i == 0) {
                bug_glMultiTexCoord3fvARB = texbug;
            } else {
                bug_normalMapTexGen = texbug;
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

    glPopAttrib();

    glDeleteTextures(1, &id);
}


void GLCaps::checkBug_redBlueMipmapSwap() {
    static bool hasAutoMipmap = false;
    {
        std::string ext = (char*)glGetString(GL_EXTENSIONS);
        hasAutoMipmap = (ext.find("GL_SGIS_generate_mipmap") != std::string::npos);
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    GLuint id;
    glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);

    if (hasAutoMipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    }

    int N = 4 * 4 * 3;
    uint8* bytes = new uint8[N];

    memset(bytes, 0, 4*4*3);
    for (int i = 0; i < N; i += 3) {
        bytes[i] = 0xFF;
    }

    // 2D texture, level of detail 0 (normal), internal format, x size from image, y size from image, 
    // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);

    // Read the data back.
    glGetTexImage(GL_TEXTURE_2D,
			      0,
			      GL_RGB,
			      GL_UNSIGNED_BYTE,
			      bytes);

    // Verify that the data made the round trip correctly
    bug_redBlueMipmapSwap = 
        (bytes[0] != 0xFF) ||
        (bytes[1] != 0x00) ||
        (bytes[2] != 0x00);

    delete[] bytes;

    // Draw to the screen.

    glDeleteTextures(1, &id);
    glPopAttrib();
}

void GLCaps::checkBug_mipmapGeneration() {
    const std::string& r = renderer();

	// The mip-maps are arbitrarily corrupted; we have not yet generated
	// a reliable test for this case.

    bug_mipmapGeneration = 
        GLCaps::supports("GL_SGIS_generate_mipmap") &&
		(beginsWith(r, "MOBILITY RADEON 90") ||
		    beginsWith(r, "MOBILITY RADEON 57") ||
		    beginsWith(r, "Intel 845G") ||
		    beginsWith(r, "Intel 854G"));
}


void GLCaps::checkBug_slowVBO() {
    bool hasVBO = 
        (strstr((char*)glGetString(GL_EXTENSIONS), "GL_ARB_vertex_buffer_object") != NULL) &&
            (glGenBuffersARB != NULL) && 
            (glBufferDataARB != NULL) &&
            (glDeleteBuffersARB != NULL);

    if (! hasVBO) {
		// Don't have VBO; don't have a bug!
        bug_slowVBO = false;
        return;
    }

	const std::string& r = renderer();

    bug_slowVBO = beginsWith(r, "MOBILITY RADEON 7500");
	return;

    // TODO: Make a real test for this case
}

void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    TextOutput& t) {

    System::describeSystem(t);
    rd->describeSystem(t);
    nd->describeSystem(t);
}


void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    std::string&        s) {
    
    TextOutput t;
    describeSystem(rd, nd, t);
    t.commitString(s);
}


const class ImageFormat* GLCaps::firstSupportedTexture(const Array<const class ImageFormat*>& prefs) {
    for (int i = 0; i < prefs.size(); ++i) {
        if (supportsTexture(prefs[i])) {
            return prefs[i];
        }
    }

    return NULL;
}
}


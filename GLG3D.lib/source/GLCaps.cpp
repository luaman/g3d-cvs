/**
  @file GLCaps.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2004-03-28
  @edited  2006-08-10
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
static Table<const ImageFormat*, bool>      _supportedImageFormat;
static Table<const ImageFormat*, bool>      _supportedRenderBufferFormat;

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
        debugAssert(!wcscmp(pVS->szKey, L"VS_VERSION_INFO"));

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
    DECLARE_EXT(GL_ARB_shadow);
    DECLARE_EXT(GL_ARB_shader_objects);
    DECLARE_EXT(GL_ARB_shading_language_100);
    DECLARE_EXT(GL_ARB_fragment_shader);
    DECLARE_EXT(GL_ARB_vertex_shader);
    DECLARE_EXT(GL_EXT_geometry_shader4);
    DECLARE_EXT(GL_EXT_framebuffer_object);
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

#   ifdef __GNUC__
       // Avoid "ISO C++ forbids casting between pointer-to-function and pointer-to-object" error
#      define LOAD_EXTENSION(name, type) \
            name = (type)(glGetProcAddress(#name));
#   else
#      define LOAD_EXTENSION(name, type) \
            name = reinterpret_cast<type>(glGetProcAddress(#name));
#   endif

    // Don't load the multitexture extensions when they are
    // statically linked
    #ifndef G3D_GL_ARB_multitexture_static
        LOAD_EXTENSION(glMultiTexCoord2fARB,    PFNGLMULTITEXCOORD2FARBPROC);
        LOAD_EXTENSION(glMultiTexCoord1fARB,    PFNGLMULTITEXCOORD1FARBPROC);
        LOAD_EXTENSION(glMultiTexCoord2fvARB,   PFNGLMULTITEXCOORD2FVARBPROC);
        LOAD_EXTENSION(glMultiTexCoord3fvARB,   PFNGLMULTITEXCOORD3FVARBPROC);
        LOAD_EXTENSION(glMultiTexCoord4fvARB,   PFNGLMULTITEXCOORD4FVARBPROC);
        LOAD_EXTENSION(glMultiTexCoord1dARB,    PFNGLMULTITEXCOORD1DARBPROC);
        LOAD_EXTENSION(glMultiTexCoord2dvARB,   PFNGLMULTITEXCOORD2DVARBPROC);
        LOAD_EXTENSION(glMultiTexCoord3dvARB,   PFNGLMULTITEXCOORD3DVARBPROC);
        LOAD_EXTENSION(glMultiTexCoord4dvARB,   PFNGLMULTITEXCOORD3DVARBPROC);
        LOAD_EXTENSION(glActiveTextureARB,      PFNGLACTIVETEXTUREARBPROC);
        LOAD_EXTENSION(glClientActiveTextureARB,PFNGLCLIENTACTIVETEXTUREARBPROC);
    #endif

    LOAD_EXTENSION(glBlendEquationEXT, PFNGLBLENDEQUATIONEXTPROC);

    #ifdef G3D_WIN32
        LOAD_EXTENSION(wglSwapIntervalEXT, PFNWGLSWAPINTERVALEXTPROC);
        LOAD_EXTENSION(wglChoosePixelFormatARB, PFNWGLCHOOSEPIXELFORMATARBPROC);
        LOAD_EXTENSION(wglAllocateMemoryNV, PFNWGLALLOCATEMEMORYNVPROC);
        LOAD_EXTENSION(wglFreeMemoryNV, PFNWGLFREEMEMORYNVPROC);
    #endif
    LOAD_EXTENSION(glVertexArrayRangeNV, PFNGLVERTEXARRAYRANGENVPROC);
    LOAD_EXTENSION(glCompressedTexImage2DARB, PFNGLCOMPRESSEDTEXIMAGE2DARBPROC);
    LOAD_EXTENSION(glGetCompressedTexImageARB, PFNGLGETCOMPRESSEDTEXIMAGEARBPROC);
    LOAD_EXTENSION(glGenFencesNV, PFNGLGENFENCESNVPROC);
    LOAD_EXTENSION(glDeleteFencesNV, PFNGLDELETEFENCESNVPROC);
    LOAD_EXTENSION(glSetFenceNV, PFNGLSETFENCENVPROC);
    LOAD_EXTENSION(glFlushVertexArrayRangeNV, PFNGLFLUSHVERTEXARRAYRANGENVPROC);
    LOAD_EXTENSION(glFinishFenceNV, PFNGLFINISHFENCENVPROC);
    LOAD_EXTENSION(glGenProgramsARB, PFNGLGENPROGRAMSARBPROC);
    LOAD_EXTENSION(glBindProgramARB, PFNGLBINDPROGRAMARBPROC);
    LOAD_EXTENSION(glDeleteProgramsARB, PFNGLDELETEPROGRAMSARBPROC);
    LOAD_EXTENSION(glProgramStringARB, PFNGLPROGRAMSTRINGARBPROC);
    LOAD_EXTENSION(glProgramEnvParameter4fARB, PFNGLPROGRAMENVPARAMETER4FARBPROC);
    LOAD_EXTENSION(glProgramLocalParameter4fARB, PFNGLPROGRAMLOCALPARAMETER4FARBPROC);
    LOAD_EXTENSION(glProgramLocalParameter4fvARB, PFNGLPROGRAMLOCALPARAMETER4FVARBPROC);
    LOAD_EXTENSION(glProgramEnvParameter4dvARB, PFNGLPROGRAMENVPARAMETER4DVARBPROC);
    LOAD_EXTENSION(glProgramLocalParameter4dvARB, PFNGLPROGRAMLOCALPARAMETER4DVARBPROC);
    LOAD_EXTENSION(glVertexAttribPointerARB, PFNGLVERTEXATTRIBPOINTERARBPROC);
    LOAD_EXTENSION(glEnableVertexAttribArrayARB, PFNGLENABLEVERTEXATTRIBARRAYARBPROC);
    LOAD_EXTENSION(glDisableVertexAttribArrayARB, PFNGLDISABLEVERTEXATTRIBARRAYARBPROC);
    LOAD_EXTENSION(glPointParameterfARB, PFNGLPOINTPARAMETERFARBPROC);
    LOAD_EXTENSION(glPointParameterfvARB, PFNGLPOINTPARAMETERFVARBPROC);
    LOAD_EXTENSION(glMultiDrawArraysEXT, PFNGLMULTIDRAWARRAYSEXTPROC);
    LOAD_EXTENSION(glMultiDrawElementsEXT, PFNGLMULTIDRAWELEMENTSEXTPROC);
    LOAD_EXTENSION(glCombinerParameterfvNV, PFNGLCOMBINERPARAMETERFVNVPROC);
    LOAD_EXTENSION(glCombinerParameterfNV, PFNGLCOMBINERPARAMETERFNVPROC);
    LOAD_EXTENSION(glCombinerParameterivNV, PFNGLCOMBINERPARAMETERIVNVPROC);
    LOAD_EXTENSION(glCombinerParameteriNV, PFNGLCOMBINERPARAMETERINVPROC);
    LOAD_EXTENSION(glCombinerInputNV, PFNGLCOMBINERINPUTNVPROC);
    LOAD_EXTENSION(glCombinerOutputNV, PFNGLCOMBINEROUTPUTNVPROC);
    LOAD_EXTENSION(glFinalCombinerInputNV, PFNGLFINALCOMBINERINPUTNVPROC);
    LOAD_EXTENSION(glGetCombinerInputParameterfvNV, PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGetCombinerInputParameterivNV, PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC);
    LOAD_EXTENSION(glGetCombinerOutputParameterfvNV, PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGetCombinerOutputParameterivNV, PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC);
    LOAD_EXTENSION(glGetFinalCombinerInputParameterfvNV, PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGetFinalCombinerInputParameterivNV, PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC);
    LOAD_EXTENSION(glCombinerStageParameterfvNV, PFNGLCOMBINERSTAGEPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGetCombinerStageParameterfvNV, PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGenProgramsNV, PFNGLGENPROGRAMSNVPROC);
    LOAD_EXTENSION(glDeleteProgramsNV, PFNGLDELETEPROGRAMSNVPROC);
    LOAD_EXTENSION(glBindProgramNV, PFNGLBINDPROGRAMNVPROC);
    LOAD_EXTENSION(glLoadProgramNV, PFNGLLOADPROGRAMNVPROC);
    LOAD_EXTENSION(glTrackMatrixNV, PFNGLTRACKMATRIXNVPROC);
    LOAD_EXTENSION(glProgramParameter4fvNV, PFNGLPROGRAMPARAMETER4FVNVPROC);
    LOAD_EXTENSION(glActiveStencilFaceEXT, PFNGLACTIVESTENCILFACEEXTPROC);
    LOAD_EXTENSION(glGetProgramParameterfvNV, PFNGLGETPROGRAMPARAMETERFVNVPROC);
    LOAD_EXTENSION(glGetProgramParameterdvNV, PFNGLGETPROGRAMPARAMETERDVNVPROC);
    LOAD_EXTENSION(glBindBufferARB, PFNGLBINDBUFFERARBPROC);
    LOAD_EXTENSION(glDeleteBuffersARB, PFNGLDELETEBUFFERSARBPROC);
    LOAD_EXTENSION(glGenBuffersARB, PFNGLGENBUFFERSARBPROC);
    LOAD_EXTENSION(glIsBufferARB, PFNGLISBUFFERARBPROC);
    LOAD_EXTENSION(glBufferDataARB, PFNGLBUFFERDATAARBPROC);
    LOAD_EXTENSION(glBufferSubDataARB, PFNGLBUFFERSUBDATAARBPROC);
    LOAD_EXTENSION(glGetBufferSubDataARB, PFNGLGETBUFFERSUBDATAARBPROC);
    LOAD_EXTENSION(glMapBufferARB, PFNGLMAPBUFFERARBPROC);
    LOAD_EXTENSION(glUnmapBufferARB, PFNGLUNMAPBUFFERARBPROC);
    LOAD_EXTENSION(glGetBufferParameterivARB, PFNGLGETBUFFERPARAMETERIVARBPROC);
    LOAD_EXTENSION(glGetBufferPointervARB, PFNGLGETBUFFERPOINTERVARBPROC);
    LOAD_EXTENSION(glDrawRangeElements, PFNGLDRAWRANGEELEMENTSPROC);

    LOAD_EXTENSION(glDeleteObjectARB, PFNGLDELETEOBJECTARBPROC);
    LOAD_EXTENSION(glGetHandleARB, PFNGLGETHANDLEARBPROC);
    LOAD_EXTENSION(glDetachObjectARB, PFNGLDETACHOBJECTARBPROC);
    LOAD_EXTENSION(glCreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC);
    LOAD_EXTENSION(glShaderSourceARB, PFNGLSHADERSOURCEARBPROC);
    LOAD_EXTENSION(glCompileShaderARB, PFNGLCOMPILESHADERARBPROC);
    LOAD_EXTENSION(glCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC);
    LOAD_EXTENSION(glAttachObjectARB, PFNGLATTACHOBJECTARBPROC);
    LOAD_EXTENSION(glLinkProgramARB, PFNGLLINKPROGRAMARBPROC);
    LOAD_EXTENSION(glUseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
    LOAD_EXTENSION(glValidateProgramARB, PFNGLVALIDATEPROGRAMARBPROC);
    LOAD_EXTENSION(glUniform1fvARB, PFNGLUNIFORM1FVARBPROC);
    LOAD_EXTENSION(glUniform2fvARB, PFNGLUNIFORM2FVARBPROC);
    LOAD_EXTENSION(glUniform3fvARB, PFNGLUNIFORM3FVARBPROC);
    LOAD_EXTENSION(glUniform4fvARB, PFNGLUNIFORM4FVARBPROC);
    LOAD_EXTENSION(glUniform1iARB, PFNGLUNIFORM1IARBPROC);
    LOAD_EXTENSION(glUniform2iARB, PFNGLUNIFORM2IARBPROC);
    LOAD_EXTENSION(glUniform3iARB, PFNGLUNIFORM3IARBPROC);
    LOAD_EXTENSION(glUniform4iARB, PFNGLUNIFORM4IARBPROC);
    LOAD_EXTENSION(glUniformMatrix2fvARB, PFNGLUNIFORMMATRIX2FVARBPROC);
    LOAD_EXTENSION(glUniformMatrix3fvARB, PFNGLUNIFORMMATRIX3FVARBPROC);
    LOAD_EXTENSION(glUniformMatrix4fvARB, PFNGLUNIFORMMATRIX4FVARBPROC);
    LOAD_EXTENSION(glGetInfoLogARB, PFNGLGETINFOLOGARBPROC);
    LOAD_EXTENSION(glGetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC);
    LOAD_EXTENSION(glGetAttribLocationARB, PFNGLGETATTRIBLOCATIONARBPROC);
    LOAD_EXTENSION(glGetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC);
    LOAD_EXTENSION(glGetActiveUniformARB, PFNGLGETACTIVEUNIFORMARBPROC);

    LOAD_EXTENSION(glGenFragmentShadersATI, PFNGLGENFRAGMENTSHADERSATIPROC);
    LOAD_EXTENSION(glDeleteFragmentShaderATI, PFNGLDELETEFRAGMENTSHADERATIPROC);
    LOAD_EXTENSION(glBindFragmentShaderATI, PFNGLBINDFRAGMENTSHADERATIPROC);
    LOAD_EXTENSION(glBeginFragmentShaderATI, PFNGLBEGINFRAGMENTSHADERATIPROC);
    LOAD_EXTENSION(glEndFragmentShaderATI, PFNGLENDFRAGMENTSHADERATIPROC);
    LOAD_EXTENSION(glPassTexCoordATI, PFNGLPASSTEXCOORDATIPROC);
    LOAD_EXTENSION(glSampleMapATI, PFNGLSAMPLEMAPATIPROC);
    LOAD_EXTENSION(glSetFragmentShaderConstantATI, PFNGLSETFRAGMENTSHADERCONSTANTATIPROC);
    LOAD_EXTENSION(glColorFragmentOp1ATI, PFNGLCOLORFRAGMENTOP1ATIPROC);
    LOAD_EXTENSION(glColorFragmentOp2ATI, PFNGLCOLORFRAGMENTOP2ATIPROC);
    LOAD_EXTENSION(glColorFragmentOp3ATI, PFNGLCOLORFRAGMENTOP3ATIPROC);
    LOAD_EXTENSION(glAlphaFragmentOp1ATI, PFNGLALPHAFRAGMENTOP1ATIPROC);
    LOAD_EXTENSION(glAlphaFragmentOp2ATI, PFNGLALPHAFRAGMENTOP2ATIPROC);
    LOAD_EXTENSION(glAlphaFragmentOp3ATI, PFNGLALPHAFRAGMENTOP3ATIPROC);

    LOAD_EXTENSION(glDrawBuffersARB, PFNGLDRAWBUFFERSARBPROC);
    
    LOAD_EXTENSION(glTexImage3DEXT, PFNGLTEXIMAGE3DEXTPROC);

    LOAD_EXTENSION(glStencilFuncSeparateATI, PFNGLSTENCILFUNCSEPARATEATIPROC);
    LOAD_EXTENSION(glStencilOpSeparateATI, PFNGLSTENCILOPSEPARATEATIPROC);

    #ifdef G3D_WIN32
        // for some reason, this function pointer doesn't use the correct naming convention
        LOAD_EXTENSION(wglEnableGenlockI3D, PFNWGLENABLEGENLOCKI3D);
        LOAD_EXTENSION(wglCreatePbufferEXT, PFNWGLCREATEPBUFFEREXTPROC);
        LOAD_EXTENSION(wglDestroyPbufferEXT, PFNWGLDESTROYPBUFFEREXTPROC);
        
    #elif defined(G3D_LINUX)

        LOAD_EXTENSION(glXJoinSwapGroupNV, PFNGLXJOINSWAPGROUPNV);
        LOAD_EXTENSION(glXBindSwapBarrierNV, PFNGLXBINDSWAPBARRIERNV);
        LOAD_EXTENSION(glXQuerySwapGroupNV, PFNGLXQUERYSWAPGROUPNV);
        LOAD_EXTENSION(glXQueryMaxSwapGroupsNV, PFNGLXQUERYMAXSWAPGROUPSNV);
        LOAD_EXTENSION(glXQueryFrameCountNV, PFNGLXQUERYFRAMECOUNTNV);
        LOAD_EXTENSION(glXResetFrameCountNV, PFNGLXRESETFRAMECOUNTNV);

        LOAD_EXTENSION(glXCreateGLXPbufferSGIX, PFNGLXCREATEGLXPBUFFERSGIX);
        LOAD_EXTENSION(glXDestroyGLXPbufferSGIX, PFNGLXDESTROYGLXPBUFFERSGIX);
        LOAD_EXTENSION(glXQueryGLXPbufferSGIX, PFNGLXQUERYGLXPBUFFERSGIX);
        LOAD_EXTENSION(glXChooseFBConfigSGIX, PFNGLXCHOOSEFBCONFIGSGIX);
        LOAD_EXTENSION(glXCreateContextWithConfigSGIX, PFNGLXCREATECONTEXTWITHCONFIGSGIX);
    #endif

    // Frame buffer object
    LOAD_EXTENSION(glIsRenderBufferEXT, PFNGLISRENDERBUFFEREXTPROC);
    LOAD_EXTENSION(glBindRenderbufferEXT, PFNGLBINDRENDERBUFFEREXTPROC);
    LOAD_EXTENSION(glDeleteRenderbuffersEXT, PFNGLDELETERENDERBUFFERSEXTPROC);
    LOAD_EXTENSION(glGenRenderbuffersEXT, PFNGLGENRENDERBUFFERSEXTPROC);
    LOAD_EXTENSION(glRenderbufferStorageEXT, PFNGLRENDERBUFFERSTORAGEEXTPROC);
    LOAD_EXTENSION(glGetRenderbufferParameterivEXT, PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC);
    LOAD_EXTENSION(glIsFramebufferEXT, PFNGLISFRAMEBUFFEREXTPROC);
    LOAD_EXTENSION(glBindFramebufferEXT, PFNGLBINDFRAMEBUFFEREXTPROC);
    LOAD_EXTENSION(glDeleteFramebuffersEXT, PFNGLDELETEFRAMEBUFFERSEXTPROC);
    LOAD_EXTENSION(glGenFramebuffersEXT, PFNGLGENFRAMEBUFFERSEXTPROC);
    LOAD_EXTENSION(glCheckFramebufferStatusEXT, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC);
    LOAD_EXTENSION(glFramebufferTexture1DEXT, PFNGLFRAMEBUFFERTEXTURE1DEXTPROC);
    LOAD_EXTENSION(glFramebufferTexture2DEXT, PFNGLFRAMEBUFFERTEXTURE2DEXTPROC);
    LOAD_EXTENSION(glFramebufferTexture3DEXT, PFNGLFRAMEBUFFERTEXTURE3DEXTPROC);
    LOAD_EXTENSION(glFramebufferRenderbufferEXT, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC);
    LOAD_EXTENSION(glGetFramebufferAttachmentParameterivEXT, PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC);
    LOAD_EXTENSION(glGenerateMipmapEXT, PFNGLGENERATEMIPMAPEXTPROC);

    // Occlusion query
    LOAD_EXTENSION(glBeginQueryARB, PFNGLBEGINQUERYARBPROC);
    LOAD_EXTENSION(glDeleteQueriesARB, PFNGLDELETEQUERIESARBPROC);
    LOAD_EXTENSION(glEndQueryARB, PFNGLENDQUERYARBPROC);
    LOAD_EXTENSION(glGenQueriesARB, PFNGLGENQUERIESARBPROC);
    LOAD_EXTENSION(glGetQueryivARB, PFNGLGETQUERYIVARBPROC);
    LOAD_EXTENSION(glGetQueryObjectivARB, PFNGLGETQUERYOBJECTIVARBPROC);
    LOAD_EXTENSION(glGetQueryObjectuivARB, PFNGLGETQUERYOBJECTUIVARBPROC);
    LOAD_EXTENSION(glIsQueryARB, PFNGLISQUERYARBPROC);

    // Depth bounds
    LOAD_EXTENSION(glDepthBoundsEXT, PFNGLDEPTHBOUNDSEXTPROC);

    #undef LOAD_EXTENSION

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
        #define DECLARE_EXT(extname) _supports_##extname = supports(#extname)
        #define DECLARE_EXT_GL2(extname) _supports_##extname = (supports(#extname) || _hasGLMajorVersion2)
            DECLARE_EXT(GL_ARB_texture_float);
            DECLARE_EXT_GL2(GL_ARB_texture_non_power_of_two);
            DECLARE_EXT(GL_EXT_texture_rectangle);
            DECLARE_EXT(GL_ARB_vertex_program);
            DECLARE_EXT(GL_NV_vertex_program2);
            DECLARE_EXT(GL_ARB_vertex_buffer_object);
            DECLARE_EXT(GL_EXT_texture_edge_clamp);
            DECLARE_EXT(GL_ARB_texture_border_clamp);
            DECLARE_EXT(GL_EXT_texture3D);
            DECLARE_EXT(GL_ARB_fragment_program);
            DECLARE_EXT(GL_ARB_multitexture);
            DECLARE_EXT(GL_EXT_stencil_wrap);
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
        #undef DECLARE_EXT

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
            Log::common()->printf("WARNING: This ATI Radeon Mobility card has a known bug with cube maps.\n"
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
    if (GLCaps::supports("GL_NV_fragment_program")) {
        glGetIntegerv(GL_MAX_TEXTURE_COORDS_NV, &_numTextureCoords);
        _numTextureCoords = iClamp(_numTextureCoords,
                                   _numTextureUnits,
                                   G3D_MAX_TEXTURE_UNITS);

        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_NV, &_numTextures);
        _numTextures = iClamp(_numTextures,
                              _numTextureUnits, 
                              G3D_MAX_TEXTURE_UNITS);
    } else {
        _numTextureCoords = _numTextureUnits;
        _numTextures      = _numTextureUnits;
    }

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
    if (! _supportedImageFormat.containsKey(fmt)) {

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

        _supportedImageFormat.set(fmt, supportsFormat);
    }

    return _supportedImageFormat[fmt];
}


bool GLCaps::supportsRenderBuffer(const ImageFormat* fmt) {

    // First, check if we've already tested this format
    if (! _supportedRenderBufferFormat.containsKey(fmt)) {

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

        _supportedRenderBufferFormat.set(fmt, supportsFormat);
    }

    return _supportedRenderBufferFormat[fmt];
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


#if 0
/** Called from hasBug_slowVBO */
static void configureCameraAndLights() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float w = 0.8, h = 0.6;
    glFrustum(-w/2, w/2, -h/2, h/2, 0.5, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glShadeModel(GL_SMOOTH);

    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    {
        float pos[4] = {-1.0, 1.0, 1.0, 0.0};
        glLightfv(GL_LIGHT0, GL_POSITION, pos);

        float col[4] = {1.0, 1.0, 1.0, 1.0};
        glLightfv(GL_LIGHT0, GL_DIFFUSE, col);

    }

    glEnable(GL_LIGHT1);
    {
        float pos[4] = {1.0, -1.0, 1.0, 0.0};
        glLightfv(GL_LIGHT1, GL_POSITION, pos);

        float col[4] = {0.4, 0.1, 0.1, 1.0};
        glLightfv(GL_LIGHT1, GL_DIFFUSE, col);
    }

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    {
        float amb[4] = {0.5, 0.5, 0.5, 1.0};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    }

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}
#endif


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
#if 0
    USE_TEMPORARY_CONTEXT;

    // Load the vertex arrays.  It is important to create a reasonably coherent object;
    // random triangles are a pathological case for the graphics card and will produce
    // poor rendering performance.

    // Vertices per side
    const int sq = 187;

    // Number of indices
    const int N = (sq - 1) * (sq - 1) * 3 * 2;

    // Number of vertices
    const int V = sq * sq;

    // Make a grid of triangles
    std::vector<int> index(N);
    {
        int k = 0;
        for (int i = 0; i < sq - 1; ++i) {
            for (int j = 0; j < sq - 1; ++j) {
                debugAssert(k < N - 5);

                // Bottom triangle
                index[k + 0] = i + j * sq;
                index[k + 1] = i + (j + 1) * sq;
                index[k + 2] = (i + 1) + (j + 1) * sq;

                // Top triangle
                index[k + 3] = i + j * sq;
                index[k + 4] = (i + 1) + (j + 1) * sq;
                index[k + 5] = (i + 1) + j * sq;

                k += 6;
            }
        }
    }

    // Create data
    std::vector<float> vertex(V * 3), normal(V * 3), texCoord(V * 2), color(V * 4);

    // Map V indices to a sq x sq grid
    for (int i = 0; i < sq; ++i) {
        for (int j = 0; j < sq; ++j) {

            int v = (i + j * sq) * 3;
            float x = (i / (float)sq - 0.5) * 2;
            float y = 0.5 - j / (float)sq;
            float a = x * 2 * 3.1415927;
            float r = ceil(cos(a * 10)) * 0.05 + 0.3; 
            vertex[v + 0] = -cos(a) * r;
            vertex[v + 1] = y;
            vertex[v + 2] = sin(a) * r;

            // Scale the normal
            float s = 1.0 / sqrt(0.0001 + square(vertex[v]) + square(vertex[v + 1]) + square(vertex[v + 2]));
            normal[v] = vertex[v] * s;
            normal[v + 1] = vertex[v + 1] * s;
            normal[v + 2] = vertex[v + 2] * s;

            v = (i + j * sq) * 4;
            color[v + 0] = r + 0.7;
            color[v + 1] = 0.5;
            color[v + 2] = 1.0 - r;
            color[v + 3] = 1.0;


            v = (i + j * sq) * 2;
            texCoord[v] = i / (float)sq;
            texCoord[v + 1] = j / (float)sq;
        }
    }

    // number of objects to draw
    const int count = 4;
    const int frames = 15;
    double t0 = 0;

    size_t vertexSize   = V * sizeof(float) * 3;
    size_t normalSize   = V * sizeof(float) * 3;
    size_t colorSize    = V * sizeof(float) * 4;
    size_t texCoordSize = V * sizeof(float) * 2;
    size_t totalSize    = vertexSize + normalSize + texCoordSize + colorSize;

    size_t indexSize    = N * sizeof(int);

    debugAssert(frames >= 2);

    // Time for each rendering method
    double VBOTime;
    double RAMTime;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_ALL_CLIENT_ATTRIB_BITS);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_ALL_CLIENT_ATTRIB_BITS);

        GLuint vbo, indexBuffer;
        glGenBuffersARB(1, &vbo);
        glGenBuffersARB(1, &indexBuffer);

        // Pointers relative to the start of the vbo in video memory
        GLintptrARB vertexPtr   = 0;
        GLintptrARB normalPtr   = vertexSize + vertexPtr;
        GLintptrARB texCoordPtr = normalSize  + normalPtr;
        GLintptrARB colorPtr    = texCoordSize + texCoordPtr;

        GLintptrARB indexPtr    = 0;

        // Upload data
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexBuffer);
        glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexSize, &index[0], GL_STATIC_DRAW_ARB);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, totalSize, NULL, GL_STATIC_DRAW_ARB);

        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, texCoordPtr, texCoordSize, &texCoord[0]);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, normalPtr,   normalSize,   &normal[0]);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, colorPtr,    colorSize,    &color[0]);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vertexPtr,   vertexSize,   &vertex[0]);
    
        {
            float k = 0;
            configureCameraAndLights();
            glDisable(GL_TEXTURE_2D);
            glClearColor(1.0f, 1.0f, 1.0f, 0.04f);
            glColor3f(1, .5, 0);
            glFinish();
            System::sleep(0.05);
            for (int j = 0; j < frames + 1; ++j) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                // Don't count the first frame against us; it is cache warmup
                if (j == 1) {
                    t0 = System::time();
                }
                k += 3;

                glEnableClientState(GL_NORMAL_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glEnableClientState(GL_VERTEX_ARRAY);

                glNormalPointer(GL_FLOAT, 0, (void*)normalPtr);
                glTexCoordPointer(2, GL_FLOAT, 0, (void*)texCoordPtr);
                glColorPointer(4, GL_FLOAT, 0, (void*)colorPtr);
                glVertexPointer(3, GL_FLOAT, 0, (void*)vertexPtr);

                for (int c = 0; c < count; ++c) {
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();
                    glTranslatef(c - (count - 1) / 2.0, 0, -2);
                    glRotatef(k * ((c & 1) * 2 - 1) + 90, 0, 1, 0);

                    glDrawElements(GL_TRIANGLES, N, GL_UNSIGNED_INT, (void*)indexPtr);
                }
                //RenderDevice::lastRenderDeviceCreated->window()->swapGLBuffers();
                glFlush();
            }
            glFinish();
            VBOTime = System::time() - t0;
        }

    glPopClientAttrib();
    glPopAttrib();

    glDeleteBuffersARB(1, &indexBuffer);
    glDeleteBuffersARB(1, &vbo);

    glPushClientAttrib(GL_ALL_CLIENT_ATTRIB_BITS);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    {
        float k = 0;
        configureCameraAndLights();
        glDisable(GL_TEXTURE_2D);
        glClearColor(1.0f, 1.0f, 1.0f, 0.04f);
        glColor3f(1, .5, 0);
        glFinish();
        System::sleep(0.05);

        for (int j = 0; j < frames; ++j) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // Don't count the first frame against us; it is cache warmup
            if (j == 1) {
                t0 = System::time();
            }
            k += 3;

            glEnableClientState(GL_NORMAL_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_VERTEX_ARRAY);

            glNormalPointer(GL_FLOAT, 0, &normal[0]);
            glColorPointer(4, GL_FLOAT, 0, &color[0]);
            glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
            glVertexPointer(3, GL_FLOAT, 0, &vertex[0]);

            for (int c = 0; c < count; ++c) {
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glTranslatef(c - (count - 1) / 2.0, 0, -2);
                glRotatef(k * ((c & 1) * 2 - 1) + 90, 0, 1, 0);

                glDrawElements(GL_TRIANGLES, N, GL_UNSIGNED_INT, &index[0]);
            }
            glFlush();
            //RenderDevice::lastRenderDeviceCreated->window()->swapGLBuffers();
        }
        glFinish();
        RAMTime = System::time() - t0;
    }
    glPopClientAttrib();
    glPopAttrib();

    glPopClientAttrib();
    glPopAttrib();

    Log::common()->printf("\n%d triangles\n", count * N / 3);
    Log::common()->printf("RAM performance = %f FPS     VBO performance = %f FPS\n", (float)(frames - 1) / RAMTime, (float)(frames - 1)/ VBOTime);

    // See if the RAM performance was conservatively faster.
    value = RAMTime < VBOTime * 0.9;
    return value;
#endif
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

}


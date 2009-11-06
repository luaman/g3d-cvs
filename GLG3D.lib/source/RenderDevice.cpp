/**
 @file RenderDevice.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2001-07-08
 @edited  2009-01-15
 */

#include "G3D/platform.h"
#include "G3D/Log.h"
#include "G3D/GCamera.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Texture.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/Framebuffer.h"
#include "G3D/fileutils.h"
#include "GLG3D/Lighting.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/SuperShader.h" // to purge cache
#include "GLG3D/GLCaps.h"

#include "GLG3D/GApp.h" // for screenPrintf

namespace G3D {

RenderDevice* RenderDevice::lastRenderDeviceCreated = NULL;


static GLenum toGLBlendFunc(RenderDevice::BlendFunc b) {
    debugAssert(b != RenderDevice::BLEND_CURRENT);
    return GLenum(b);
}


static void _glViewport(double a, double b, double c, double d) {
    glViewport(iRound(a), iRound(b), 
	       iRound(a + c) - iRound(a), iRound(b + d) - iRound(b));
}


static GLenum primitiveToGLenum(RenderDevice::Primitive primitive) {
    return GLenum(primitive);
}


std::string RenderDevice::getCardDescription() const {
    return cardDescription;
}


void RenderDevice::beginOpenGL() {
    debugAssert(! m_inRawOpenGL);

    beforePrimitive();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    debugAssertGLOk();

    m_inRawOpenGL = true;
}


void RenderDevice::endOpenGL() {
    debugAssert(m_inRawOpenGL);
    m_inRawOpenGL = false;

    glPopClientAttrib();
    glPopAttrib();

    afterPrimitive();    
}


GCamera RenderDevice::projectionAndCameraMatrix() const {
    return GCamera(projectionMatrix(), cameraToWorldMatrix());
}


void RenderDevice::getFixedFunctionLighting(const LightingRef& lighting) const {
    // Reset state
    lighting->lightArray.fastClear();
    lighting->shadowedLightArray.fastClear();
    lighting->ambientBottom = lighting->ambientTop = Color3::black();

    // Load the lights
    if (m_state.lights.lightEnabled) {

        lighting->ambientBottom = lighting->ambientTop = m_state.lights.ambient.rgb();

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (m_state.lights.lightEnabled[i]) {
                lighting->lightArray.append(m_state.lights.light[i]);
            }
        }
    }
}

RenderDevice::RenderDevice() :
    m_window(NULL), m_deleteWindow(false), m_inRawOpenGL(false), m_minLineWidth(0), m_inIndexedPrimitive(false) {
    m_initialized = false;
    m_cleanedup = false;
    m_inPrimitive = false;
    m_inShader = false;
    m_numTextureUnits = 0;
    m_numTextures = 0;
    m_numTextureCoords = 0;
    m_lastTime = System::time();

    for (int i = 0; i < GLCaps::G3D_MAX_TEXTURE_UNITS; ++i) {
        currentlyBoundTexture[i] = 0;
    }

    lastRenderDeviceCreated = this;
}


void RenderDevice::setVARAreaMilestone() {
    m_currentVARArea->m_renderDevice = this;

    if (m_currentVARArea->m_mode == VertexBuffer::VBO_MEMORY) {
        // We don't need milestones when using VBO; the spec guarantees
        // correct synchronization.
        return;
    }

    MilestoneRef milestone = createMilestone("VertexRange Milestone");
    setMilestone(milestone);

    debugAssert(m_currentVARArea.notNull());

    // Overwrite any preexisting milestone
    m_currentVARArea->milestone = milestone;
}


RenderDevice::~RenderDevice() {
    debugAssertM(m_cleanedup || !initialized(), "You deleted an initialized RenderDevice without calling RenderDevice::cleanup()");
}

/**
 Used by RenderDevice::init.
 */
static const char* isOk(bool x) {
    return x ? "ok" : "UNSUPPORTED";
}

/**
 Used by RenderDevice::init.
 */
static const char* isOk(void* x) {
    // GCC incorrectly claims this function is not called.
    return isOk(x != NULL);
}


bool RenderDevice::supportsOpenGLExtension(
    const std::string& extension) const {

    return GLCaps::supports(extension);
}


void RenderDevice::init(
    const OSWindow::Settings&      _settings) {

    m_deleteWindow = true;
    init(OSWindow::create(_settings));
}


OSWindow* RenderDevice::window() const {
    return m_window;
}


void RenderDevice::init(OSWindow* window) {
    debugAssert(! initialized());
    debugAssert(window);

    m_swapBuffersAutomatically = true;
    m_swapGLBuffersPending = false;
    m_window = window;

    OSWindow::Settings settings;
    window->getSettings(settings);
    
    // Load the OpenGL extensions if they have not already been loaded.
    GLCaps::init();

    m_beginEndFrame = 0;

    // Under Windows, reset the last error so that our debug box
    // gives the correct results
    #ifdef G3D_WIN32
        SetLastError(0);
    #endif

    const int minimumDepthBits    = iMin(16, settings.depthBits);
    const int desiredDepthBits    = settings.depthBits;

    const int minimumStencilBits  = settings.stencilBits;
    const int desiredStencilBits  = settings.stencilBits;

    // Don't use more texture units than allowed at compile time.
    m_numTextureUnits  = GLCaps::numTextureUnits();
    m_numTextureCoords = GLCaps::numTextureCoords();
    m_numTextures      = GLCaps::numTextures();

    debugAssertGLOk();

    logPrintf("Setting video mode\n");

    setVideoMode();

    if (! strcmp((char*)glGetString(GL_RENDERER), "GDI Generic")) {
        logPrintf(
         "\n*********************************************************\n"
           "* WARNING: This computer does not have correctly        *\n"
           "*          installed graphics drivers and is using      *\n"
           "*          the default Microsoft OpenGL implementation. *\n"
           "*          Most graphics capabilities are disabled.  To *\n"
           "*          correct this problem, download and install   *\n"
           "*          the latest drivers for the graphics card.    *\n"
           "*********************************************************\n\n");
    }

    glViewport(0, 0, width(), height());
    int depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;
    depthBits       = glGetInteger(GL_DEPTH_BITS);
    stencilBits     = glGetInteger(GL_STENCIL_BITS);
    redBits         = glGetInteger(GL_RED_BITS);
    greenBits       = glGetInteger(GL_GREEN_BITS);
    blueBits        = glGetInteger(GL_BLUE_BITS);
    alphaBits       = glGetInteger(GL_ALPHA_BITS);
    debugAssertGLOk();

    bool depthOk   = depthBits >= minimumDepthBits;
    bool stencilOk = stencilBits >= minimumStencilBits;

    cardDescription = GLCaps::renderer() + " " + GLCaps::driverVersion();

    {
        int t = 0;
       
        int t0 = 0;
        if (GLCaps::supports_GL_ARB_multitexture()) {
            t = t0 = glGetInteger(GL_MAX_TEXTURE_UNITS_ARB);
        }

        if (GLCaps::supports_GL_ARB_fragment_program()) {
            t = glGetInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB);
        }
        
        logLazyPrintf("numTextureCoords                      = %d\n"
                     "numTextures                           = %d\n"
                     "numTextureUnits                       = %d\n"
                     "glGet(GL_MAX_TEXTURE_UNITS_ARB)       = %d\n"
                     "glGet(GL_MAX_TEXTURE_IMAGE_UNITS_ARB) = %d\n",
                     m_numTextureCoords, m_numTextures, m_numTextureUnits,
                     t0,
                     t);   

        logLazyPrintf("Operating System: %s\n",
                         System::operatingSystem().c_str());

        logLazyPrintf("Processor Architecture: %s\n\n", 
                         System::cpuArchitecture().c_str());

        logLazyPrintf("GL Vendor:      %s\n", GLCaps::vendor().c_str());

        logLazyPrintf("GL Renderer:    %s\n", GLCaps::renderer().c_str());

        logLazyPrintf("GL Version:     %s\n", GLCaps::glVersion().c_str());

        logLazyPrintf("Driver version: %s\n\n", GLCaps::driverVersion().c_str());

	    std::string extStringCopy = (char*)glGetString(GL_EXTENSIONS);

        logLazyPrintf(
            "GL extensions: \"%s\"\n\n",
            extStringCopy.c_str());

        // Test which texture and render buffer formats are supported by this card
        logLazyPrintf("Supported Formats:\n");
        logLazyPrintf("%20s  %s %s\n", "Format", "Texture", "RenderBuffer");
	
        for (int code = 0; code < ImageFormat::CODE_NUM; ++code) {
	        if ((code == ImageFormat::CODE_DEPTH24_STENCIL8) && 
		        (GLCaps::enumVendor() == GLCaps::MESA)) {
	            // Mesa seems to crash on this format
	            continue;
   	        }

            const ImageFormat* fmt = 
	            ImageFormat::fromCode((ImageFormat::Code)code);

            if (fmt) {
	        // printf("Format: %s\n", fmt->name().c_str());
                bool t = GLCaps::supportsTexture(fmt);
                bool r = GLCaps::supportsRenderBuffer(fmt);
                logLazyPrintf("%20s  %s       %s\n", fmt->name().c_str(), t ? "Yes" : "No ", r ? "Yes" : "No ");
            }
        }

        logLazyPrintf("\n");
    
        OSWindow::Settings actualSettings;
        window->getSettings(actualSettings);

        // This call is here to make GCC realize that isOk is used.
        (void)isOk(false);
        (void)isOk((void*)NULL);

        logLazyPrintf(
                 "Capability    Minimum   Desired   Received  Ok?\n"
                 "-------------------------------------------------\n"
                 "* RENDER DEVICE \n"
                 "Depth       %4d bits %4d bits %4d bits   %s\n"
                 "Stencil     %4d bits %4d bits %4d bits   %s\n"
                 "Alpha                           %4d bits   %s\n"
                 "Red                             %4d bits   %s\n"
                 "Green                           %4d bits   %s\n"
                 "Blue                            %4d bits   %s\n"
                 "FSAA                      %2d    %2d    %s\n"

                 "Width             %8d pixels           %s\n"
                 "Height            %8d pixels           %s\n"
                 "Mode                 %10s             %s\n\n",

                 minimumDepthBits, desiredDepthBits, depthBits, isOk(depthOk),
                 minimumStencilBits, desiredStencilBits, stencilBits, isOk(stencilOk),

                 alphaBits, "ok",
                 redBits, "ok", 
                 greenBits, "ok", 
                 blueBits, "ok", 

                 settings.fsaaSamples, actualSettings.fsaaSamples,
                 isOk(settings.fsaaSamples == actualSettings.fsaaSamples),

                 settings.width, "ok",
                 settings.height, "ok",
                 (settings.fullScreen ? "Fullscreen" : "Windowed"), "ok"
                 );

        logPrintf("Done initializing RenderDevice.\n"); 
    }

    m_initialized = true;

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    m_window->m_renderDevice = this;
}


void RenderDevice::describeSystem(
                                  std::string&        s) {
    
    TextOutput t;
    describeSystem(t);
    t.commitString(s);
}


bool RenderDevice::initialized() const {
    return m_initialized;
}


#ifdef G3D_WIN32

HDC RenderDevice::getWindowHDC() const {
    return wglGetCurrentDC();
}

#endif

void RenderDevice::setGamma(
    double              brightness,
    double              gamma) {
    
    Array<uint16> gammaRamp;
    gammaRamp.resize(256);

    for (int i = 0; i < 256; ++i) {
        gammaRamp[i] =
            (uint16)min(65535, 
                      max(0, 
                      (int)(pow((brightness * (i + 1)) / 256.0, gamma) * 65535 + 0.5)));
	}
    
    m_window->setGammaRamp(gammaRamp);
}


void RenderDevice::setVideoMode() {

    debugAssertM(m_stateStack.size() == 0, 
                 "Cannot call setVideoMode between pushState and popState");
    debugAssertM(m_beginEndFrame == 0, 
                 "Cannot call setVideoMode between beginFrame and endFrame");

    // Reset all state

    OSWindow::Settings settings;
    m_window->getSettings(settings);

    // Set the refresh rate
#   ifdef G3D_WIN32
        if (settings.asynchronous) {
            logLazyPrintf("wglSwapIntervalEXT(0);\n");
            wglSwapIntervalEXT(0);
        } else {
            logLazyPrintf("wglSwapIntervalEXT(1);\n");
            wglSwapIntervalEXT(1);
        }
#   endif

    // Enable proper specular lighting
    if (GLCaps::supports("GL_EXT_separate_specular_color")) {
        logLazyPrintf("Enabling separate specular lighting.\n");
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, 
                      GL_SEPARATE_SPECULAR_COLOR_EXT);
        debugAssertGLOk();
    } else {
        logLazyPrintf("Cannot enable separate specular lighting, extension not supported.\n");
    }

    // Make sure we use good interpolation.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Smoothing by default on NVIDIA cards.  On ATI cards
    // there is a bug that causes shaders to generate incorrect
    // results and run at 0 fps, so we can't enable this.
    if (! beginsWith(GLCaps::vendor(), "ATI")) {
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
    }
    

    // glHint(GL_GENERATE_MIPMAP_HINT_EXT, GL_NICEST);
    if (GLCaps::supports("GL_ARB_multisample")) {
        glEnable(GL_MULTISAMPLE_ARB);
    }

    debugAssertGLOk();
    if (GLCaps::supports("GL_NV_multisample_filter_hint")) {
        glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    }

    resetState();

    logPrintf("Done setting initial state.\n");
}


int RenderDevice::width() const {
    if (m_state.framebuffer.isNull()) {
        return m_window->width();
    } else {
        return m_state.framebuffer->width();
    }
}


int RenderDevice::height() const {
    if (m_state.framebuffer.isNull()) {
        return m_window->height();
    } else {
        return m_state.framebuffer->height();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////


Vector4 RenderDevice::project(const Vector3& v) const {
    return project(Vector4(v, 1));
}


Vector4 RenderDevice::project(const Vector4& v) const {
//    return glToScreen(v) + Vector4(0, viewport().y0(), 0, 0);
    Vector4 p = glToScreen(v);
    
    p.y += viewport().y1() + viewport().y0() - height();

    return p;
}


void RenderDevice::cleanup() {
    debugAssert(initialized());

    SuperShader::Pass::purgeCache();

    logLazyPrintf("Shutting down RenderDevice.\n");
    logPrintf("Restoring gamma.\n");
    setGamma(1, 1);

    logPrintf("Freeing all VertexRange memory\n");

    if (m_deleteWindow) {
        logPrintf("Deleting window.\n");
        VertexBuffer::cleanupAllVARAreas();
        delete m_window;
        m_window = NULL;
    }

    m_cleanedup = true;
}


void RenderDevice::push2D() {
    push2D(viewport());
}


void RenderDevice::push2D(const Rect2D& viewport) {
    push2D(m_state.framebuffer, viewport);
}


void RenderDevice::push2D(const FramebufferRef& fb) {
    const Rect2D& viewport = 
        (fb.notNull() && (fb->width() > 0)) ? 
            fb->rect2DBounds() : 
            Rect2D::xywh(0, 0, m_window->width(), m_window->height());
    push2D(fb, viewport);
}


void RenderDevice::push2D(const FramebufferRef& fb, const Rect2D& viewport) {
    pushState();

    setFramebuffer(fb);
    setDepthWrite(false);
    setDepthTest(DEPTH_ALWAYS_PASS);
    disableLighting();
    setCullFace(CULL_NONE);
    setViewport(viewport);
    setObjectToWorldMatrix(CoordinateFrame());

    // 0.375 is a float-to-int adjustment.  See: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/opengl/apptips_7wqb.asp
    //setCameraToWorldMatrix(CoordinateFrame(Matrix3::identity(), Vector3(-0.375, -0.375, 0.0)));

    setCameraToWorldMatrix(CoordinateFrame());

    setProjectionMatrix(Matrix4::orthogonalProjection(viewport.x0(), viewport.x0() + viewport.width(), 
                                                      viewport.y0() + viewport.height(), viewport.y0(), -1, 1));
}


void RenderDevice::pop2D() {
    popState();
}

////////////////////////////////////////////////////////////////////////////////////////////////
RenderDevice::RenderState::RenderState(int width, int height, int htutc) :

    // WARNING: this must be kept in sync with the initialization code
    // in init();
    viewport(Rect2D::xywh(0, 0, width, height)),

    useClip2D(false),

    depthWrite(true),
    colorWrite(true),
    alphaWrite(true),

    depthTest(DEPTH_LEQUAL),
    alphaTest(ALPHA_ALWAYS_PASS),
    alphaReference(0.0) {

    framebuffer = NULL;

    lights.twoSidedLighting =    false;


    srcBlendFunc                = BLEND_ONE;
    dstBlendFunc                = BLEND_ZERO;
    blendEq                     = BLENDEQ_ADD;

    drawBuffer                  = DRAW_BACK;
    readBuffer                  = READ_BACK;

    stencil.stencilTest         = STENCIL_ALWAYS_PASS;
    stencil.stencilReference    = 0;
    stencil.frontStencilFail    = STENCIL_KEEP;
    stencil.frontStencilZFail   = STENCIL_KEEP;
    stencil.frontStencilZPass   = STENCIL_KEEP;
    stencil.backStencilFail     = STENCIL_KEEP;
    stencil.backStencilZFail    = STENCIL_KEEP;
    stencil.backStencilZPass    = STENCIL_KEEP;

    polygonOffset               = 0;
    lineWidth                   = 1;
    pointSize                   = 1;

    renderMode                  = RenderDevice::RENDER_SOLID;

    shininess                   = 15;
    specular                    = Color3::white() * 0.8f;

    lights.ambient              = Color4(0.25f, 0.25f, 0.25f, 1.0f);

    lights.lighting             = false;
    color                       = Color4(1,1,1,1);
    normal                      = Vector3(0,0,0);

    // Note: texture units and lights initialize themselves

    matrices.objectToWorldMatrix         = CoordinateFrame();
    matrices.cameraToWorldMatrix         = CoordinateFrame();
    matrices.cameraToWorldMatrixInverse  = CoordinateFrame();

    stencil.stencilClear        = 0;
    depthClear                  = 1;
    colorClear                  = Color4(0,0,0,1);

    shadeMode                   = SHADE_FLAT;

    vertexAndPixelShader        = NULL;
    shader                      = NULL;
    vertexProgram               = NULL;
    pixelProgram                = NULL;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        lights.lightEnabled[i] = false;
    }

    // Set projection matrix
    double aspect;
    aspect = (double)viewport.width() / viewport.height();

    matrices.projectionMatrix = Matrix4::perspectiveProjection(-aspect, aspect, -1, 1, 0.1f, 100.0f);

    cullFace                    = CULL_BACK;

    lowDepthRange               = 0;
    highDepthRange              = 1;

    highestTextureUnitThatChanged = htutc;
}


RenderDevice::RenderState::TextureUnit::TextureUnit() : texture(NULL), LODBias(0) {
    texCoord        = Vector4(0,0,0,1);
    combineMode     = TEX_MODULATE;

    // Identity texture matrix
    memset(textureMatrix, 0, sizeof(float) * 16);
    for (int i = 0; i < 4; ++i) {
        textureMatrix[i + i * 4] = (float)1.0;
    }
}


void RenderDevice::resetState() {
    m_state = RenderState(width(), height());

    glClearDepth(1.0);

    glEnable(GL_NORMALIZE);

    debugAssertGLOk();
    if (GLCaps::supports_GL_EXT_stencil_two_side()) {
        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
    }

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    // Compute specular term correctly
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    debugAssertGLOk();

    logPrintf("Setting initial rendering state.\n");
    glDisable(GL_LIGHT0);
    debugAssertGLOk();
    {
        // WARNING: this must be kept in sync with the 
        // RenderState constructor
        m_state = RenderState(width(), height(), iMax(m_numTextures, m_numTextureCoords) - 1);

        _glViewport(m_state.viewport.x0(), m_state.viewport.y0(), m_state.viewport.width(), m_state.viewport.height());
        glDepthMask(GL_TRUE);
        glColorMask(1,1,1,1);

        if (GLCaps::supports_GL_EXT_stencil_two_side()) {
            glActiveStencilFaceEXT(GL_BACK);
        }
        for (int i = 0; i < 2; ++i) {
            glStencilMask((GLuint)~0);
            glDisable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
            glDisable(GL_ALPHA_TEST);
            if (GLCaps::supports_GL_EXT_stencil_two_side()) {
                glActiveStencilFaceEXT(GL_FRONT);
            }
        }

        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);

        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glLineWidth(1);
        glPointSize(1);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_state.lights.ambient);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, m_state.lights.twoSidedLighting);

        glDisable(GL_LIGHTING);

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            setLight(i, NULL, true);
        }
        glColor4d(1, 1, 1, 1);
        glNormal3d(0, 0, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        setShininess(m_state.shininess);
        setSpecularCoefficient(m_state.specular);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glShadeModel(GL_FLAT);

        glClearStencil(0);
        glClearDepth(1);
        glClearColor(0,0,0,1);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrix(m_state.matrices.projectionMatrix);
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glDepthRange(0, 1);

        // Set up the texture units.
        if (glMultiTexCoord4fvARB != NULL) {
            for (int t = m_numTextureCoords - 1; t >= 0; --t) {
                float f[] = {0,0,0,1};
                glMultiTexCoord4fvARB(GL_TEXTURE0_ARB + t, f);
            }
        } else if (m_numTextureCoords > 0) {
            glTexCoord(Vector4(0,0,0,1));
        }

        if (glActiveTextureARB != NULL) {
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }
    }
    debugAssertGLOk();

}


bool RenderDevice::RenderState::Lights::operator==(const Lights& other) const {
    for (int L = 0; L < MAX_LIGHTS; ++L) {
        if ((lightEnabled[L] != other.lightEnabled[L]) ||
            (lightEnabled[L] && (light[L] != other.light[L]))) {
            return false;
        }
    }

    debugAssertM(changed == false,
        "Should never enter lighting comparison "
        "when lighting has not changed.");

    return 
        (lighting == other.lighting) && 
        (ambient == other.ambient) &&
        (twoSidedLighting == other.twoSidedLighting);
}


bool RenderDevice::RenderState::Stencil::operator==(const Stencil& other) const {
    return 
        (stencilTest == other.stencilTest) &&
        (stencilReference == other.stencilReference) &&
        (stencilClear == other.stencilClear) &&
        (frontStencilFail == other.frontStencilFail) &&
        (frontStencilZFail == other.frontStencilZFail) &&
        (frontStencilZPass == other.frontStencilZPass) &&
        (backStencilFail == other.backStencilFail) &&
        (backStencilZFail == other.backStencilZFail) &&
        (backStencilZPass == other.backStencilZPass);
}


bool RenderDevice::RenderState::Matrices::operator==(const Matrices& other) const {
    return
        (objectToWorldMatrix == other.objectToWorldMatrix) &&
        (cameraToWorldMatrix == other.cameraToWorldMatrix) &&
        (projectionMatrix == other.projectionMatrix);
}


void RenderDevice::setState(
    const RenderState&          newState) {
    debugAssertGLOk();
    // The state change checks inside the individual
    // methods will (for the most part) minimize
    // the state changes so we can set all of the
    // new state explicitly.
    
    // Set framebuffer first, since it can affect the viewport
    if (m_state.framebuffer != newState.framebuffer) {
        setFramebuffer(newState.framebuffer);
        debugAssertGLOk();
        
        // Intentionally corrupt the viewport, forcing renderdevice to
        // reset it below.
        m_state.viewport = Rect2D::xywh(-1, -1, -1, -1);
    }
    debugAssertGLOk();
    
    setViewport(newState.viewport);

    if (newState.useClip2D) {
        setClip2D(newState.clip2D);
    } else {
        setClip2D(Rect2D::inf());
    }
    
    setDepthWrite(newState.depthWrite);
    setColorWrite(newState.colorWrite);
    setAlphaWrite(newState.alphaWrite);

    debugAssertGLOk();
    setDrawBuffer(newState.drawBuffer);
    debugAssertGLOk();
    setReadBuffer(newState.readBuffer);
    debugAssertGLOk();

    setShadeMode(newState.shadeMode);
    debugAssertGLOk();
    setDepthTest(newState.depthTest);
    debugAssertGLOk();

    if (newState.stencil != m_state.stencil) {
        setStencilConstant(newState.stencil.stencilReference);

        debugAssertGLOk();
        setStencilTest(newState.stencil.stencilTest);

        setStencilOp(
            newState.stencil.frontStencilFail, newState.stencil.frontStencilZFail, newState.stencil.frontStencilZPass,
            newState.stencil.backStencilFail, newState.stencil.backStencilZFail, newState.stencil.backStencilZPass);

        setStencilClearValue(newState.stencil.stencilClear);
    }

    setDepthClearValue(newState.depthClear);

    setColorClearValue(newState.colorClear);

    setAlphaTest(newState.alphaTest, newState.alphaReference);

    setBlendFunc(newState.srcBlendFunc, newState.dstBlendFunc, newState.blendEq);

    setRenderMode(newState.renderMode);
    setPolygonOffset(newState.polygonOffset);
    setLineWidth(newState.lineWidth);
    setPointSize(newState.pointSize);

    setSpecularCoefficient(newState.specular);
    setShininess(newState.shininess);

    if (m_state.lights.changed) {
        if (newState.lights.lighting) {
            enableLighting();
        } else {
            disableLighting();
        }

        if (newState.lights.twoSidedLighting) {
            enableTwoSidedLighting();
        } else {
            disableTwoSidedLighting();
        }

        setAmbientLightColor(newState.lights.ambient);

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (newState.lights.lightEnabled[i]) {
                setLight(i, newState.lights.light[i]);
            } else {
                setLight(i, NULL);
            }
        }
    }

    setColor(newState.color);
    setNormal(newState.normal);

    for (int u = m_state.highestTextureUnitThatChanged; u >= 0; --u) {
        if (newState.textureUnit[u] != m_state.textureUnit[u]) {

            if (u < (int)numTextures()) {
                setTexture(u, newState.textureUnit[u].texture);

                if (u < (int)numTextureUnits()) {
                    setTextureCombineMode(u, newState.textureUnit[u].combineMode);
                    setTextureMatrix(u, newState.textureUnit[u].textureMatrix);

                    setTextureLODBias(u, newState.textureUnit[u].LODBias);
                }
            }
            setTexCoord(u, newState.textureUnit[u].texCoord);            
        }
    }

    setCullFace(newState.cullFace);

    setDepthRange(newState.lowDepthRange, newState.highDepthRange);

    if (m_state.matrices.changed) { //(newState.matrices != m_state.matrices) { 
        if (newState.matrices.cameraToWorldMatrix != m_state.matrices.cameraToWorldMatrix) {
            setCameraToWorldMatrix(newState.matrices.cameraToWorldMatrix);
        }

        if (newState.matrices.objectToWorldMatrix != m_state.matrices.objectToWorldMatrix) {
            setObjectToWorldMatrix(newState.matrices.objectToWorldMatrix);
        }

        setProjectionMatrix(newState.matrices.projectionMatrix);
    }

    setVertexAndPixelShader(newState.vertexAndPixelShader);
    setShader(newState.shader);

    if (supportsVertexProgram()) {
        setVertexProgram(newState.vertexProgram);
    }

    if (supportsPixelProgram()) {
        setPixelProgram(newState.pixelProgram);
    }
    
    // Adopt the popped state's deltas relative the state that it replaced.
    m_state.highestTextureUnitThatChanged = newState.highestTextureUnitThatChanged;
    m_state.matrices.changed = newState.matrices.changed;
    m_state.lights.changed = newState.lights.changed;
}


void RenderDevice::enableTwoSidedLighting() {
    minStateChange();
    if (! m_state.lights.twoSidedLighting) {
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
        m_state.lights.twoSidedLighting = true;
        m_state.lights.changed = true;
        minGLStateChange();
    }
}


void RenderDevice::disableTwoSidedLighting() {
    minStateChange();
    if (m_state.lights.twoSidedLighting) {
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
        m_state.lights.twoSidedLighting = false;
        m_state.lights.changed = true;
        minGLStateChange();
    }
}


void RenderDevice::syncDrawBuffer(bool alreadyBound) {
    if (m_state.framebuffer.isNull()) {
        return;
    }

    if (m_state.framebuffer->bind(alreadyBound)) {
        debugAssertGLOk();
        
        // Apply the bindings from this framebuffer
        const Array<GLenum>& array = m_state.framebuffer->openGLDrawArray();
        if (array.size() > 0) {
            debugAssertM(array.size() <= glGetInteger(GL_MAX_DRAW_BUFFERS),
                         format("This graphics card only supports %d draw buffers.",
                                glGetInteger(GL_MAX_DRAW_BUFFERS)));
            
            glDrawBuffersARB(array.size(), array.getCArray());
            debugAssertGLOk();
        } else {
            // May be only depth or stencil; don't need a draw buffer.
            
            debugAssertGLOk();
            // Some drivers crash when providing NULL or an actual
            // zero-element array for a zero-element array, so make a fake
            // array.
            const GLenum noColorBuffers[] = { GL_NONE };
            glDrawBuffersARB(1, noColorBuffers);
            debugAssertGLOk();
        }
    }
}


void RenderDevice::beforePrimitive() {
    debugAssertM(! m_inRawOpenGL, "Cannot make RenderDevice calls while inside beginOpenGL...endOpenGL");

    syncDrawBuffer(true);

    if (! m_state.shader.isNull()) {
        debugAssert(! m_inShader);
        m_inShader = true;
        m_state.shader->beforePrimitive(this);
        m_inShader = false;
    }
    
    // If a Shader was bound, it will force this.  Otherwise we need to do so.
    forceVertexAndPixelShaderBind();
}


void RenderDevice::afterPrimitive() {
    if (! m_state.shader.isNull()) {
        debugAssert(! m_inShader);
        m_inShader = true;
        m_state.shader->afterPrimitive(this);
        m_inShader = false;
    }
}


void RenderDevice::setSpecularCoefficient(const Color3& c) {
    minStateChange();
    if (m_state.specular != c) {
        m_state.specular = c;

        static float spec[4];
        spec[0] = c[0];
        spec[1] = c[1];
        spec[2] = c[2];
        spec[3] = 1.0f;

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
        minGLStateChange();
    }
}


void RenderDevice::setSpecularCoefficient(float s) {
    setSpecularCoefficient(s * Color3::white());
}


void RenderDevice::setShininess(float s) {
    minStateChange();
    if (m_state.shininess != s) {
        m_state.shininess = s;
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, s);
        minGLStateChange();
    }
}


void RenderDevice::setRenderMode(RenderMode m) {
    minStateChange();

    if (m == RENDER_CURRENT) {
        return;
    }

    if (m_state.renderMode != m) {
        minGLStateChange();
        m_state.renderMode = m;
        switch (m) {
        case RENDER_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case RENDER_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;

        case RENDER_POINTS:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;

        case RENDER_CURRENT:
            return;
            break;
        }
    }
}


RenderDevice::RenderMode RenderDevice::renderMode() const {
    return m_state.renderMode;
}


void RenderDevice::setDrawBuffer(DrawBuffer b) {
    minStateChange();

    if (b == DRAW_CURRENT) {
        return;
    }

    if (m_state.framebuffer.isNull()) {
        alwaysAssertM
            (!( (b >= DRAW_COLOR0) && (b <= DRAW_COLOR15)), 
             "Drawing to a color buffer is only supported by application-created framebuffers!");
    }

    if (b != m_state.drawBuffer) {
        minGLStateChange();
        m_state.drawBuffer = b;
        if (m_state.framebuffer.isNull()) {
            // Only update the GL state if there is no framebuffer bound.
            glDrawBuffer(GLenum(m_state.drawBuffer));
        }
    }
}


void RenderDevice::setReadBuffer(ReadBuffer b) {
    minStateChange();

    if (b == READ_CURRENT) {
        return;
    }

    if (m_state.framebuffer.isNull()) {
        alwaysAssertM
            (!( (b >= READ_COLOR0) && (b <= READ_COLOR15)), 
             "Drawing to a color buffer is only supported by application-created framebuffers!");
    }

    if (b != m_state.readBuffer) {
        minGLStateChange();
        m_state.readBuffer = b;
        glReadBuffer(GLenum(m_state.readBuffer));
    }
}


void RenderDevice::setCullFace(CullFace f) {
    minStateChange();
    if (f != m_state.cullFace && f != CULL_CURRENT) {
        minGLStateChange();
        
        if (f == CULL_NONE) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GLenum(f));
        }

        m_state.cullFace = f;
    }
}


void RenderDevice::pushState() {
    debugAssert(! m_inPrimitive);

    // texgen enables
    glPushAttrib(GL_TEXTURE_BIT);
    m_stateStack.push(m_state);

    // Record that that the lights and matrices are unchanged since the previous state.
    // This allows popState to restore the lighting environment efficiently.

    m_state.lights.changed = false;
    m_state.matrices.changed = false;
    m_state.highestTextureUnitThatChanged = -1;

    m_stats.pushStates += 1;
}


void RenderDevice::popState() {
    debugAssertGLOk();
    debugAssert(! m_inPrimitive);
    debugAssertM(m_stateStack.size() > 0, "More calls to RenderDevice::pushState() than RenderDevice::popState().");
    setState(m_stateStack.last());
    m_stateStack.popDiscard();
    // texgen enables
    glPopAttrib();
}


void RenderDevice::clear(bool clearColor, bool clearDepth, bool clearStencil) {
    debugAssert(! m_inPrimitive);
    syncDrawBuffer(true);

#   ifdef G3D_DEBUG
    {
        std::string why;
        debugAssertM(currentFramebufferComplete(why), why);
    }
#   endif
    majStateChange();
    majGLStateChange();

    GLint mask = 0;

    bool oldColorWrite = colorWrite();
    if (clearColor) {
        mask |= GL_COLOR_BUFFER_BIT;
        setColorWrite(true);
    }

    bool oldDepthWrite = depthWrite();
    if (clearDepth) {
        mask |= GL_DEPTH_BUFFER_BIT;
        setDepthWrite(true);
    }

    if (clearStencil) {
        mask |= GL_STENCIL_BUFFER_BIT;
        minGLStateChange();
        minStateChange();
    }

    glClear(mask);
    minGLStateChange();
    minStateChange();
    setColorWrite(oldColorWrite);
    setDepthWrite(oldDepthWrite);
}


uint32 RenderDevice::numTextureUnits() const {
    return m_numTextureUnits;
}


uint32 RenderDevice::numTextures() const {
    return m_numTextures;
}


uint32 RenderDevice::numTextureCoords() const {
    return m_numTextureCoords;
}


void RenderDevice::beginFrame() {
    if (m_swapGLBuffersPending) {
        swapBuffers();
    }

    m_stats.reset();

    ++m_beginEndFrame;
    debugAssertM(m_beginEndFrame == 1, "Mismatched calls to beginFrame/endFrame");
}


void RenderDevice::swapBuffers() {

    // Process the pending swap buffers call
    m_swapTimer.tick();
    m_window->swapGLBuffers();
    m_swapTimer.tock();
    m_swapGLBuffersPending = false;
}


void RenderDevice::setSwapBuffersAutomatically(bool b) {
    if (b == m_swapBuffersAutomatically) {
        // Setting to current state; nothing to do.
        return;
    }

    if (m_swapGLBuffersPending) {
        swapBuffers();
    }

    m_swapBuffersAutomatically = b;
}


void RenderDevice::endFrame() {
    --m_beginEndFrame;
    debugAssertM(m_beginEndFrame == 0, "Mismatched calls to beginFrame/endFrame");

    // Schedule a swap buffer iff we are handling them automatically.
    m_swapGLBuffersPending = m_swapBuffersAutomatically;

    debugAssertM(m_stateStack.size() == 0, "Missing RenderDevice::popState or RenderDevice::pop2D.");

    double now = System::time();
    double dt = now - m_lastTime;
    if (dt <= 0) {
        dt = 0.0001;
    }

    m_stats.frameRate = 1.0f / dt;
    m_stats.triangleRate = m_stats.triangles * dt;

    {
        // small inter-frame time: A (interpolation parameter) is small
        // large inter-frame time: A is big
        double A = clamp(dt * 0.6, 0.001, 1.0);
        if (abs(m_stats.smoothFrameRate - m_stats.frameRate) / max(m_stats.smoothFrameRate, m_stats.frameRate) > 0.18) {
            // There's a huge discrepancy--something major just changed in the way we're rendering
            // so we should jump to the new value.
            A = 1.0;
        }
    
        m_stats.smoothFrameRate     = lerp(m_stats.smoothFrameRate, m_stats.frameRate, (float)A);
        m_stats.smoothTriangleRate  = lerp(m_stats.smoothTriangleRate, m_stats.triangleRate, A);
        m_stats.smoothTriangles     = lerp(m_stats.smoothTriangles, m_stats.triangles, A);
    }

    if ((m_stats.smoothFrameRate == finf()) || (isNaN(m_stats.smoothFrameRate))) {
        m_stats.smoothFrameRate = 1000000;
    } else if (m_stats.smoothFrameRate < 0) {
        m_stats.smoothFrameRate = 0;
    }

    if ((m_stats.smoothTriangleRate == finf()) || isNaN(m_stats.smoothTriangleRate)) {
        m_stats.smoothTriangleRate = 1e20;
    } else if (m_stats.smoothTriangleRate < 0) {
        m_stats.smoothTriangleRate = 0;
    }

    if ((m_stats.smoothTriangles == finf()) || isNaN(m_stats.smoothTriangles)) {
        m_stats.smoothTriangles = 1e20;
    } else if (m_stats.smoothTriangles < 0) {
        m_stats.smoothTriangles = 0;
    }

    m_lastTime = now;
}


bool RenderDevice::alphaWrite() const {
    return m_state.alphaWrite;
}


bool RenderDevice::depthWrite() const {
    return m_state.depthWrite;
}


bool RenderDevice::colorWrite() const {
    return m_state.colorWrite;
}


void RenderDevice::setStencilClearValue(int s) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.stencil.stencilClear != s) {
        minGLStateChange();
        glClearStencil(s);
        m_state.stencil.stencilClear = s;
    }
}


void RenderDevice::setDepthClearValue(float d) {
    minStateChange();
    debugAssert(! m_inPrimitive);
    if (m_state.depthClear != d) {
        minGLStateChange();
        glClearDepth(d);
        m_state.depthClear = d;
    }
}


void RenderDevice::setColorClearValue(const Color4& c) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.colorClear != c) {
        minGLStateChange();
        glClearColor(c.r, c.g, c.b, c.a);
        m_state.colorClear = c;
    }
}


void RenderDevice::setViewport(const Rect2D& v) {
    minStateChange();
    if (m_state.viewport != v) {
        // Flip to OpenGL y-axis
        float h = height();
        _glViewport(v.x0(), h - v.y1(), v.width(), v.height());
        m_state.viewport = v;
        minGLStateChange();
    }
}


void RenderDevice::setClip2D(const Rect2D& clip) {
    minStateChange();

    if (clip.isFinite()) {
        // set the new clip Rect2D
        minGLStateChange();
        m_state.clip2D = clip;

        int clipX0 = iFloor(clip.x0());
        int clipY0 = iFloor(clip.y0());
        int clipX1 = iCeil(clip.x1());
        int clipY1 = iCeil(clip.y1());

        glScissor(clipX0, height() - clipY1, clipX1 - clipX0, clipY1 - clipY0);

        if (clip.area() == 0) {
            // On some graphics cards a clip region that is zero without being (0,0,0,0) 
            // fails to actually clip everything.
            glScissor(0,0,0,0);
            glEnable(GL_SCISSOR_TEST);
        }

        // enable scissor test itself if not 
        // just adjusting the clip region
        if (! m_state.useClip2D) {
            glEnable(GL_SCISSOR_TEST);
            minStateChange();
            minGLStateChange();
            m_state.useClip2D = true;
        }
    } else if (m_state.useClip2D) {
        // disable scissor test if not already disabled
        minGLStateChange();
        glDisable(GL_SCISSOR_TEST);
        m_state.useClip2D = false;
    }
}


Rect2D RenderDevice::clip2D() const {
    if (m_state.useClip2D) {
        return m_state.clip2D;
    } else {
        return m_state.viewport;
    }
}

void RenderDevice::setProjectionAndCameraMatrix(const GCamera& camera) {
    Matrix4 P;
    camera.getProjectUnitMatrix(viewport(), P);
    setProjectionMatrix(P);
    setCameraToWorldMatrix(camera.coordinateFrame());
}


Rect2D RenderDevice::viewport() const {
    return m_state.viewport;
}


void RenderDevice::pushState(const FramebufferRef& fb) {
    pushState();

    if (fb.notNull()) {
        setFramebuffer(fb);
        setViewport(fb->rect2DBounds());
    }
}


void RenderDevice::setFramebuffer(const FramebufferRef& fbo) {
    if (fbo != m_state.framebuffer) {
        majGLStateChange();

        // Set Framebuffer
        if (fbo.isNull()) {
            m_state.framebuffer = NULL;
            Framebuffer::bindWindowBuffer();
            debugAssertGLOk();

            // Restore the buffer that was in use before the framebuffer was attached
            glDrawBuffer(GLenum(m_state.drawBuffer));
            debugAssertGLOk();
        } else {
            debugAssertM(GLCaps::supports_GL_EXT_framebuffer_object(), 
                "Framebuffer Object not supported!");
            m_state.framebuffer = fbo;
            syncDrawBuffer(false);

            if (m_state.readBuffer != READ_NONE) {
                if (! fbo->has(static_cast<Framebuffer::AttachmentPoint>(m_state.readBuffer))) {
                    // Switch to color0 or none
                    if (fbo->has(Framebuffer::COLOR0)) {
                        setReadBuffer(READ_COLOR0);
                    } else {
                        setReadBuffer(READ_NONE);
                    }
                }
            }

            // The enables for this framebuffer will be set during beforePrimitive()            
        }
    }
}


void RenderDevice::setDepthTest(DepthTest test) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    // On ALWAYS_PASS we must force a change because under OpenGL
    // depthWrite and depth test are dependent.
    if ((test == DEPTH_CURRENT) && (test != DEPTH_ALWAYS_PASS)) {
        return;
    }
    
    if ((m_state.depthTest != test) && (test != DEPTH_ALWAYS_PASS)) {
        minGLStateChange();
        if ((test == DEPTH_ALWAYS_PASS) && (m_state.depthWrite == false)) {
            // http://www.opengl.org/sdk/docs/man/xhtml/glDepthFunc.xml
            // "Even if the depth buffer exists and the depth mask is non-zero, the
            // depth buffer is not updated if the depth test is disabled."
            glDisable(GL_DEPTH_TEST);
        } else {
            minStateChange();
            minGLStateChange();
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GLenum(test));
        }

        m_state.depthTest = test;
    }
}

static GLenum toGLEnum(RenderDevice::StencilTest t) {
    debugAssert(t != RenderDevice::STENCIL_CURRENT);
    return GLenum(t);
}


void RenderDevice::_setStencilTest(RenderDevice::StencilTest test, int reference) {

    if (test == RenderDevice::STENCIL_CURRENT) {
        return;
    }

    const GLenum t = toGLEnum(test);

    if (GLCaps::supports_GL_EXT_stencil_two_side()) {
        // NVIDIA/EXT
        glActiveStencilFaceEXT(GL_BACK);
        glStencilFunc(t, reference, 0xFFFFFFFF);
        glActiveStencilFaceEXT(GL_FRONT);
        glStencilFunc(t, reference, 0xFFFFFFFF);
        minGLStateChange(4);
    } else if (GLCaps::supports_GL_ATI_separate_stencil()) {
        // ATI
        glStencilFuncSeparateATI(t, t, reference, 0xFFFFFFFF);
        minGLStateChange(1);
    } else {
        // Default OpenGL
        glStencilFunc(t, reference, 0xFFFFFFFF);
        minGLStateChange(1);
    }
}


void RenderDevice::setStencilConstant(int reference) {
    minStateChange();

    debugAssert(! m_inPrimitive);
    if (m_state.stencil.stencilReference != reference) {

        m_state.stencil.stencilReference = reference;
        _setStencilTest(m_state.stencil.stencilTest, reference);
        minStateChange();

    }
}


void RenderDevice::setStencilTest(StencilTest test) {
    minStateChange();

	if (test == STENCIL_CURRENT) {
		return;
	}

    debugAssert(! m_inPrimitive);

    if (m_state.stencil.stencilTest != test) {
        glEnable(GL_STENCIL_TEST);

        if (test == STENCIL_ALWAYS_PASS) {

            // Can't actually disable if the stencil op is using the test as well
            // due to an OpenGL limitation.
            if ((m_state.stencil.frontStencilFail   == STENCIL_KEEP) &&
                (m_state.stencil.frontStencilZFail  == STENCIL_KEEP) &&
                (m_state.stencil.frontStencilZPass  == STENCIL_KEEP) &&
                (! GLCaps::supports_GL_EXT_stencil_two_side() ||
                 ((m_state.stencil.backStencilFail  == STENCIL_KEEP) &&
                  (m_state.stencil.backStencilZFail == STENCIL_KEEP) &&
                  (m_state.stencil.backStencilZPass == STENCIL_KEEP)))) {
                minGLStateChange();
                glDisable(GL_STENCIL_TEST);
            }

        } else {

            _setStencilTest(test, m_state.stencil.stencilReference);
        }

        m_state.stencil.stencilTest = test;

    }
}


RenderDevice::AlphaTest RenderDevice::alphaTest() const {
    return m_state.alphaTest;
}


float RenderDevice::alphaTestReference() const {
    return m_state.alphaReference;
}


void RenderDevice::setAlphaTest(AlphaTest test, float reference) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    if (test == ALPHA_CURRENT) {
        return;
    }
    
    if ((m_state.alphaTest != test) || (m_state.alphaReference != reference)) {
        minGLStateChange();
        if (test == ALPHA_ALWAYS_PASS) {
            
            glDisable(GL_ALPHA_TEST);

        } else {

            glEnable(GL_ALPHA_TEST);
            switch (test) {
            case ALPHA_LESS:
                glAlphaFunc(GL_LESS, reference);
                break;

            case ALPHA_LEQUAL:
                glAlphaFunc(GL_LEQUAL, reference);
                break;

            case ALPHA_GREATER:
                glAlphaFunc(GL_GREATER, reference);
                break;

            case ALPHA_GEQUAL:
                glAlphaFunc(GL_GEQUAL, reference);
                break;

            case ALPHA_EQUAL:
                glAlphaFunc(GL_EQUAL, reference);
                break;

            case ALPHA_NOTEQUAL:
                glAlphaFunc(GL_NOTEQUAL, reference);
                break;

            case ALPHA_NEVER_PASS:
                glAlphaFunc(GL_NEVER, reference);
                break;

            default:
                debugAssertM(false, "Fell through switch");
            }
        }

        m_state.alphaTest = test;
        m_state.alphaReference = reference;
    }
}


GLint RenderDevice::toGLStencilOp(RenderDevice::StencilOp op) const {
    debugAssert(op != STENCILOP_CURRENT);
    switch (op) {
    case RenderDevice::STENCIL_INCR_WRAP:
        if (GLCaps::supports_GL_EXT_stencil_wrap()) {
            return GL_INCR_WRAP_EXT;
        } else {
            return GL_INCR;
        }

    case RenderDevice::STENCIL_DECR_WRAP:
        if (GLCaps::supports_GL_EXT_stencil_wrap()) {
            return GL_DECR_WRAP_EXT;
        } else {
            return GL_DECR;
        }
    default:
        return GLenum(op);
    }
}


void RenderDevice::setShader(const ShaderRef& s) {
    majStateChange();
    
    if (s != m_state.shader) {
        debugAssertM(! m_inShader, "Cannot set the Shader from within a Shader!");
        m_state.shader = s;
    }

    if (s.isNull()) {
        setVertexAndPixelShader(NULL);
    }
}


void RenderDevice::forceVertexAndPixelShaderBind() {
    // Only change the vertex shader if it does not match the one used
    // last time.
    if (m_lastVertexAndPixelShader != m_state.vertexAndPixelShader) {

        majGLStateChange();
        if (m_state.vertexAndPixelShader.isNull()) {
            // Disables the programmable pipeline
            glUseProgramObjectARB(0);
        } else {
            glUseProgramObjectARB(m_state.vertexAndPixelShader->glProgramObject());
        }
        debugAssertGLOk();

        m_lastVertexAndPixelShader = m_state.vertexAndPixelShader;
    }
}


void RenderDevice::setVertexAndPixelShader(const VertexAndPixelShaderRef& s) {
    majStateChange();

    if (s != m_state.vertexAndPixelShader) {

        m_state.vertexAndPixelShader = s;

        if (s.notNull()) {
            alwaysAssertM(s->ok(), s->messages());
        }

        // The actual shader will change in beforePrimitive or when the arg list is bound.
    }
}


void RenderDevice::setVertexAndPixelShader(
    const VertexAndPixelShaderRef&          s,
    const VertexAndPixelShader::ArgList&    args) {

    setVertexAndPixelShader(s);

    if (s.notNull()) {
        s->bindArgList(this, args);
    }
}


void RenderDevice::setVertexProgram(const VertexProgramRef& vp) {
    majStateChange();
    if (vp != m_state.vertexProgram) {
        if (m_state.vertexProgram != (VertexProgramRef)NULL) {
            m_state.vertexProgram->disable();
        }

        majGLStateChange();
        if (vp != (VertexProgramRef)NULL) {
            debugAssert(supportsVertexProgram());
            vp->bind();
        }

        m_state.vertexProgram = vp;
    }
}


void RenderDevice::setVertexProgram(
    const VertexProgramRef& vp,
    const GPUProgram::ArgList& args) {

    setVertexProgram(vp);
    
    vp->setArgs(this, args);
}


void RenderDevice::setPixelProgram(const PixelProgramRef& pp) {
    majStateChange();
    if (pp != m_state.pixelProgram) {
        if (m_state.pixelProgram != (PixelProgramRef)NULL) {
            m_state.pixelProgram->disable();
        }
        if (pp != (PixelProgramRef)NULL) {
            debugAssert(supportsPixelProgram());
            pp->bind();
        }
        majGLStateChange();
        m_state.pixelProgram = pp;
    }
}


void RenderDevice::setPixelProgram(
    const PixelProgramRef& pp,
    const GPUProgram::ArgList& args) {

    setPixelProgram(pp);
    
    pp->setArgs(this, args);
}


void RenderDevice::setStencilOp(
    StencilOp                       frontStencilFail,
    StencilOp                       frontZFail,
    StencilOp                       frontZPass,
    StencilOp                       backStencilFail,
    StencilOp                       backZFail,
    StencilOp                       backZPass) {

    minStateChange();

	if (frontStencilFail == STENCILOP_CURRENT) {
		frontStencilFail = m_state.stencil.frontStencilFail;
	}
	
	if (frontZFail == STENCILOP_CURRENT) {
		frontZFail = m_state.stencil.frontStencilZFail;
	}
	
	if (frontZPass == STENCILOP_CURRENT) {
		frontZPass = m_state.stencil.frontStencilZPass;
	}

	if (backStencilFail == STENCILOP_CURRENT) {
		backStencilFail = m_state.stencil.backStencilFail;
	}
	
	if (backZFail == STENCILOP_CURRENT) {
		backZFail = m_state.stencil.backStencilZFail;
	}
	
	if (backZPass == STENCILOP_CURRENT) {
		backZPass = m_state.stencil.backStencilZPass;
	}
    
	if ((frontStencilFail  != m_state.stencil.frontStencilFail) ||
        (frontZFail        != m_state.stencil.frontStencilZFail) ||
        (frontZPass        != m_state.stencil.frontStencilZPass) || 
        (GLCaps::supports_two_sided_stencil() && 
        ((backStencilFail  != m_state.stencil.backStencilFail) ||
         (backZFail        != m_state.stencil.backStencilZFail) ||
         (backZPass        != m_state.stencil.backStencilZPass)))) { 

        if (GLCaps::supports_GL_EXT_stencil_two_side()) {
            // NVIDIA

            // Set back face operation
            glActiveStencilFaceEXT(GL_BACK);
            glStencilOp(
                toGLStencilOp(backStencilFail),
                toGLStencilOp(backZFail),
                toGLStencilOp(backZPass));
            
            // Set front face operation
            glActiveStencilFaceEXT(GL_FRONT);
            glStencilOp(
                toGLStencilOp(frontStencilFail),
                toGLStencilOp(frontZFail),
                toGLStencilOp(frontZPass));

            minGLStateChange(4);

        } else if (GLCaps::supports_GL_ATI_separate_stencil()) {
            // ATI
            minGLStateChange(2);
            glStencilOpSeparateATI(GL_FRONT, 
                toGLStencilOp(frontStencilFail),
                toGLStencilOp(frontZFail),
                toGLStencilOp(frontZPass));

            glStencilOpSeparateATI(GL_BACK, 
                toGLStencilOp(backStencilFail),
                toGLStencilOp(backZFail),
                toGLStencilOp(backZPass));

        } else {
            // Generic OpenGL

            // Set front face operation
            glStencilOp(
                toGLStencilOp(frontStencilFail),
                toGLStencilOp(frontZFail),
                toGLStencilOp(frontZPass));

            minGLStateChange(1);
        }


        // Need to manage the mask as well

        if ((frontStencilFail  == STENCIL_KEEP) &&
            (frontZPass        == STENCIL_KEEP) && 
            (frontZFail        == STENCIL_KEEP) &&
            (! GLCaps::supports_two_sided_stencil() ||
            ((backStencilFail  == STENCIL_KEEP) &&
             (backZPass        == STENCIL_KEEP) &&
             (backZFail        == STENCIL_KEEP)))) {

            // Turn off writing.  May need to turn off the stencil test.
            if (m_state.stencil.stencilTest == STENCIL_ALWAYS_PASS) {
                // Test doesn't need to be on
                glDisable(GL_STENCIL_TEST);
            }

        } else {

            debugAssertM(glGetInteger(GL_STENCIL_BITS) > 0,
                "Allocate stencil bits from RenderDevice::init before using the stencil buffer.");

            // Turn on writing.  We also need to turn on the
            // stencil test in this case.

            if (m_state.stencil.stencilTest == STENCIL_ALWAYS_PASS) {
                // Test is not already on
                glEnable(GL_STENCIL_TEST);
                _setStencilTest(m_state.stencil.stencilTest, m_state.stencil.stencilReference);
            }
        }

        m_state.stencil.frontStencilFail  = frontStencilFail;
        m_state.stencil.frontStencilZFail = frontZFail;
        m_state.stencil.frontStencilZPass = frontZPass;
        m_state.stencil.backStencilFail   = backStencilFail;
        m_state.stencil.backStencilZFail  = backZFail;
        m_state.stencil.backStencilZPass  = backZPass;
    }
}


void RenderDevice::setStencilOp(
    StencilOp           fail,
    StencilOp           zfail,
    StencilOp           zpass) {
    debugAssert(! m_inPrimitive);

    setStencilOp(fail, zfail, zpass, fail, zfail, zpass);
}


static GLenum toGLBlendEq(RenderDevice::BlendEq e) {
    switch (e) {
    case RenderDevice::BLENDEQ_MIN:
        debugAssert(GLCaps::supports("GL_EXT_blend_minmax"));
        return GL_MIN;

    case RenderDevice::BLENDEQ_MAX:
        debugAssert(GLCaps::supports("GL_EXT_blend_minmax"));
        return GL_MAX;

    case RenderDevice::BLENDEQ_ADD:
        return GL_FUNC_ADD;

    case RenderDevice::BLENDEQ_SUBTRACT:
        debugAssert(GLCaps::supports("GL_EXT_blend_subtract"));
        return GL_FUNC_SUBTRACT;

    case RenderDevice::BLENDEQ_REVERSE_SUBTRACT:
        debugAssert(GLCaps::supports("GL_EXT_blend_subtract"));
        return GL_FUNC_REVERSE_SUBTRACT;

    default:
        debugAssertM(false, "Fell through switch");
        return GL_ZERO;
    }
}


void RenderDevice::setBlendFunc(
    BlendFunc src,
    BlendFunc dst,
    BlendEq   eq) {
    debugAssert(! m_inPrimitive);

    minStateChange();
    if (src == BLEND_CURRENT) {
        src = m_state.srcBlendFunc;
    }
    
    if (dst == BLEND_CURRENT) {
        dst = m_state.dstBlendFunc;
    }

    if (eq == BLENDEQ_CURRENT) {
        eq = m_state.blendEq;
    }

    if ((m_state.dstBlendFunc != dst) ||
        (m_state.srcBlendFunc != src) ||
        (m_state.blendEq != eq)) {

        minGLStateChange();

        if ((dst == BLEND_ZERO) && (src == BLEND_ONE) && 
            ((eq == BLENDEQ_ADD) || (eq == BLENDEQ_SUBTRACT))) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendFunc(toGLBlendFunc(src), toGLBlendFunc(dst));

            debugAssert(eq == BLENDEQ_ADD ||
                GLCaps::supports("GL_EXT_blend_minmax") ||
                GLCaps::supports("GL_EXT_blend_subtract"));

            static bool supportsBlendEq = GLCaps::supports("GL_EXT_blend_minmax");

            if (supportsBlendEq && (glBlendEquationEXT != 0)) {
                glBlendEquationEXT(toGLBlendEq(eq));
            }
        }
        m_state.dstBlendFunc = dst;
        m_state.srcBlendFunc = src;
        m_state.blendEq = eq;
    }
}


void RenderDevice::setLineWidth(
    float               width) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.lineWidth != width) {
        glLineWidth(max(m_minLineWidth, width));
        minGLStateChange();
        m_state.lineWidth = width;
    }
}


void RenderDevice::setPointSize(
    float               width) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.pointSize != width) {
        glPointSize(width);
        minGLStateChange();
        m_state.pointSize = width;
    }
}


void RenderDevice::setAmbientLightColor(
    const Color4&        color) {
    debugAssert(! m_inPrimitive);

    minStateChange();
    if (color != m_state.lights.ambient) {
        m_state.lights.changed = true;
        minGLStateChange();
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color);
        m_state.lights.ambient = color;
    }
}


void RenderDevice::setAmbientLightColor(
    const Color3&        color) {
    setAmbientLightColor(Color4(color, 1.0));
}


void RenderDevice::enableLighting() {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (! m_state.lights.lighting) {
        glEnable(GL_LIGHTING);
        minGLStateChange();
        m_state.lights.lighting = true;
        m_state.lights.changed = true;
    }
}


void RenderDevice::disableLighting() {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.lights.lighting) {
        glDisable(GL_LIGHTING);
        minGLStateChange();
        m_state.lights.lighting = false;
        m_state.lights.changed = true;
    }
}


void RenderDevice::setObjectToWorldMatrix(
    const CoordinateFrame& cFrame) {
    
    minStateChange();
    debugAssert(! m_inPrimitive);

    // No test to see if it is already equal; this is called frequently and is
    // usually different.
    m_state.matrices.changed = true;
    m_state.matrices.objectToWorldMatrix = cFrame;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse * m_state.matrices.objectToWorldMatrix);
    minGLStateChange();
}


const CoordinateFrame& RenderDevice::objectToWorldMatrix() const {
    return m_state.matrices.objectToWorldMatrix;
}


void RenderDevice::setCameraToWorldMatrix(
    const CoordinateFrame& cFrame) {

    debugAssert(! m_inPrimitive);
    majStateChange();
    majGLStateChange();
    
    m_state.matrices.changed = true;
    m_state.matrices.cameraToWorldMatrix = cFrame;
    m_state.matrices.cameraToWorldMatrixInverse = cFrame.inverse();

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse * m_state.matrices.objectToWorldMatrix);

    // Reload lights since the camera matrix changed.  We must do this even for lights
    // that are not enabled since those lights will not be re-set at the GL level if they
    // are enabled without being otherwise changed.  Of course, we must follow up by disabling
    // those lights again.
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        bool wasEnabled = m_state.lights.lightEnabled[i];
        setLight(i, &m_state.lights.light[i], true);
        if (! wasEnabled) {
            setLight(i, NULL);
        }
    }
}


const CoordinateFrame& RenderDevice::cameraToWorldMatrix() const {
    return m_state.matrices.cameraToWorldMatrix;
}


Matrix4 RenderDevice::projectionMatrix() const {
    return m_state.matrices.projectionMatrix;
}


CoordinateFrame RenderDevice::modelViewMatrix() const {
    return m_state.matrices.cameraToWorldMatrixInverse * objectToWorldMatrix();
}


Matrix4 RenderDevice::modelViewProjectionMatrix() const {
    return projectionMatrix() * modelViewMatrix();
}


void RenderDevice::setProjectionMatrix(const Matrix4& P) {
    minStateChange();
    if (m_state.matrices.projectionMatrix != P) {
        m_state.matrices.projectionMatrix = P;
        m_state.matrices.changed = true;
        glMatrixMode(GL_PROJECTION);
        glLoadMatrix(P);
        glMatrixMode(GL_MODELVIEW);
        minGLStateChange();
    }
}


void RenderDevice::forceSetTextureMatrix(int unit, const double* m) {
    float f[16];
    for (int i = 0; i < 16; ++i) {
        f[i] = m[i];
    }

    forceSetTextureMatrix(unit, f);
}


void RenderDevice::forceSetTextureMatrix(int unit, const float* m) {
    minStateChange();
    minGLStateChange();

    m_state.touchedTextureUnit(unit);
    memcpy(m_state.textureUnit[unit].textureMatrix, m, sizeof(float)*16);
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }

    // Transpose the texture matrix
    float tt[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            tt[i + j * 4] = m[j + i * 4];
        }
    }
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(tt);

    const Texture::Ref& texture = m_state.textureUnit[unit].texture;

    // invert y
    if (texture.notNull() && texture->invertY) {

        float ymax = 1.0;
    
        if (texture->dimension() == Texture::DIM_2D_RECT) {
            ymax = texture->height();
        }

        float m[16] = 
        { 1,  0,  0,  0,
          0, -1,  0,  0,
          0,  0,  1,  0,
          0,  ymax,  0,  1};

        glMultMatrixf(m);
    }

}


Matrix4 RenderDevice::getTextureMatrix(uint32 unit) {
    debugAssertM((int)unit < m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    const float* M = m_state.textureUnit[unit].textureMatrix;

    return Matrix4(M[0], M[4], M[8],  M[12],
                   M[1], M[5], M[9],  M[13],
                   M[2], M[6], M[10], M[14],
                   M[3], M[7], M[11], M[15]);
}



void RenderDevice::setTextureMatrix(
    uint32               unit,
    const Matrix4&	     m) {

    float f[16];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            f[r * 4 + c] = m[r][c];
        }
    }
    
    setTextureMatrix(unit, f);
}


void RenderDevice::setTextureMatrix(
    uint32               unit,
    const double*        m) {

    debugAssert(! m_inPrimitive);
    debugAssertM((int)unit <  m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    forceSetTextureMatrix(unit, m);
}


void RenderDevice::setTextureMatrix(
    uint32              unit,
    const float*        m) {

    debugAssert(! m_inPrimitive);
    debugAssertM((int)unit <  m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    if (memcmp(m, m_state.textureUnit[unit].textureMatrix, sizeof(float)*16)) {
        forceSetTextureMatrix(unit, m);
    }
}


void RenderDevice::setTextureMatrix(
    uint32                  unit,
    const CoordinateFrame&  c) {

    float m[16] = 
    {c.rotation[0][0], c.rotation[0][1], c.rotation[0][2], c.translation.x,
     c.rotation[1][0], c.rotation[1][1], c.rotation[1][2], c.translation.y,
     c.rotation[2][0], c.rotation[2][1], c.rotation[2][2], c.translation.z,
                    0,                0,                0,               1};

    setTextureMatrix(unit, m);
}


const ImageFormat* RenderDevice::colorFormat() const {
    Framebuffer::Ref fbo = framebuffer();
    if (fbo.isNull()) {
        OSWindow::Settings settings;
        window()->getSettings(settings);
        return settings.colorFormat();
    } else {
        Framebuffer::Attachment::Ref screen = fbo->get(Framebuffer::COLOR0);
        if (screen.isNull()) {
            return NULL;
        }
        return screen->format();
    }
}


void RenderDevice::setTextureLODBias(
    uint32                  unit,
    float                   bias) {

    minStateChange();
    if (m_state.textureUnit[unit].LODBias != bias) {
        m_state.touchedTextureUnit(unit);

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }

        m_state.textureUnit[unit].LODBias = bias;

        minGLStateChange();
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, bias);
    }
}


void RenderDevice::setTextureCombineMode(
    uint32                  unit,
    const CombineMode       mode) {

    minStateChange();
	if (mode == TEX_CURRENT) {
		return;
	}

    debugAssertM((int)unit < m_numTextureUnits,
        format("Attempted to access texture unit %d on a device with %d units.",
        unit, m_numTextureUnits));

    if ((m_state.textureUnit[unit].combineMode != mode)) {
        minGLStateChange();
        m_state.touchedTextureUnit(unit);

        m_state.textureUnit[unit].combineMode = mode;

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }

        static const bool hasAdd = GLCaps::supports("GL_EXT_texture_env_add");
        static const bool hasCombine = GLCaps::supports("GL_ARB_texture_env_combine");
        static const bool hasDot3 = GLCaps::supports("GL_ARB_texture_env_dot3");

        switch (mode) {
        case TEX_REPLACE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            break;

        case TEX_BLEND:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            break;

        case TEX_MODULATE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;

        case TEX_INTERPOLATE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            break;

        case TEX_ADD:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasCombine ? GL_ADD : GL_BLEND);
            break;

        case TEX_SUBTRACT:
            // (add and subtract are in the same extension)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasAdd ? GL_SUBTRACT_ARB : GL_BLEND);
            break;

        case TEX_ADD_SIGNED:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasAdd ? GL_ADD_SIGNED_ARB : GL_BLEND);
            break;
            
        case TEX_DOT3_RGB:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasDot3 ? GL_DOT3_RGB_ARB : GL_BLEND);
            break;
             
        case TEX_DOT3_RGBA:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasDot3 ? GL_DOT3_RGBA_ARB : GL_BLEND);
            break;

        default:
            debugAssertM(false, "Unrecognized texture combine mode");
        }
    }
}


void RenderDevice::resetTextureUnit(
    uint32                  unit) {
    debugAssertM((int)unit < m_numTextureUnits,
        format("Attempted to access texture unit %d on a device with %d units.",
        unit, m_numTextureUnits));

    RenderState newState(m_state);
    m_state.textureUnit[unit] = RenderState::TextureUnit();
    m_state.touchedTextureUnit(unit);
    setState(newState);
}


void RenderDevice::setPolygonOffset(
    float              offset) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    if (m_state.polygonOffset != offset) {
        minGLStateChange();
        if (offset != 0) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(offset, sign(offset) * 2.0f);
        } else {
            glDisable(GL_POLYGON_OFFSET_POINT);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }
        m_state.polygonOffset = offset;
    }
}


void RenderDevice::setNormal(const Vector3& normal) {
    m_state.normal = normal;
    glNormal3fv(normal);
    minStateChange();
    minGLStateChange();
}


void RenderDevice::setTexCoord(uint32 unit, const Vector4& texCoord) {
    debugAssertM((int)unit < m_numTextureCoords,
        format("Attempted to access texture coordinate %d on a device with %d coordinates.",
               unit, m_numTextureCoords));

    m_state.textureUnit[unit].texCoord = texCoord;
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glMultiTexCoord(GL_TEXTURE0_ARB + unit, texCoord);
    } else {
        debugAssertM(unit == 0, "This machine has only one texture unit");
        glTexCoord(texCoord);
    }
    m_state.touchedTextureUnit(unit);
    minStateChange();
    minGLStateChange();
}


void RenderDevice::setTexCoord(uint32 unit, const Vector3& texCoord) {
    setTexCoord(unit, Vector4(texCoord, 1));
}


void RenderDevice::setTexCoord(uint32 unit, const Vector3int16& texCoord) {
    setTexCoord(unit, Vector4(texCoord.x, texCoord.y, texCoord.z, 1));
}


void RenderDevice::setTexCoord(uint32 unit, const Vector2& texCoord) {
    setTexCoord(unit, Vector4(texCoord, 0, 1));
}


void RenderDevice::setTexCoord(uint32 unit, const Vector2int16& texCoord) {
    setTexCoord(unit, Vector4(texCoord.x, texCoord.y, 0, 1));
}


void RenderDevice::setTexCoord(uint32 unit, double texCoord) {
    setTexCoord(unit, Vector4(texCoord, 0, 0, 1));
}


void RenderDevice::sendVertex(const Vector2& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex2fv(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::sendVertex(const Vector3& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex3fv(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::sendVertex(const Vector4& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex4fv(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::beginPrimitive(Primitive p) {
    debugAssertM(! m_inPrimitive, "Already inside a primitive");
    std::string why;
    debugAssertM( currentFramebufferComplete(why), why);

    beforePrimitive();
    
    m_inPrimitive = true;
    m_currentPrimitiveVertexCount = 0;
    m_currentPrimitive = p;

    debugAssertGLOk();

    glBegin(primitiveToGLenum(p));
}


void RenderDevice::endPrimitive() {
    debugAssertM(m_inPrimitive, "Call to endPrimitive() without matching beginPrimitive()");

    minStateChange(m_currentPrimitiveVertexCount);
    minGLStateChange(m_currentPrimitiveVertexCount);
    countTriangles(m_currentPrimitive, m_currentPrimitiveVertexCount);

    glEnd();
    debugAssertGLOk();
    m_inPrimitive = false;

    afterPrimitive();
}


void RenderDevice::countTriangles(RenderDevice::Primitive primitive, int numVertices) {
    switch (primitive) {
    case PrimitiveType::LINES:
        m_stats.triangles += (numVertices / 2);
        break;

    case PrimitiveType::LINE_STRIP:
        m_stats.triangles += (numVertices - 1);
        break;

    case PrimitiveType::TRIANGLES:
        m_stats.triangles += (numVertices / 3);
        break;

    case PrimitiveType::TRIANGLE_STRIP:
    case PrimitiveType::TRIANGLE_FAN:
        m_stats.triangles += (numVertices - 2);
        break;

    case PrimitiveType::QUADS:
        m_stats.triangles += ((numVertices / 4) * 2);
        break;

    case PrimitiveType::QUAD_STRIP:
        m_stats.triangles += (((numVertices / 2) - 1) * 2);
        break;

    case PrimitiveType::POINTS:
        m_stats.triangles += numVertices;
        break;
    }
}


void RenderDevice::setTexture(
    uint32                  unit,
    const Texture::Ref&     texture) {

    // NVIDIA cards have more textures than texture units.
    // "fixedFunction" textures have an associated unit 
    // and must be glEnabled.  Programmable units *cannot*
    // be enabled.
    bool fixedFunction = ((int)unit < m_numTextureUnits);

    debugAssertM(! m_inPrimitive, 
                 "Can't change textures while rendering a primitive.");

    debugAssertM((int)unit < m_numTextures,
        format("Attempted to access texture %d"
               " on a device with %d textures.",
               unit, m_numTextures));

    // early-return if the texture is already set
    if (m_state.textureUnit[unit].texture == texture) {
        return;
    }

    majStateChange();
    majGLStateChange();

    // cache old texture for texture matrix check below
    Texture::Ref oldTexture = m_state.textureUnit[unit].texture;

    // assign new texture
    m_state.textureUnit[unit].texture = texture;
    m_state.touchedTextureUnit(unit);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }

    // Turn off whatever was on previously if this is a fixed function unit
    if (fixedFunction) {
        glDisableAllTextures();
    }

    if (texture.notNull()) {
        GLint id = texture->openGLID();
        GLint target = texture->openGLTextureTarget();

        if ((GLint)currentlyBoundTexture[unit] != id) {
            glBindTexture(target, id);
            currentlyBoundTexture[unit] = id;
        }

        if (fixedFunction) {
            glEnable(target);
        }
    } else {
        // Disabled texture unit
        currentlyBoundTexture[unit] = 0;
    }

    // Force a reload of the texture matrix if invertY != old invertY.
    // This will take care of flipping the texture when necessary.
    if (oldTexture.isNull() ||
        texture.isNull() ||
        (oldTexture->invertY != texture->invertY)) {

        forceSetTextureMatrix(unit, m_state.textureUnit[unit].textureMatrix);
    }
}


double RenderDevice::getDepthBufferValue(
    int                     x,
    int                     y) const {

    GLfloat depth = 0;
    debugAssertGLOk();

    if (m_state.framebuffer.notNull()) {
        debugAssertM(m_state.framebuffer->has(FrameBuffer::DEPTH_ATTACHMENT),
            "No depth attachment");
    }

    glReadPixels(x,
	         (height() - 1) - y,
                 1, 1,
                 GL_DEPTH_COMPONENT,
	         GL_FLOAT,
	         &depth);

    debugAssertM(glGetError() != GL_INVALID_OPERATION, 
        "getDepthBufferValue failed, probably because you did not allocate a depth buffer.");

    return depth;
}


void RenderDevice::screenshotPic(GImage& dest, bool getAlpha, bool invertY) const {
    int ch = getAlpha ? 4 : 3;

    if ((dest.channels() != ch) ||
        (dest.width() != width()) ||
        (dest.height() != height())) {
        // Only resize if the current size is not correct
        dest.resize(width(), height(), ch);
    }

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width(), height(), 
        getAlpha ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, dest.byte());    

    glPopClientAttrib();

    // Flip right side up
    if (invertY) {
        dest.flipVertical();
    }
}


std::string RenderDevice::screenshot(const std::string& filepath) const {
    GImage screen;

    std::string filename = pathConcat(filepath, generateFilenameBase("", "_" + System::appName()) + ".jpg");

    screenshotPic(screen);
    screen.save(filename);

    return filename;
}


void RenderDevice::beginIndexedPrimitives() {
    debugAssert(! m_inPrimitive);
    debugAssert(! m_inIndexedPrimitive);

    // TODO: can we avoid this push?
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT); 

    m_inIndexedPrimitive = true;
}


void RenderDevice::endIndexedPrimitives() {
    debugAssert(! m_inPrimitive);
    debugAssert(m_inIndexedPrimitive);

    // Allow garbage collection of VARs
    m_tempVAR.fastClear();
    
    if (GLCaps::supports_GL_ARB_vertex_buffer_object()) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    glPopClientAttrib();
    m_inIndexedPrimitive = false;
    m_currentVARArea = NULL;
}


void RenderDevice::setVARAreaFromVAR(const class VertexRange& v) {
    debugAssert(m_inIndexedPrimitive);
    debugAssert(! m_inPrimitive);
    alwaysAssertM(m_currentVARArea.isNull() || (v.area() == m_currentVARArea), 
        "All vertex arrays used within a single begin/endIndexedPrimitive"
                  " block must share the same VertexBuffer.");

    majStateChange();

    if (v.area() != m_currentVARArea) {
        m_currentVARArea = const_cast<VertexRange&>(v).area();

        if (VertexBuffer::m_mode == VertexBuffer::VBO_MEMORY) {
            // Bind the buffer (for MAIN_MEMORY, we need do nothing)
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_currentVARArea->m_glbuffer);
            majGLStateChange();
        }
    }
}


void RenderDevice::setVARs
(const class VertexRange&  vertex, 
 const class VertexRange&  normal, 
 const class VertexRange&  color,
 const Array<VertexRange>& texCoord) {

    // Wipe old VertexBuffer
    m_currentVARArea = NULL;

    // Disable anything that is not about to be set
    debugAssertM((m_varState.highestEnabledTexCoord == 0) || GLCaps::supports_GL_ARB_multitexture(),
                 "Graphics card does not support multitexture");
    for (int i = texCoord.size(); i <= m_varState.highestEnabledTexCoord; ++i) {
        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    // Bind new
    setVertexArray(vertex);

    if (normal.size() > 0) {
        setNormalArray(normal);
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (color.size() > 0) {
        setColorArray(color);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    for (int i = 0; i < texCoord.size(); ++i) {
        setTexCoordArray(i, texCoord[i]);
        if (texCoord[i].size() > 0) {
            m_varState.highestEnabledTexCoord = i;
        }
    }
}


void RenderDevice::setVARs(const class VertexRange& vertex, const class VertexRange& normal, const class VertexRange& texCoord0,
                           const class VertexRange& texCoord1) {
    m_tempVAR.fastClear();
    if ((texCoord0.size() > 0) || (texCoord1.size() > 0)) {
        m_tempVAR.append(texCoord0, texCoord1);
    }
    setVARs(vertex, normal, VertexRange(), m_tempVAR);
}



void RenderDevice::setVertexArray(const class VertexRange& v) {
    setVARAreaFromVAR(v);
    v.vertexPointer();
}


void RenderDevice::setVertexAttribArray(unsigned int attribNum, const class VertexRange& v, bool normalize) {
    setVARAreaFromVAR(v);
    v.vertexAttribPointer(attribNum, normalize);
}


void RenderDevice::setNormalArray(const class VertexRange& v) {
    setVARAreaFromVAR(v);
    v.normalPointer();
}


void RenderDevice::setColorArray(const class VertexRange& v) {
    setVARAreaFromVAR(v);
    v.colorPointer();
}


void RenderDevice::setTexCoordArray(unsigned int unit, const class VertexRange& v) {
    if (v.size() == 0) {
        debugAssertM(GLCaps::supports_GL_ARB_multitexture() || (unit == 0),
            "Graphics card does not support multitexture");

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
        }
    } else {
        setVARAreaFromVAR(v);
        v.texCoordPointer(unit);
    }
}


MilestoneRef RenderDevice::createMilestone(const std::string& name) {
    return new Milestone(name);
}


void RenderDevice::setMilestone(const MilestoneRef& m) {
    minStateChange();
    minGLStateChange();
    m->set();
}


void RenderDevice::waitForMilestone(const MilestoneRef& m) {
    minStateChange();
    minGLStateChange();
    m->wait();
}


void RenderDevice::setLight(int i, const GLight& light) {
    
    setLight(i, &light, false);
}


void RenderDevice::setLight(int i, void* x) {
    debugAssert(x == NULL); (void)x;
    setLight(i, NULL, false);
}


void RenderDevice::setLight(int i, const GLight* _light, bool force) {
    debugAssert(i >= 0);
    debugAssert(i < MAX_LIGHTS);
    int gi = GL_LIGHT0 + i;

    const GLight& light = *_light;


    minStateChange();

    if (_light == NULL) {

        if (m_state.lights.lightEnabled[i] || force) {
            // Don't bother copying this light over
            m_state.lights.lightEnabled[i] = false;
            m_state.lights.changed = true;
            glDisable(gi);
        }

        minGLStateChange();
    
    } else {

        for (int j = 0; j < 3; ++j) {
            debugAssert(light.attenuation[j] >= 0);
        }

    
        if (! m_state.lights.lightEnabled[i] || force) {
            glEnable(gi);
            m_state.lights.lightEnabled[i] = true;
            m_state.lights.changed = true;
        }

    
        if ((m_state.lights.light[i] != light) || force) {
            m_state.lights.changed = true;
            m_state.lights.light[i] = light;

            minGLStateChange();

            Color4 zero(0, 0, 0, 1);
            Color4 brightness(light.color, 1);

            // Calling glGetInteger causes SLI GPUs to stall 
            //int mm = glGetInteger(GL_MATRIX_MODE);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
                glLoadIdentity();
                glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse);
                glLightfv(gi, GL_POSITION,              light.position);
                glLightfv(gi, GL_SPOT_DIRECTION,        light.spotDirection);
                glLightf (gi, GL_SPOT_CUTOFF,           light.spotCutoff);
                glLightfv(gi, GL_AMBIENT,               zero);
                if (light.diffuse) {
                    glLightfv(gi, GL_DIFFUSE,               brightness);
                } else {
                    glLightfv(gi, GL_DIFFUSE,               zero);
                }
                if (light.specular) {
                    glLightfv(gi, GL_SPECULAR,              brightness);
                } else {
                    glLightfv(gi, GL_SPECULAR,              zero);
                }
                glLightf (gi, GL_CONSTANT_ATTENUATION,  light.attenuation[0]);
                glLightf (gi, GL_LINEAR_ATTENUATION,    light.attenuation[1]);
                glLightf (gi, GL_QUADRATIC_ATTENUATION, light.attenuation[2]);
            glPopMatrix();
            //glMatrixMode(mm);
        }    
    }
    
}


void RenderDevice::configureShadowMap(
    uint32              unit,
    const ShadowMap::Ref& shadowMap) {
    configureShadowMap(unit, shadowMap->lightMVP(), shadowMap->depthTexture());
}


void RenderDevice::configureShadowMap(
    uint32              unit,
    const Matrix4&      lightMVP,
    const Texture::Ref& shadowMap) {

    minStateChange();
    minGLStateChange();

    // http://www.nvidia.com/dev_content/nvopenglspecs/GL_ARB_shadow.txt

    debugAssertM(shadowMap->format()->openGLBaseFormat == GL_DEPTH_COMPONENT,
        "Can only configure shadow maps from depth textures");

    debugAssertM(shadowMap->settings().depthReadMode != Texture::DEPTH_NORMAL,
        "Shadow maps must be configured for either Texture::DEPTH_LEQUAL"
        " or Texture::DEPTH_GEQUAL comparisions.");

    debugAssertM(GLCaps::supports_GL_ARB_shadow(),
        "The device does not support shadow maps");


    // Texture coordinate generation will use the current MV matrix
    // to determine OpenGL "eye" space.  We want OpenGL "eye" space to
    // be our world space, so set the object to world matrix to be the
    // identity.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse);

    setTexture(unit, shadowMap);
    
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }
    
    const Matrix4& textureMatrix = m_state.textureUnit[unit].textureMatrix;

	const Matrix4& textureProjectionMatrix2D =
        textureMatrix  * lightMVP;

	// Set up tex coord generation - all 4 coordinates required
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, textureProjectionMatrix2D[0]);
	glEnable(GL_TEXTURE_GEN_S);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, textureProjectionMatrix2D[1]);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_R, GL_EYE_PLANE, textureProjectionMatrix2D[2]);
	glEnable(GL_TEXTURE_GEN_R);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_Q, GL_EYE_PLANE, textureProjectionMatrix2D[3]);
	glEnable(GL_TEXTURE_GEN_Q);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void RenderDevice::configureReflectionMap(
    uint32              textureUnit,
    Texture::Ref        reflectionTexture) {

    debugAssert(! GLCaps::hasBug_normalMapTexGen());
    debugAssert(reflectionTexture->dimension() == Texture::DIM_CUBE_MAP);

    // Texture coordinates will be generated in object space.
    // Set the texture matrix to transform them into camera space.
    CoordinateFrame cframe = cameraToWorldMatrix();

    // The environment map assumes we are always in the center,
    // so zero the translation.
    cframe.translation = Vector3::zero();

    // The environment map is in world space.  The reflection vector
    // will automatically be multiplied by the object->camera space 
    // transformation by hardware (just like any other vector) so we 
    // take it back from camera space to world space for the correct
    // vector.
    setTexture(textureUnit, reflectionTexture);

    setTextureMatrix(textureUnit, cframe);

    minStateChange();
    minGLStateChange();
    glActiveTextureARB(GL_TEXTURE0_ARB + textureUnit);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
}


void RenderDevice::sendSequentialIndices
(RenderDevice::Primitive primitive, int numVertices) {

    beforePrimitive();

    glDrawArrays(primitiveToGLenum(primitive), 0, numVertices);
    // Mark all active arrays as busy.
    setVARAreaMilestone();

    countTriangles(primitive, numVertices);
    afterPrimitive();

    minStateChange();
    minGLStateChange();
}


void RenderDevice::sendSequentialIndicesInstanced
(RenderDevice::Primitive primitive, int numVertices, int numInstances) {

    beforePrimitive();

    glDrawArraysInstancedARB(primitiveToGLenum(primitive), 0, numVertices, numInstances);
    // Mark all active arrays as busy.
    setVARAreaMilestone();

    countTriangles(primitive, numVertices * numInstances);
    afterPrimitive();

    minStateChange();
    minGLStateChange();
}


void RenderDevice::sendIndices
(RenderDevice::Primitive primitive, const VertexRange& indexVAR) {
    sendIndices(primitive, indexVAR, 1, false);
}


void RenderDevice::sendIndicesInstanced
(RenderDevice::Primitive primitive, const VertexRange& indexVAR, int numInstances) {
    sendIndices(primitive, indexVAR, numInstances, true);
}


void RenderDevice::sendIndices
(RenderDevice::Primitive primitive, const VertexRange& indexVAR,
 int numInstances, bool useInstances) {

    std::string why;
    debugAssertM(currentFramebufferComplete(why), why);

    if (indexVAR.m_numElements == 0) {
        // There's nothing in this index array, so don't bother rendering.
        return;
    }

    debugAssertM(indexVAR.area().notNull(), "Corrupt VertexRange");
    debugAssertM(indexVAR.type() == VertexBuffer::INDEX, "Must be an index VertexRange");

    // Calling glGetInteger triggers a stall on SLI GPUs
    //int old = glGetInteger(GL_ELEMENT_ARRAY_BUFFER_BINDING);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indexVAR.area()->openGLVertexBufferObject());

    internalSendIndices(primitive, indexVAR.m_elementSize, indexVAR.m_numElements, 
                        indexVAR.pointer(), numInstances, useInstances);

    // Set the milestone on the current area
    {
        indexVAR.area()->m_renderDevice = this;

        if (indexVAR.area()->m_mode != VertexBuffer::VBO_MEMORY) {
            MilestoneRef milestone = createMilestone("VertexRange Milestone");
            setMilestone(milestone);

            // Overwrite any preexisting milestone
            indexVAR.area()->milestone = milestone;
        }
    }

    // Mark all active arrays as busy.
    setVARAreaMilestone();
    
    countTriangles(primitive, indexVAR.m_numElements * numInstances);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void RenderDevice::internalSendIndices(
    RenderDevice::Primitive primitive,
    int                     indexSize, 
    int                     numIndices, 
    const void*             index,
    int                     numInstances,
    bool                    useInstances) {

    beforePrimitive();

    GLenum i;
    
    switch (indexSize) {
    case sizeof(uint32):
        i = GL_UNSIGNED_INT;
        break;
    
    case sizeof(uint16):
        i = GL_UNSIGNED_SHORT;
        break;
    
    case sizeof(uint8):
        i = GL_UNSIGNED_BYTE;
        break;
    
    default:
        debugAssertM(false, "Indices must be either 8, 16, or 32-bytes each.");
        i = 0;
    }
    
    GLenum p = primitiveToGLenum(primitive);
    
    if (useInstances) {
         glDrawElementsInstancedARB(p, numIndices, i, index, numInstances);
    } else {
        glDrawElements(p, numIndices, i, index);
    }
        
    afterPrimitive();
}


bool RenderDevice::supportsTwoSidedStencil() const {
    return GLCaps::supports_GL_EXT_stencil_two_side();
}


bool RenderDevice::supportsTextureRectangle() const {
    return GLCaps::supports_GL_EXT_texture_rectangle();
}


bool RenderDevice::supportsVertexProgramNV2() const {
    return GLCaps::supports_GL_NV_vertex_program2();
}


bool RenderDevice::supportsVertexBufferObject() const { 
    return GLCaps::supports_GL_ARB_vertex_buffer_object();
}


std::string RenderDevice::dummyString;
bool RenderDevice::checkFramebuffer(std::string& whyNot) const {
    GLenum status;
    status = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

    switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        whyNot = "Framebuffer Incomplete: Incomplete Attachment.";
		break;

    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        whyNot = "Unsupported framebuffer format.";
		break;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        whyNot = "Framebuffer Incomplete: Missing attachment.";
		break;

    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        whyNot = "Framebuffer Incomplete: Attached images must have same dimensions.";
		break;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        whyNot = "Framebuffer Incomplete: Attached images must have same format.";
		break;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        whyNot = "Framebuffer Incomplete: Missing draw buffer.";
		break;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        whyNot = "Framebuffer Incomplete: Missing read buffer.";
		break;

    default:
        whyNot = "Framebuffer Incomplete: Unknown error.";
    }

    return false;    
}


/////////////////////////////////////////////////////////////////////////////////////

RenderDevice::Stats::Stats() {
    smoothTriangles = 0;
    smoothTriangleRate = 0;
    smoothFrameRate = 0;
    reset();
}

void RenderDevice::Stats::reset() {
    minorStateChanges = 0;
    minorOpenGLStateChanges = 0;
    majorStateChanges = 0;
    majorOpenGLStateChanges = 0;
    pushStates = 0;
    primitives = 0;
    triangles = 0;
    swapbuffersTime = 0;
    frameRate = 0;
    triangleRate = 0;
}

/////////////////////////////////////////////////////////////////////////////////////


static void var(TextOutput& t, const std::string& name, const std::string& val) {
    t.writeSymbols(name,"=");
    t.writeString(val);
    t.writeNewline();
}


static void var(TextOutput& t, const std::string& name, const bool val) {
    t.writeSymbols(name, "=", val ? "Yes" : "No");
    t.writeNewline();
}


static void var(TextOutput& t, const std::string& name, const int val) {
    t.writeSymbols(name,"=");
    t.writeNumber(val);
    t.writeNewline();
}


void RenderDevice::describeSystem(
    TextOutput& t) {

    t.writeSymbols("GPU", "{");
    t.writeNewline();
    t.pushIndent();
        var(t, "Chipset", GLCaps::renderer());
        var(t, "Vendor", GLCaps::vendor());
        var(t, "Driver", GLCaps::driverVersion());
        var(t, "OpenGL version", GLCaps::glVersion());
        var(t, "Textures", GLCaps::numTextures());
        var(t, "Texture coordinates", GLCaps::numTextureCoords());
        var(t, "Texture units", GLCaps::numTextureUnits());
        var(t, "GL_MAX_TEXTURE_SIZE", glGetInteger(GL_MAX_TEXTURE_SIZE));
        var(t, "GL_MAX_COLOR_ATTACHMENTS_EXT", glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT));
    t.popIndent();
    t.writeSymbols("}");
    t.writeNewline();
    t.writeNewline();

    OSWindow* w = window();
    OSWindow::Settings settings;
    w->getSettings(settings);

    t.writeSymbols("Window", "{");
    t.writeNewline();
    t.pushIndent();
        var(t, "API", w->getAPIName());
        var(t, "Version", w->getAPIVersion());
        t.writeNewline();

        var(t, "In focus", w->hasFocus());
        var(t, "Centered", settings.center);
        var(t, "Framed", settings.framed);
        var(t, "Visible", settings.visible);
        var(t, "Resizable", settings.resizable);
        var(t, "Full screen", settings.fullScreen);
        var(t, "Top", settings.y);
        var(t, "Left", settings.x);
        var(t, "Width", settings.width);
        var(t, "Height", settings.height);
        var(t, "Refresh rate", settings.refreshRate);
        t.writeNewline();

        var(t, "Alpha bits", settings.alphaBits);
        var(t, "Red bits", settings.rgbBits);
        var(t, "Green bits", settings.rgbBits);
        var(t, "Blue bits", settings.rgbBits);
        var(t, "Depth bits", settings.depthBits);
        var(t, "Stencil bits", settings.stencilBits);
        var(t, "Asynchronous", settings.asynchronous);
        var(t, "Stereo", settings.stereo);
        var(t, "FSAA samples", settings.fsaaSamples);

    t.popIndent();
    t.writeSymbols("}");
    t.writeNewline();
    t.writeNewline();
}

} // namespace


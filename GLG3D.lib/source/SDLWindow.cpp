/**
  @file SDLWindow.cpp

  @maintainer Morgan McGuire, morgan@graphics3d.com
  @created 2004-02-10
  @edited  2008-01-01
*/

#include "G3D/platform.h"

#if defined(G3D_LINUX) || defined(G3D_FREEBSD)

#include "G3D/Log.h"
#include "G3D/Rect2D.h"
#include "GLG3D/SDLWindow.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"

#if defined(G3D_OSX) 
#   include <SDL/SDL.h>
#   include <SDL/SDL_syswm.h>
#elif defined(G3D_FREEBSD)
#   include <SDL/SDL.h>
#   include <SDL/SDL_syswm.h>
#elif defined(G3D_LINUX)
#   include <SDL/SDL.h>
#   include <SDL/SDL_syswm.h>
#elif defined(G3D_WIN32)
#   include <SDL.h>
#   include <SDL_syswm.h>
#endif

#ifdef _MSC_VER
    // GetSystemMetrics parameters missing in header files
    #ifndef SM_XVIRTUALSCREEN
    #define SM_XVIRTUALSCREEN       76
    #endif
    #ifndef SM_YVIRTUALSCREEN
    #define SM_YVIRTUALSCREEN       77
    #endif
    #ifndef SM_CXVIRTUALSCREEN
    #define SM_CXVIRTUALSCREEN      78
    #endif
    #ifndef SM_CYVIRTUALSCREEN
    #define SM_CYVIRTUALSCREEN      79
    #endif
    #ifndef SM_CMONITORS
    #define SM_CMONITORS            80
    #endif
    #ifndef SM_SAMEDISPLAYFORMAT
    #define SM_SAMEDISPLAYFORMAT    81
    #endif
#endif

#define SDL_FSAA (SDL_MAJOR_VERSION * 100 + SDL_MINOR_VERSION * 10 + SDL_PATCHLEVEL > 125)

namespace G3D {

#ifdef G3D_WIN32
int screenWidth() {
    return GetSystemMetrics(SM_CXFULLSCREEN);
    /*
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);

    if (w == 0) {
        // This call is not supported on older versions of windows
        return GetSystemMetrics(SM_CXFULLSCREEN);
    } else {
        return w;
    }
    */
}


int screenHeight() {
    return GetSystemMetrics(SM_CYFULLSCREEN);
    /*
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (h == 0) {
        // This call is not supported on older versions of windows
        return GetSystemMetrics(SM_CYFULLSCREEN);
    } else {
        return h;
    }
    */
}
#endif


#ifdef G3D_LINUX

int screenWidth(Display* display) {
	const int screenNumber = DefaultScreen(display);
	return DisplayWidth(display, screenNumber);
}

int screenHeight(Display* display) {
	const int screenNumber = DefaultScreen(display);
	return DisplayHeight(display, screenNumber);
}

#endif


#if defined(G3D_LINUX)

/** Replacement for the default assertion hook on Linux. */
static bool SDL_handleDebugAssert_(
    const char* expression,
    const std::string& message,
    const char* filename,
    int         lineNumber,
    bool&       ignoreAlways,
    bool        useGuiPrompt) {

    SDL_ShowCursor(SDL_ENABLE);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
	
    return _internal::_handleDebugAssert_(expression, message, filename, lineNumber, ignoreAlways, useGuiPrompt);
}

/** Replacement for the default failure hook on Linux. */
static bool SDL_handleErrorCheck_(
    const char* expression,
    const std::string& message,
    const char* filename,
    int         lineNumber,
    bool&       ignoreAlways,
    bool        useGuiPrompt) {

    SDL_ShowCursor(SDL_ENABLE);
    SDL_WM_GrabInput(SDL_GRAB_OFF);

    return _internal::_handleErrorCheck_(expression, message, filename, lineNumber, ignoreAlways, useGuiPrompt);
}
#endif


SDLWindow* SDLWindow::create(const OSWindow::Settings& settings) {

    return new SDLWindow(settings);    

}


SDLWindow::SDLWindow(const OSWindow::Settings& settings) {

    m_settings = settings;

#   if defined(G3D_OSX)
        NSApplicationWrapper wrapper;

	// Hack to get our window/process to the front...
	ProcessSerialNumber psn = { 0, kCurrentProcess};    
	TransformProcessType (&psn, kProcessTransformToForegroundApplication);
	SetFrontProcess (&psn);

	_pool = new NSAutoreleasePoolWrapper();
#   endif

    if (SDL_Init(SDL_INIT_NOPARACHUTE | 
		 SDL_INIT_VIDEO | 
		 SDL_INIT_JOYSTICK) < 0 ) {

        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
	    debugPrintf("Unable to initialize SDL: %s\n", SDL_GetError());
	    Log::common()->printf("Unable to initialize SDL: %s\n", SDL_GetError());
	    exit(1);
    }
    
    // Set default icon if available
    if (!m_settings.defaultIconFilename.empty()) {
        try {
	        GImage defaultIcon;
	        defaultIcon.load(m_settings.defaultIconFilename);
	
	        setIcon(defaultIcon);
        } catch (const GImage::Error& e) {
	        logPrintf("OSWindow's default icon failed to load: %s (%s)", 
	        e.filename.c_str(), e.reason.c_str());            
        }
    }

    if (! m_settings.fullScreen) {
        // This doesn't really work very well due to SDL bugs so we fix up 
        // the position after the window is created.
        if (m_settings.center) {
	        System::setEnv("SDL_VIDEO_CENTERED", "");
	    } else {
	        System::setEnv("SDL_VIDEO_WINDOW_POS", format("%d,%d", m_settings.x, m_settings.y));
        }
    }

    m_mouseVisible = true;
    m_inputCapture = false;

    // Request various OpenGL parameters
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,      m_settings.depthBits);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,    1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,    m_settings.stencilBits);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,        m_settings.rgbBits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,      m_settings.rgbBits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,       m_settings.rgbBits);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,      m_settings.alphaBits);
    SDL_GL_SetAttribute(SDL_GL_STEREO,          m_settings.stereo);
    
    #if SDL_FSAA
        if (m_settings.fsaaSamples > 1) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 
                                m_settings.fsaaSamples);
        }
    #endif

    // Create a width x height OpenGL screen 
    int flags = 
        (m_settings.hardware ? SDL_HWSURFACE : 0) |
        SDL_OPENGL |
        (m_settings.fullScreen ? SDL_FULLSCREEN : 0) |
        (m_settings.resizable ? SDL_RESIZABLE : 0) |
        (m_settings.framed ? 0 : SDL_NOFRAME);

    if (SDL_SetVideoMode(m_settings.width, m_settings.height, 0, flags) == NULL) {
        // Try dropping down to 16-bit depth buffer, the most
        // common problem on X11
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

        if (SDL_SetVideoMode(m_settings.width, m_settings.height, 0, flags) == NULL) {
            logPrintf("SDLWindow was unable to create an OpenGL screen: %s\n", 
		    SDL_GetError());
	        logPrintf(" width       = %d\n", m_settings.width);
	        logPrintf(" height      = %d\n", m_settings.height);
	        logPrintf(" fullscreen  = %d\n", m_settings.fullScreen);
	        logPrintf(" resizable   = %d\n", m_settings.resizable);
	        logPrintf(" framed      = %d\n", m_settings.framed);
	        logPrintf(" rgbBits     = %d\n", m_settings.rgbBits);
	        logPrintf(" alphaBits   = %d\n", m_settings.alphaBits);
	        logPrintf(" depthBits   = %d\n", m_settings.depthBits);
	        logPrintf(" stencilBits = %d\n", m_settings.stencilBits);
            alwaysAssertM(false, "Unable to create OpenGL screen");
	        exit(-3);
        }
    }

    // See what video mode we really got
    int depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

    glGetIntegerv(GL_RED_BITS,   &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS,  &blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
    int actualFSAABuffers = 0, actualFSAASamples = 0;

    #if SDL_FSAA
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &actualFSAABuffers);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &actualFSAASamples);
    #else
        (void)actualFSAABuffers;
        (void)actualFSAASamples;
    #endif
    m_settings.rgbBits     = iMin(iMin(redBits, greenBits), blueBits);
    m_settings.alphaBits   = alphaBits;
    m_settings.stencilBits = stencilBits;
    m_settings.depthBits   = depthBits;
    m_settings.fsaaSamples = actualFSAASamples;

    SDL_version ver;
    SDL_VERSION(&ver);
    m_version = format("%d,%0d.%0d", ver.major, ver.minor, ver.patch);

    SDL_EnableUNICODE(1);
    setCaption(settings.caption);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);

    m_glContext = glGetCurrentContext();

    #if defined(G3D_WIN32)
        // Extract SDL HDC/HWND on Win32
        _Win32HWND  = info.window;
        _Win32HDC   = wglGetCurrentDC();
    #elif defined(G3D_LINUX)
        // Extract SDL's internal Display pointer on Linux        
        m_X11Display = info.info.x11.display;
        m_X11Window  = info.info.x11.window;
        m_X11WMWindow  = info.info.x11.wmwindow;

        G3D::_internal::x11Display = glXGetCurrentDisplay();

        // A Drawable appears to be either a Window or a Pixmap
        G3D::_internal::x11Window  = glXGetCurrentDrawable();
    #endif

    // Adjust window position
    #ifdef G3D_WIN32
        if (! settings.fullScreen) {
            int W = screenWidth();
            int H = screenHeight();
            int x = iClamp(settings.x, 0, W);
            int y = iClamp(settings.y, 0, H);

            if (settings.center) {
                x = (W - settings.width) / 2;
                y = (H - settings.height) / 2;
            }

            SetWindowPos(_Win32HWND, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        }
    #endif

#   ifdef G3D_LINUX
	if (! settings.fullScreen) {
	    int W = screenWidth(m_X11Display);
	    int H = screenHeight(m_X11Display);
	    int x = iClamp(settings.x, 0, W);
	    int y = iClamp(settings.y, 0, H);
	  
        if (settings.center) {
            x = (W  - settings.width) / 2;
	        y = (H - settings.height) / 2;
	    }
	    XMoveWindow(m_X11Display, m_X11WMWindow, x, y);
    }
#    endif


    // Check for joysticks
    int j = SDL_NumJoysticks();
    if ((j < 0) || (j > 10)) {
        // If there is no joystick adapter on Win32,
        // SDL returns ridiculous numbers.
        j = 0;
    }

    if (j > 0) {
        SDL_JoystickEventState(SDL_ENABLE);
	    // Turn on the joysticks

        m_joy.resize(j);
        for (int i = 0; i < j; ++i) {
	        m_joy[i] = SDL_JoystickOpen(i);
	        debugAssert(m_joy[i]);
        }
    }

    GLCaps::init();

    // Register this window as the current window
    makeCurrent();

#   if defined(G3D_LINUX)
        // If G3D is using the default assertion hooks, replace them with
        // our own that use SDL functions to release the mouse, since
        // we've been unable to implement a non-SDL way of releasing the
        // mouse using the X11 handle directly.
        if (assertionHook() == _internal::_handleDebugAssert_) {
	        setFailureHook(SDL_handleDebugAssert_);
	    }

	    if (failureHook() == _internal::_handleErrorCheck_) {
            setFailureHook(SDL_handleErrorCheck_);
	    }
#   endif
}



SDLWindow::~SDLWindow() {
    // Close joysticks, if opened
    for (int j = 0; j < m_joy.size(); ++j) {
        SDL_JoystickClose(m_joy[j]);
    }

    // Release the mouse
    setMouseVisible(true);
    setInputCapture(false);

    m_joy.clear();

    SDL_Quit();

#if defined(G3D_OSX)
	delete _pool;
#endif
}


::SDL_Joystick* SDLWindow::getSDL_Joystick(unsigned int num) const {
    if ((unsigned int)m_joy.size() >= num) {
        return m_joy[num];
    } else {
        return NULL;
    }
}


void SDLWindow::getDroppedFilenames(Array<std::string>& files) {
    files.fastClear();
}


void SDLWindow::getSettings(OSWindow::Settings& settings) const {
    settings = m_settings;
}


int SDLWindow::width() const {
    return m_settings.width;
}


int SDLWindow::height() const {
    return m_settings.height;
}


Rect2D SDLWindow::dimensions() const {
    return Rect2D::xywh(0, 0, m_settings.width, m_settings.height);;
}


void SDLWindow::setDimensions(const Rect2D& dims) {
#   ifdef G3D_WIN32
        int W = screenWidth();
        int H = screenHeight();

        int x = iClamp((int)dims.x0(), 0, W);
        int y = iClamp((int)dims.y0(), 0, H);
        int w = iClamp((int)dims.width(), 1, W);
        int h = iClamp((int)dims.height(), 1, H);
        SetWindowPos(_Win32HWND, NULL, x, y, w, h, SWP_NOZORDER);
        // Do not update settings-- wait for an event to notify us
#    endif

#    ifdef G3D_LINUX
        int W = screenWidth(m_X11Display);
        int H = screenHeight(m_X11Display);

        int x = iClamp((int)dims.x0(), 0, W);
        int y = iClamp((int)dims.y0(), 0, H);
        int w = iClamp((int)dims.width(), 1, W);
        int h = iClamp((int)dims.height(), 1, H);
        XMoveResizeWindow(m_X11Display, m_X11WMWindow, x, y, w, h);
#    endif 

    // TODO: OS X
}


void SDLWindow::setPosition(int x, int y) {
    #ifdef G3D_WIN32
        int W = screenWidth();
        int H = screenHeight();

        x = iClamp(x, 0, W);
        y = iClamp(y, 0, H);

        SetWindowPos(_Win32HWND, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        // Do not update settings-- wait for an event to notify us
    #endif

	#ifdef G3D_LINUX
	    const int W = screenWidth(m_X11Display);
        const int H = screenHeight(m_X11Display);

        x = iClamp(x, 0, W);
        y = iClamp(y, 0, H);

	    XMoveWindow(m_X11Display, m_X11WMWindow, x, y);
	#endif
    // TODO: OS X
}


bool SDLWindow::hasFocus() const {
    uint8 s = SDL_GetAppState();
    
    return ((s & SDL_APPMOUSEFOCUS) != 0) &&
           ((s & SDL_APPINPUTFOCUS) != 0) &&
           ((s & SDL_APPACTIVE) != 0);
}


std::string SDLWindow::getAPIVersion() const {
    return m_version;
}


std::string SDLWindow::getAPIName() const {
    return "SDL";
}


void SDLWindow::setGammaRamp(const Array<uint16>& gammaRamp) {
    alwaysAssertM(gammaRamp.size() >= 256, "Gamma ramp must have at least 256 entries");

    Log* debugLog = Log::common();

    uint16* ptr = const_cast<uint16*>(gammaRamp.getCArray());
    #ifdef WIN32
        // On windows, use the more reliable SetDeviceGammaRamp function.
        // It requires separate RGB gamma ramps.
        uint16 wptr[3 * 256];
        for (int i = 0; i < 256; ++i) {
            wptr[i] = wptr[i + 256] = wptr[i + 512] = ptr[i]; 
        }
        BOOL success = SetDeviceGammaRamp(wglGetCurrentDC(), wptr);
    #else
        bool success = (SDL_SetGammaRamp(ptr, ptr, ptr) != -1);
    #endif

    if (! success) {
        if (debugLog) {debugLog->println("Error setting gamma ramp!");}

        #ifdef WIN32
            debugAssertM(false, "Failed to set gamma ramp");
        #else
            if (debugLog) {debugLog->println(SDL_GetError());}
            debugAssertM(false, SDL_GetError());
        #endif
    }
}


int SDLWindow::numJoysticks() const {
    return m_joy.size();
}


void SDLWindow::getJoystickState(
    unsigned int    stickNum,
    Array<float>&   axis,
    Array<bool>&    button) {

    debugAssert(stickNum < ((unsigned int) m_joy.size()));

    ::SDL_Joystick* sdlstick = m_joy[stickNum];

    axis.resize(SDL_JoystickNumAxes(sdlstick), DONT_SHRINK_UNDERLYING_ARRAY);

    for (int a = 0; a < axis.size(); ++a) {
        axis[a] = SDL_JoystickGetAxis(sdlstick, a) / 32768.0;
    }

    button.resize(SDL_JoystickNumButtons(sdlstick), DONT_SHRINK_UNDERLYING_ARRAY);

    for (int b = 0; b < button.size(); ++b) {
        button[b] = (SDL_JoystickGetButton(sdlstick, b) != 0);
    }
}


std::string SDLWindow::joystickName(unsigned int sticknum) {
    debugAssert(sticknum < ((unsigned int) m_joy.size()));
    return SDL_JoystickName(sticknum);
}


void SDLWindow::setCaption(const std::string& caption) {
    m_caption = caption;
    SDL_WM_SetCaption(m_caption.c_str(), NULL);
}


std::string SDLWindow::caption() {
    return m_caption;
}


void SDLWindow::setIcon(const GImage& image) {
    alwaysAssertM((image.channels == 3) ||
                  (image.channels == 4), 
                  "Icon image must have at least 3 channels.");

    #ifdef G3D_WIN32
        alwaysAssertM((image.width == 32) && (image.height == 32),
            "Icons must be 32x32 on windows.");
    #endif

    uint32 amask = 0xFF000000;
    uint32 bmask = 0x00FF0000;
    uint32 gmask = 0x0000FF00;
    uint32 rmask = 0x000000FF;

    if (image.channels == 3) {
        // Take away the 4th channel.
        amask = 0x00000000;
    }

    int pixelBitLen     = image.channels * 8;
    int scanLineByteLen = image.channels * image.width;

    SDL_Surface* surface =
        SDL_CreateRGBSurfaceFrom((void*)image.byte(), image.width, image.height,
        pixelBitLen, scanLineByteLen, 
        rmask, gmask, bmask, amask);

    alwaysAssertM((surface != NULL),
        "Icon data failed to load into SDL.");

    // Let SDL create mask from image data directly
    SDL_WM_SetIcon(surface, NULL);

    SDL_FreeSurface(surface);
}


void SDLWindow::swapGLBuffers() {
    SDL_GL_SwapBuffers();
}


void SDLWindow::notifyResize(int w, int h) {
    debugAssert(w > 0);
    debugAssert(h > 0);
    m_settings.width = w;
    m_settings.height = h;

	// Mutate the SDL surface (which one is not supposed to do).
	// We can't resize the actual surface or SDL will destroy
	// our GL context, however.
	SDL_Surface* surface = SDL_GetVideoSurface();
	surface->w = w;
	surface->h = h;
	surface->clip_rect.x = 0;
	surface->clip_rect.y = 0;
	surface->clip_rect.w = w;
	surface->clip_rect.h = h;
}


void SDLWindow::setRelativeMousePosition(double x, double y) {
    SDL_WarpMouse(iRound(x), iRound(y));
}


void SDLWindow::setRelativeMousePosition(const Vector2& p) {
    setRelativeMousePosition(p.x, p.y);
}


void SDLWindow::getRelativeMouseState(Vector2& p, uint8& mouseButtons) const {
    int x, y;
    getRelativeMouseState(x, y, mouseButtons);
    p.x = x;
    p.y = y;
}


void SDLWindow::getRelativeMouseState(int& x, int& y, uint8& mouseButtons) const {
    mouseButtons = SDL_GetMouseState(&x, &y);
}


void SDLWindow::getRelativeMouseState(double& x, double& y, uint8& mouseButtons) const {
    int ix, iy;
    getRelativeMouseState(ix, iy, mouseButtons);
    x = ix;
    y = iy;
}


void SDLWindow::setMouseVisible(bool v) {
    m_mouseHideCount = v ? 0 : 1;

    if (v) {
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }

    m_mouseVisible = v;
}


void SDLWindow::setInputCapture(bool c) {
    m_inputCaptureCount = c ? 1 : 0;

    if (m_inputCapture != c) {
        m_inputCapture = c;

        if (m_inputCapture) {
            SDL_WM_GrabInput(SDL_GRAB_ON);
        } else {
            SDL_WM_GrabInput(SDL_GRAB_OFF);
        }
    }
}
    

void SDLWindow::getOSEvents(Queue<GEvent>& events) {
    // Note that GEvent conveniently has exactly the same memory layout
    // as SDL_Event.
    GEvent e;
    bool hadEvent = (SDL_PollEvent(reinterpret_cast<SDL_Event*>(&e)) != 0);
    if (hadEvent) {
        if ((e.type == GEventType::MOUSE_BUTTON_UP) ||
            (e.type == GEventType::MOUSE_BUTTON_DOWN)) {
            // SDL mouse buttons are off by one from our convention
            --e.button.button;
        }
        events.pushBack(e);
    }
}


#if defined(G3D_LINUX) || defined(G3D_FREEBSD)

Window SDLWindow::x11Window() const {
    return m_X11Window;
}


Display* SDLWindow::x11Display() const {
    return m_X11Display;
}

#elif defined(G3D_WIN32)

HDC SDLWindow::win32HDC() const {
    return _Win32HDC;
}


HWND SDLWindow::win32HWND() const {
    return _Win32HWND;
}

#endif

void SDLWindow::reallyMakeCurrent() const {
#   ifdef G3D_WIN32
        if (wglMakeCurrent(_Win32HDC, m_glContext) == FALSE)	{
            debugAssertM(false, "Failed to set context");
	}
#   elif defined(G3D_LINUX)
        if (! glXMakeCurrent(m_X11Display, m_X11Window, m_glContext)) {
            //debugAssertM(false, "Failed to set context");
            // only check OpenGL as False seems to be returned when
            // context is already current
            debugAssertGLOk();
        }
#   elif defined(G3D_OSX)
        
#   endif
}

} // namespace

#endif // ifndef G3D_WIN32

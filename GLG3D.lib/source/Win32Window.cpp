/**
@file Win32Window.cpp

@maintainer Morgan McGuire
@cite       Written by Corey Taylor & Morgan McGuire
@cite       Special thanks to Max McGuire of Ironlore
@created 	  2004-05-21
@edited  	  2009-04-05

Copyright 2000-2009, Morgan McGuire.
All rights reserved.
*/

#include "G3D/platform.h"
#include "GLG3D/GLCaps.h"

// This file is ignored on other platforms
#ifdef G3D_WIN32

#include "G3D/Log.h"
#include "GLG3D/Win32Window.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/UserInput.h"
#include "directinput8.h"
#include <Winuser.h>
#include <windowsx.h>
#include <shellapi.h> // for drag and drop

#include <time.h>
#include <sstream>
#include <crtdbg.h>

#include "GLG3D/GApp.h" // for screenPrintf

using G3D::_internal::_DirectInput;

/*
DirectInput8 support is added by loading dinupt8.dll if found.

COM calls are not used to limit the style of code and includes needed.
DirectInput8 has a special creation function that lets us do this properly.

The joystick state structure used simulates the exports found in dinput8.lib

The joystick axis order returned to users is: X, Y, Z, Slider1, Slider2, rX, rY, rZ.

Important:  The cooperation level of Win32Window joysticks is Foreground:Non-Exclusive.
This means that other programs can get access to the joystick(preferably non-exclusive) and the joystick
is only aquired when Win32Window is in the foreground.
*/

namespace G3D {

// Deals with unicode/MBCS/char issues
static LPCTSTR toTCHAR(const std::string& str) {
#   if defined(_MBCS) || defined(_UNICODE)
    static const int LEN = 1024;
    static TCHAR x[LEN];

    MultiByteToWideChar(
        CP_ACP, 
        0, 
        str.c_str(), 
        -1, 
        x, 
        LEN);

    //swprintf(x, LEN, _T("%s"), str.c_str());
    return const_cast<LPCTSTR>(x);
#   else
    return str.c_str();
#   endif
}

static const UINT BLIT_BUFFER = 0xC001;

#define WGL_SAMPLE_BUFFERS_ARB	0x2041
#define WGL_SAMPLES_ARB		    0x2042

static bool hasWGLMultiSampleSupport = false;

static unsigned int _sdlKeys[GKey::LAST];
static bool sdlKeysInitialized = false;

// Prototype static helper functions at end of file
static bool ChangeResolution(int, int, int, int);
static void makeKeyEvent(int, int, GEvent&);
static void initWin32KeyMap();

std::auto_ptr<Win32Window> Win32Window::m_shareWindow(NULL);

static uint8 buttonsToUint8(const bool* buttons) {
    uint8 mouseButtons = 0;
    // Clear mouseButtons and set each button bit.
    mouseButtons |= (buttons[0] ? 1 : 0) << 0;
    mouseButtons |= (buttons[1] ? 1 : 0) << 1;
    mouseButtons |= (buttons[2] ? 1 : 0) << 2;
    mouseButtons |= (buttons[3] ? 1 : 0) << 4;
    mouseButtons |= (buttons[4] ? 1 : 0) << 8;
    return mouseButtons;
}

Win32Window::Win32Window(const OSWindow::Settings& s, bool creatingShareWindow)
    :createdWindow(true)
    ,m_diDevices(NULL)
    ,m_sysEventQueue(NULL)
{
    initWGL();

    m_hDC = NULL;
    m_mouseVisible = true;
    m_inputCapture = false;
    m_thread = ::GetCurrentThread();

    if (! sdlKeysInitialized) {
        initWin32KeyMap();
    }

    m_settings = s;

    std::string name = "";

    // Add the non-client area
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_settings.width;
    rect.bottom = m_settings.height;

    DWORD style = 0;

    if (s.framed) {

        // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/WinUI/WindowsUserInterface/Windowing/Windows/WindowReference/WindowStyles.asp
        style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

        if (s.resizable) {
            style |= WS_SIZEBOX;
            if (s.allowMaximize) {
                style |= WS_MAXIMIZEBOX;
            }
        }

    } else {

        // Show nothing but the client area (cannot move window with mouse)
        style |= WS_POPUP;
    }

    int oldTop = rect.top;
    int oldLeft = rect.left;
    AdjustWindowRect(&rect, style, false);

    m_clientRectOffset.x = oldLeft - rect.left;
    m_clientRectOffset.y = oldTop - rect.top;

    int total_width  = rect.right - rect.left;
    int total_height = rect.bottom - rect.top;

    int startX = 0;
    int startY = 0;

    // Don't make the shared window full screen
    bool fullScreen = s.fullScreen && ! creatingShareWindow;

    if (! fullScreen) {
        if (s.center) {

            startX = (GetSystemMetrics(SM_CXSCREEN) - total_width) / 2;
            startY = (GetSystemMetrics(SM_CYSCREEN) - total_height) / 2;

        } else {

            startX = s.x;
            startY = s.y;
        }
    }

    m_clientX = m_settings.x = startX;
    m_clientY = m_settings.y = startY;

    HWND window = CreateWindow(
        Win32Window::g3dWndClass(), 
        toTCHAR(name),
        style,
        startX,
        startY,
        total_width,
        total_height,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (! creatingShareWindow) {
        DragAcceptFiles(window, true);
    }

    alwaysAssertM(window != NULL, "");

    // Set early so windows messages have value
    m_window = window;

    SetWindowLong(window, GWL_USERDATA, (LONG)this);

    init(window, creatingShareWindow);

    // Set default icon if available
    if (! m_settings.defaultIconFilename.empty()) {
        try {

            GImage defaultIcon;
            defaultIcon.load(m_settings.defaultIconFilename);

            setIcon(defaultIcon);
        } catch (const GImage::Error& e) {
            // Throw away default icon
            debugPrintf("OSWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());
            logPrintf("OSWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());            
        }
    }

    if (fullScreen) {
        // Change the desktop resolution if we are running in fullscreen mode
        if (!ChangeResolution(m_settings.width, m_settings.height, (m_settings.rgbBits * 3) + m_settings.alphaBits, m_settings.refreshRate)) {
            alwaysAssertM(false, "Failed to change resolution");
        }
    }

    if (s.visible) {
        ShowWindow(window, SW_SHOW);
    }
}



Win32Window::Win32Window(const OSWindow::Settings& s, HWND hwnd) : createdWindow(false), m_diDevices(NULL) {
    initWGL();

    m_thread = ::GetCurrentThread();
    m_settings = s;
    init(hwnd);
}


Win32Window::Win32Window(const OSWindow::Settings& s, HDC hdc) : createdWindow(false), m_diDevices(NULL)  {
    initWGL();

    m_thread = ::GetCurrentThread();
    m_settings = s;

    HWND hwnd = ::WindowFromDC(hdc);

    debugAssert(hwnd != NULL);

    init(hwnd);
}


Win32Window* Win32Window::create(const OSWindow::Settings& settings) {

    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings);    

}


Win32Window* Win32Window::create(const OSWindow::Settings& settings, HWND hwnd) {

    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings, hwnd);    

}

Win32Window* Win32Window::create(const OSWindow::Settings& settings, HDC hdc) {

    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings, hdc);    

}


void Win32Window::init(HWND hwnd, bool creatingShareWindow) {

    if (! creatingShareWindow) {
        createShareWindow(m_settings);
    }

    m_window = hwnd;

    // Setup the pixel format properties for the output device
    m_hDC = GetDC(m_window);

    bool foundARBFormat = false;
    int pixelFormat = 0;

    if (wglChoosePixelFormatARB != NULL) {
        // Use wglChoosePixelFormatARB to override the pixel format choice for antialiasing.
        // Based on http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=46
        // and http://oss.sgi.com/projects/ogl-sample/registry/ARB/wgl_pixel_format.txt

        Array<float> fAttributes;
        Array<int>   iAttributes;

        // End sentinel; we have no float attribute to set
        fAttributes.append(0.0f, 0.0f);

        iAttributes.append(WGL_DRAW_TO_WINDOW_ARB, GL_TRUE);
        iAttributes.append(WGL_SUPPORT_OPENGL_ARB, GL_TRUE);
        if (m_settings.hardware) {
            iAttributes.append(WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB);
        }
        iAttributes.append(WGL_DOUBLE_BUFFER_ARB,  GL_TRUE);
        iAttributes.append(WGL_COLOR_BITS_ARB,     m_settings.rgbBits * 3);
        iAttributes.append(WGL_RED_BITS_ARB,       m_settings.rgbBits);
        iAttributes.append(WGL_GREEN_BITS_ARB,     m_settings.rgbBits);
        iAttributes.append(WGL_BLUE_BITS_ARB,      m_settings.rgbBits);
        iAttributes.append(WGL_ALPHA_BITS_ARB,     m_settings.alphaBits);
        iAttributes.append(WGL_DEPTH_BITS_ARB,     m_settings.depthBits);
        iAttributes.append(WGL_STENCIL_BITS_ARB,   m_settings.stencilBits);
        iAttributes.append(WGL_STEREO_ARB,         m_settings.stereo);
        if (hasWGLMultiSampleSupport && (m_settings.fsaaSamples > 1)) {
            // On some ATI cards, even setting the samples to false will turn it on,
            // so we only take this branch when FSAA is explicitly requested.
            iAttributes.append(WGL_SAMPLE_BUFFERS_ARB, m_settings.fsaaSamples > 1);
            iAttributes.append(WGL_SAMPLES_ARB,        m_settings.fsaaSamples);
        } else {
            // Report actual settings
            m_settings.fsaaSamples = 0;
        }
        iAttributes.append(0, 0); // end sentinel

        // http://www.nvidia.com/dev_content/nvopenglspecs/WGL_ARB_pixel_format.txt
        uint32 numFormats;
        int valid = wglChoosePixelFormatARB(
            m_hDC,
            iAttributes.getCArray(), 
            fAttributes.getCArray(),
            1,
            &pixelFormat,
            &numFormats);

        // "If the function succeeds, the return value is TRUE. If the function
        // fails the return value is FALSE. To get extended error information,
        // call GetLastError. If no matching formats are found then nNumFormats
        // is set to zero and the function returns TRUE."  -- I think this means
        // that when numFormats == 0 some reasonable format is still selected.

        // Corey - I don't think it does, but now I check for valid pixelFormat + valid return only.

        if ( valid && (pixelFormat > 0)) {
            // Found a valid format
            foundARBFormat = true;
        }

    }

    PIXELFORMATDESCRIPTOR pixelFormatDesc;

    if ( !foundARBFormat ) {

        ZeroMemory(&pixelFormatDesc, sizeof(PIXELFORMATDESCRIPTOR));

        pixelFormatDesc.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
        pixelFormatDesc.nVersion     = 1; 
        pixelFormatDesc.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pixelFormatDesc.iPixelType   = PFD_TYPE_RGBA; 
        pixelFormatDesc.cColorBits   = m_settings.rgbBits * 3;
        pixelFormatDesc.cDepthBits   = m_settings.depthBits;
        pixelFormatDesc.cStencilBits = m_settings.stencilBits;
        pixelFormatDesc.iLayerType   = PFD_MAIN_PLANE; 
        pixelFormatDesc.cRedBits     = m_settings.rgbBits;
        pixelFormatDesc.cGreenBits   = m_settings.rgbBits;
        pixelFormatDesc.cBlueBits    = m_settings.rgbBits;
        pixelFormatDesc.cAlphaBits   = m_settings.alphaBits;

        // Reset for completeness
        pixelFormat = 0;

        // Get the initial pixel format.  We'll override this below in a moment.
        pixelFormat = ChoosePixelFormat(m_hDC, &pixelFormatDesc);
    } else {
        DescribePixelFormat(m_hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDesc);
    }

    alwaysAssertM(pixelFormat != 0, "[0] Unsupported video mode");

    if (SetPixelFormat(m_hDC, pixelFormat, &pixelFormatDesc) == FALSE) {
        alwaysAssertM(false, "[1] Unsupported video mode");
    }

    // Create the OpenGL context
    m_glContext = wglCreateContext(m_hDC);

    alwaysAssertM(m_glContext != NULL, "Failed to create OpenGL context.");

    // Initialize mouse buttons to up
    for (int i = 0; i < 8; ++i) {
        m_mouseButtons[i] = false;
    }

    // Clear all keyboard buttons to up (not down)
    for (int i = 0; i < 256; ++i) {
        m_keyboardButtons[i] = false;
    }

    if (! creatingShareWindow) {
        // Now share resources with the global window
        wglShareLists(m_shareWindow->m_glContext, m_glContext);
    }

    this->makeCurrent();

    if (! creatingShareWindow) {
        GLCaps::init();
        setCaption(m_settings.caption);
    }
}


int Win32Window::width() const {
    return m_settings.width;
}


int Win32Window::height() const {
    return m_settings.height;
}


void Win32Window::setDimensions(const Rect2D& dims) {

    int W = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int H = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

    int x = iClamp((int)dims.x0(), 0, W);
    int y = iClamp((int)dims.y0(), 0, H);
    int w = iClamp((int)dims.width(), 1, W);
    int h = iClamp((int)dims.height(), 1, H);

    // Set dimensions and repaint.
    ::MoveWindow(m_window, x, y, w, h, true);
}


Rect2D Win32Window::dimensions() const {
    return Rect2D::xywh((float)m_clientX, (float)m_clientY, (float)width(), (float)height());
}


bool Win32Window::hasFocus() const {
    // Double check state with foreground and visibility just to be sure.
    return ( (m_window == ::GetForegroundWindow()) && (::IsWindowVisible(m_window)) );
}


std::string Win32Window::getAPIVersion() const {
    return "1.1";
}


std::string Win32Window::getAPIName() const {
    return "Win32";
}


bool Win32Window::requiresMainLoop() const {
    return false;
}


void Win32Window::setIcon(const GImage& image) {
    alwaysAssertM((image.channels == 3) ||
        (image.channels == 4), 
        "Icon image must have at least 3 channels.");

    alwaysAssertM((image.width == 32) && (image.height == 32),
        "Icons must be 32x32 on windows.");

    uint8 bwMaskData[128];
    uint8 colorMaskData[1024*4];


    GImage icon;
    if (image.channels == 3) {
        GImage alpha(image.width, image.height, 1);
        System::memset(alpha.byte(), 255, (image.width * image.height));
        image.insertRedAsAlpha(alpha, icon);
    } else {
        icon = image;
    }

    int colorMaskIdx = 0;
    System::memset(bwMaskData, 0x00, 128);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bwMaskData[ (y * 4) + (x / 8) ] |= ((icon.pixel4(x, y).a > 127) ? 1 : 0) << (x % 8);

            // Windows icon images are BGRA like a lot of windows image data
            colorMaskData[colorMaskIdx] = icon.pixel4()[y * 32 + x].b;
            colorMaskData[colorMaskIdx + 1] = icon.pixel4()[y * 32 + x].g;
            colorMaskData[colorMaskIdx + 2] = icon.pixel4()[y * 32 + x].r;
            colorMaskData[colorMaskIdx + 3] = icon.pixel4()[y * 32 + x].a;
            colorMaskIdx += 4;
        }
    }

    HBITMAP bwMask = ::CreateBitmap(32, 32, 1, 1, bwMaskData);  
    HBITMAP colorMask = ::CreateBitmap(32, 32, 1, 32, colorMaskData);

    ICONINFO iconInfo;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmColor = colorMask;
    iconInfo.hbmMask = bwMask;
    iconInfo.fIcon = true;

    HICON hicon = ::CreateIconIndirect(&iconInfo);
    m_usedIcons.insert((int)hicon);

    // Purposely leak any icon created indirectly like hicon becase we don't know.
    HICON hsmall = (HICON)::SendMessage(this->m_window, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hicon);
    HICON hlarge = (HICON)::SendMessage(this->m_window, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

    if (m_usedIcons.contains((int)hsmall)) {
        ::DestroyIcon(hsmall);
        m_usedIcons.remove((int)hsmall);
    }

    if (m_usedIcons.contains((int)hlarge)) {
        ::DestroyIcon(hlarge);
        m_usedIcons.remove((int)hlarge);
    }

    ::DeleteObject(bwMask);
    ::DeleteObject(colorMask);
}


void Win32Window::swapGLBuffers() {
    debugAssertGLOk();
    // RealTime t2 = System::time(); // TODO: Remove
    ::SwapBuffers(hdc());
    // screenPrintf("swapBuffers: %f s", System::time() - t2); // TODO: Remove

#   ifdef G3D_DEBUG
        // Executing glGetError() after SwapBuffers() causes the CPU to block as if
        // a glFinish had been called, so we only do this in debug mode.
        GLenum e = glGetError();
        if (e == GL_INVALID_ENUM) {
            logPrintf("WARNING: SwapBuffers failed inside G3D::Win32Window; probably because "
                "the context changed when switching monitors.\n\n");
        }

        debugAssertGLOk();
#   endif
}


void Win32Window::close() {
    PostMessage(m_window, WM_CLOSE, 0, 0);
}


Win32Window::~Win32Window() {
    if (OSWindow::current() == this) {
        if (wglMakeCurrent(NULL, NULL) == FALSE)	{
            debugAssertM(false, "Failed to set context");
        }

        if (createdWindow) {
            // Release the mouse
            setMouseVisible(true);
            setInputCapture(false);
        }
    }

    if (createdWindow) {
        SetWindowLong(m_window, GWL_USERDATA, (LONG)NULL);
        close();
    }

    // Do not need to release private HDC's

    delete m_diDevices;
}


void Win32Window::getSettings(OSWindow::Settings& s) const {
    s = m_settings;
}


void Win32Window::setCaption(const std::string& caption) {
    if (m_title != caption) {
        m_title = caption;
        SetWindowText(m_window, toTCHAR(m_title));
    }
}


std::string Win32Window::caption() {
    return m_title;
}
     
void Win32Window::getOSEvents(Queue<GEvent>& events) {

    // init global event queue pointer for window proc
    m_sysEventQueue = &events;

    MSG message;

    while (PeekMessage(&message, m_window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    RECT rect;
    GetWindowRect(m_window, &rect);
    m_settings.x = rect.left;
    m_settings.y = rect.top;

    GetClientRect(m_window, &rect);
    m_settings.width = rect.right - rect.left;
    m_settings.height = rect.bottom - rect.top;

    m_clientX = m_settings.x;
    m_clientY = m_settings.y;

    if (m_settings.framed) {
        // Add the border offset
        m_clientX	+= GetSystemMetrics(m_settings.resizable ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
        m_clientY += GetSystemMetrics(m_settings.resizable ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
    }

    // reset global event queue pointer to be safe
    m_sysEventQueue = NULL;
}


void Win32Window::getDroppedFilenames(Array<std::string>& files) {
    files.fastClear();
    if (m_droppedFiles.size() > 0) {
        files.append(m_droppedFiles);
    }
}


void Win32Window::setMouseVisible(bool b) {
    m_mouseHideCount = b ? 0 : 1;

    if (m_mouseVisible == b) {
        return;
    }

    if (b) {
        while (ShowCursor(true) < 0);
    } else {
        while (ShowCursor(false) >= 0); 
    }

    m_mouseVisible = b;
}


bool Win32Window::mouseVisible() const {
    return m_mouseVisible;
}


bool Win32Window::inputCapture() const {
    return m_inputCapture;
}


void Win32Window::setGammaRamp(const Array<uint16>& gammaRamp) {
    alwaysAssertM(gammaRamp.size() >= 256, "Gamma ramp must have at least 256 entries");

    Log* debugLog = Log::common();

    uint16* ptr = const_cast<uint16*>(gammaRamp.getCArray());
    uint16 wptr[3 * 256];
    for (int i = 0; i < 256; ++i) {
        wptr[i] = wptr[i + 256] = wptr[i + 512] = ptr[i]; 
    }
    BOOL success = SetDeviceGammaRamp(hdc(), wptr);

    if (! success) {
        if (debugLog) {debugLog->println("Error setting gamma ramp! (Possibly LCD monitor)");}
    }
}


void Win32Window::setRelativeMousePosition(double x, double y) {
    SetCursorPos(iRound(x + m_clientX), iRound(y + m_clientY));
}


void Win32Window::setRelativeMousePosition(const Vector2& p) {
    setRelativeMousePosition(p.x, p.y);
}


void Win32Window::getRelativeMouseState(Vector2& p, uint8& mouseButtons) const {
    int x, y;
    getRelativeMouseState(x, y, mouseButtons);
    p.x = (float)x;
    p.y = (float)y;
}

void Win32Window::getRelativeMouseState(int& x, int& y, uint8& mouseButtons) const {
    POINT point;
    GetCursorPos(&point);
    x = point.x - m_clientX;
    y = point.y - m_clientY;

    mouseButtons = buttonsToUint8(m_mouseButtons);
}


void Win32Window::getRelativeMouseState(double& x, double& y, uint8& mouseButtons) const {
    int ix, iy;
    getRelativeMouseState(ix, iy, mouseButtons);
    x = ix;
    y = iy;
}

void Win32Window::enableDirectInput() const {
    if (m_diDevices == NULL) {
        m_diDevices = new _DirectInput(m_window);
    }
}

int Win32Window::numJoysticks() const {
    enableDirectInput();
    return m_diDevices->getNumJoysticks();
}


std::string Win32Window::joystickName(unsigned int sticknum)
{
    enableDirectInput();
    return m_diDevices->getJoystickName(sticknum);
}


void Win32Window::getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button) {

    enableDirectInput();
    if (!m_diDevices->joystickExists(stickNum)) {
        return;
    }

    m_diDevices->getJoystickState(stickNum, axis, button);
}


void Win32Window::setInputCapture(bool c) {
    m_inputCaptureCount = c ? 1 : 0;

    if (c != m_inputCapture) {
        m_inputCapture = c;

        if (m_inputCapture) {
            RECT wrect;
            GetWindowRect(m_window, &wrect);
            m_clientX = wrect.left;
            m_clientY = wrect.top;

            RECT rect = 
            {m_clientX + m_clientRectOffset.x, 
            m_clientY + m_clientRectOffset.y, 
            m_clientX + m_settings.width + m_clientRectOffset.x, 
            m_clientY + m_settings.height + m_clientRectOffset.y};
            ClipCursor(&rect);
        } else {
            ClipCursor(NULL);
        }
    }
}


void Win32Window::initWGL() {

    // This function need only be called once
    static bool wglInitialized = false;
    if (wglInitialized) {
        return;
    }
    wglInitialized = true;

    std::string name = "G3D";
    WNDCLASS window_class;

    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = Win32Window::windowProc;
    window_class.cbClsExtra    = 0; 
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = GetModuleHandle(NULL);
    window_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION); 
    window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszMenuName  = toTCHAR(name);
    window_class.lpszClassName = _T("window"); 

    int ret = RegisterClass(&window_class);
    alwaysAssertM(ret, "Registration Failed");

    // Create some dummy pixel format.
    PIXELFORMATDESCRIPTOR pfd =	
    {
        sizeof (PIXELFORMATDESCRIPTOR),									// Size Of This Pixel Format Descriptor
        1,																// Version Number
        PFD_DRAW_TO_WINDOW |											// Format Must Support Window
        PFD_SUPPORT_OPENGL |											// Format Must Support OpenGL
        PFD_DOUBLEBUFFER,												// Must Support Double Buffering
        PFD_TYPE_RGBA,													// Request An RGBA Format
        24,		                        								// Select Our Color Depth
        0, 0, 0, 0, 0, 0,												// Color Bits Ignored
        1,																// Alpha Buffer
        0,																// Shift Bit Ignored
        0,																// No Accumulation Buffer
        0, 0, 0, 0,														// Accumulation Bits Ignored
        16,																// 16Bit Z-Buffer (Depth Buffer)  
        0,																// No Stencil Buffer
        0,																// No Auxiliary Buffer
        PFD_MAIN_PLANE,													// Main Drawing Layer
        0,																// Reserved
        0, 0, 0															// Layer Masks Ignored
    };

    HWND hWnd = CreateWindow(_T("window"), _T(""), 0, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    debugAssert(hWnd);

    HDC  hDC  = GetDC(hWnd);
    debugAssert(hDC);

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    debugAssert(pixelFormat);

    if (SetPixelFormat(hDC, pixelFormat, &pfd) == FALSE) {
        debugAssertM(false, "Failed to set pixel format");
    }

    HGLRC hRC = wglCreateContext(hDC);
    debugAssert(hRC);

    // wglMakeCurrent is the bottleneck of this routine; it takes about 0.1 s
    if (wglMakeCurrent(hDC, hRC) == FALSE)	{
        debugAssertM(false, "Failed to set context");
    }

    // We've now brought OpenGL online.  Grab the pointers we need and 
    // destroy everything.

    wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

    if (wglGetExtensionsStringARB != NULL) {

        std::string wglExtensions = wglGetExtensionsStringARB(hDC);

        std::istringstream extensionsStream;
        extensionsStream.str(wglExtensions.c_str());

        std::string extension;
        while ( extensionsStream >> extension ) {
            if (extension == "WGL_ARB_multisample") {
                hasWGLMultiSampleSupport = true;
                break;
            }
        }

    } else {
        hasWGLMultiSampleSupport = false;
    }

    // Now destroy the dummy windows
    wglDeleteContext(hRC);					
    hRC = 0;	
    ReleaseDC(hWnd, hDC);	
    hWnd = 0;				
    DestroyWindow(hWnd);			
    hWnd = 0;
}


void Win32Window::createShareWindow(OSWindow::Settings settings) {
    static bool init = false;
    if (init) {
        return;
    }

    init = true;	

    // We want a small (low memory), invisible window
    settings.visible = false;
    settings.width = 16;
    settings.height = 16;
    settings.framed = false;
    settings.fullScreen = false;


    // This call will force us to re-enter createShareWindow, however
    // the second time through init will be true, so we'll skip the 
    // recursion.
    m_shareWindow.reset(new Win32Window(settings, true));
}


void Win32Window::reallyMakeCurrent() const {
    debugAssertM(m_thread == ::GetCurrentThread(), 
        "Cannot call OSWindow::makeCurrent on different threads.");

    if (wglMakeCurrent(m_hDC, m_glContext) == FALSE)	{
        debugAssertM(false, "Failed to set context");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Static helper functions for Win32Window
*/

/** Changes the screen resolution */
static bool ChangeResolution(int width, int height, int bpp, int refreshRate) {

    if (refreshRate == 0) {
        refreshRate = 85;
    }

    DEVMODE deviceMode;

    ZeroMemory(&deviceMode, sizeof(DEVMODE));

    int bppTries[3];
    bppTries[0] = bpp;
    bppTries[1] = 32;
    bppTries[2] = 16;

    deviceMode.dmSize       = sizeof(DEVMODE);
    deviceMode.dmPelsWidth  = width;
    deviceMode.dmPelsHeight = height;
    deviceMode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    deviceMode.dmDisplayFrequency = refreshRate;

    LONG result = -1;

    for (int i = 0; (i < 3) && (result != DISP_CHANGE_SUCCESSFUL); ++i) {
        deviceMode.dmBitsPerPel = bppTries[i];
        result = ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN);
    }

    if (result != DISP_CHANGE_SUCCESSFUL) {
        // If it didn't work, try just changing the resolution and not the
        // refresh rate.
        deviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        for (int i = 0; (i < 3) && (result != DISP_CHANGE_SUCCESSFUL); ++i) {
            deviceMode.dmBitsPerPel = bppTries[i];
            result = ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN);
        }
    }

    return result == DISP_CHANGE_SUCCESSFUL;
}


static void makeKeyEvent(int vkCode, int lParam, GEvent& e) {

    // If true, we're looking at the right hand version of
    // Fix VK_SHIFT, VK_CONTROL, VK_MENU
    bool extended = (lParam >> 24) & 0x01;

    // Check for normal letter event
    if ((vkCode >= 'A') && (vkCode <= 'Z')) {

        // Make key codes lower case canonically
        e.key.keysym.sym = (GKey::Value)(vkCode - 'A' + 'a');

    } else if (vkCode == VK_SHIFT) {

        e.key.keysym.sym = extended ? GKey::RSHIFT : GKey::LSHIFT;

    } else if (vkCode == VK_CONTROL) {

        e.key.keysym.sym = extended ? GKey::RCTRL : GKey::LCTRL;

    } else if (vkCode == VK_MENU) {

        e.key.keysym.sym = extended ? GKey::RALT : GKey::LALT;

    } else {

        e.key.keysym.sym = (GKey::Value)_sdlKeys[iClamp(vkCode, 0, GKey::LAST - 1)];

    }

    e.key.keysym.scancode = MapVirtualKey(vkCode, 0); 
    //(lParam >> 16) & 0x7F;

    static BYTE lpKeyState[256];
    GetKeyboardState(lpKeyState);

    int mod = 0;
    if (lpKeyState[VK_LSHIFT] & 0x80) {
        mod = mod | GKeyMod::LSHIFT;
    }

    if (lpKeyState[VK_RSHIFT] & 0x80) {
        mod = mod | GKeyMod::RSHIFT;
    }

    if (lpKeyState[VK_LCONTROL] & 0x80) {
        mod = mod | GKeyMod::LCTRL;
    }

    if (lpKeyState[VK_RCONTROL] & 0x80) {
        mod = mod | GKeyMod::RCTRL;
    }

    if (lpKeyState[VK_LMENU] & 0x80) {
        mod = mod | GKeyMod::LALT;
    }

    if (lpKeyState[VK_RMENU] & 0x80) {
        mod = mod | GKeyMod::RALT;
    }
    e.key.keysym.mod = (GKeyMod::Value)mod;

    ToUnicode(vkCode, e.key.keysym.scancode, lpKeyState, (LPWSTR)&e.key.keysym.unicode, 1, 0);
}


void Win32Window::mouseButton(UINT mouseMessage, DWORD lParam, DWORD wParam) {
    GEvent e;

    // all events map to mouse 0 currently
    e.button.which = 0;

    // xy position of click
    e.button.x = GET_X_LPARAM(lParam);
    e.button.y = GET_Y_LPARAM(lParam);

    switch (mouseMessage) {
        case WM_LBUTTONDBLCLK:
            // double clicks are a regular click event with 2 clicks
            // only double click events have 2 clicks, all others are 1 click even in the same location
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 0;
            break;

        case WM_MBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 1;
            break;

        case WM_RBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 2;
            break;

        case WM_XBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            break;

        case WM_LBUTTONDOWN:
            // map a button down message to a down event, pressed state and button index of
            // 0 - left, 1 - middle, 2 - right, 3+ - x
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 0;
            break;

        case WM_MBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 1;
            break;

        case WM_RBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 2;
            break;

        case WM_XBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            break;

        case WM_LBUTTONUP:
            // map a button up message to a up event, released state and button index of
            // 0 - left, 1 - middle, 2 - right, 3+ - x
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 0;
            break;

        case WM_MBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 1;
            break;

        case WM_RBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 2;
            break;

        case WM_XBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            break;

        default:
            // handling non-mouse event?
            debugAssert(false);
    }

    // add initial event
    m_sysEventQueue->pushBack(e);

    // check for any clicks as defined by a button down then up sequence
    if (e.type == GEventType::MOUSE_BUTTON_UP && m_mouseButtons[e.button.button]) {

        // add click event from same location
        e.type = GEventType::MOUSE_BUTTON_CLICK;
        e.button.numClicks = 1;

        m_sysEventQueue->pushBack(e);
    }

    m_mouseButtons[e.button.button] = (e.type == GEventType::MOUSE_BUTTON_DOWN);
}


/**
Initializes SDL to Win32 key map
*/
static void initWin32KeyMap() {
    memset(_sdlKeys, 0, sizeof(_sdlKeys));

    _sdlKeys[VK_BACK] = GKey::BACKSPACE;
    _sdlKeys[VK_TAB] = GKey::TAB;
    _sdlKeys[VK_CLEAR] = GKey::CLEAR;
    _sdlKeys[VK_RETURN] = GKey::RETURN;
    _sdlKeys[VK_PAUSE] = GKey::PAUSE;
    _sdlKeys[VK_ESCAPE] = GKey::ESCAPE;
    _sdlKeys[VK_SPACE] = GKey::SPACE;
    _sdlKeys[VK_APOSTROPHE] = GKey::QUOTE;
    _sdlKeys[VK_COMMA] = GKey::COMMA;
    _sdlKeys[VK_MINUS] = GKey::MINUS;
    _sdlKeys[VK_PERIOD] = GKey::PERIOD;
    _sdlKeys[VK_SLASH] = GKey::SLASH;
    _sdlKeys['0'] = '0';
    _sdlKeys['1'] = '1';
    _sdlKeys['2'] = '2';
    _sdlKeys['3'] = '3';
    _sdlKeys['4'] = '4';
    _sdlKeys['5'] = '5';
    _sdlKeys['6'] = '6';
    _sdlKeys['7'] = '7';
    _sdlKeys['8'] = '8';
    _sdlKeys['9'] = '9';
    _sdlKeys[VK_SEMICOLON] = GKey::SEMICOLON;
    _sdlKeys[VK_EQUALS] = GKey::EQUALS;
    _sdlKeys[VK_LBRACKET] = GKey::LEFTBRACKET;
    _sdlKeys[VK_BACKSLASH] = GKey::BACKSLASH;
    _sdlKeys[VK_RBRACKET] = GKey::RIGHTBRACKET;
    _sdlKeys[VK_GRAVE] = GKey::BACKQUOTE;
    _sdlKeys[VK_BACKTICK] = GKey::BACKQUOTE;
    _sdlKeys[VK_DELETE] = GKey::DELETE;

    _sdlKeys[VK_NUMPAD0] = GKey::KP0;
    _sdlKeys[VK_NUMPAD1] = GKey::KP1;
    _sdlKeys[VK_NUMPAD2] = GKey::KP2;
    _sdlKeys[VK_NUMPAD3] = GKey::KP3;
    _sdlKeys[VK_NUMPAD4] = GKey::KP4;
    _sdlKeys[VK_NUMPAD5] = GKey::KP5;
    _sdlKeys[VK_NUMPAD6] = GKey::KP6;
    _sdlKeys[VK_NUMPAD7] = GKey::KP7;
    _sdlKeys[VK_NUMPAD8] = GKey::KP8;
    _sdlKeys[VK_NUMPAD9] = GKey::KP9;
    _sdlKeys[VK_DECIMAL] = GKey::KP_PERIOD;
    _sdlKeys[VK_DIVIDE] = GKey::KP_DIVIDE;
    _sdlKeys[VK_MULTIPLY] = GKey::KP_MULTIPLY;
    _sdlKeys[VK_SUBTRACT] = GKey::KP_MINUS;
    _sdlKeys[VK_ADD] = GKey::KP_PLUS;

    _sdlKeys[VK_UP] = GKey::UP;
    _sdlKeys[VK_DOWN] = GKey::DOWN;
    _sdlKeys[VK_RIGHT] = GKey::RIGHT;
    _sdlKeys[VK_LEFT] = GKey::LEFT;
    _sdlKeys[VK_INSERT] = GKey::INSERT;
    _sdlKeys[VK_HOME] = GKey::HOME;
    _sdlKeys[VK_END] = GKey::END;
    _sdlKeys[VK_PRIOR] = GKey::PAGEUP;
    _sdlKeys[VK_NEXT] = GKey::PAGEDOWN;

    _sdlKeys[VK_F1] = GKey::F1;
    _sdlKeys[VK_F2] = GKey::F2;
    _sdlKeys[VK_F3] = GKey::F3;
    _sdlKeys[VK_F4] = GKey::F4;
    _sdlKeys[VK_F5] = GKey::F5;
    _sdlKeys[VK_F6] = GKey::F6;
    _sdlKeys[VK_F7] = GKey::F7;
    _sdlKeys[VK_F8] = GKey::F8;
    _sdlKeys[VK_F9] = GKey::F9;
    _sdlKeys[VK_F10] = GKey::F10;
    _sdlKeys[VK_F11] = GKey::F11;
    _sdlKeys[VK_F12] = GKey::F12;
    _sdlKeys[VK_F13] = GKey::F13;
    _sdlKeys[VK_F14] = GKey::F14;
    _sdlKeys[VK_F15] = GKey::F15;

    _sdlKeys[VK_NUMLOCK] = GKey::NUMLOCK;
    _sdlKeys[VK_CAPITAL] = GKey::CAPSLOCK;
    _sdlKeys[VK_SCROLL] = GKey::SCROLLOCK;
    _sdlKeys[VK_RSHIFT] = GKey::RSHIFT;
    _sdlKeys[VK_LSHIFT] = GKey::LSHIFT;
    _sdlKeys[VK_RCONTROL] = GKey::RCTRL;
    _sdlKeys[VK_LCONTROL] = GKey::LCTRL;
    _sdlKeys[VK_RMENU] = GKey::RALT;
    _sdlKeys[VK_LMENU] = GKey::LALT;
    _sdlKeys[VK_RWIN] = GKey::RSUPER;
    _sdlKeys[VK_LWIN] = GKey::LSUPER;

    _sdlKeys[VK_HELP] = GKey::HELP;
    _sdlKeys[VK_PRINT] = GKey::PRINT;
    _sdlKeys[VK_SNAPSHOT] = GKey::PRINT;
    _sdlKeys[VK_CANCEL] = GKey::BREAK;
    _sdlKeys[VK_APPS] = GKey::MENU;

    sdlKeysInitialized = true;
}


#if 0
static void printPixelFormatDescription(int format, HDC hdc, TextOutput& out) {

    PIXELFORMATDESCRIPTOR pixelFormat;
    DescribePixelFormat(hdc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormat);

    /*
    typedef struct tagPIXELFORMATDESCRIPTOR { // pfd   
    WORD  nSize; 
    WORD  nVersion; 
    DWORD dwFlags; 
    BYTE  iPixelType; 
    BYTE  cColorBits; 
    BYTE  cRedBits; 
    BYTE  cRedShift; 
    BYTE  cGreenBits; 
    BYTE  cGreenShift; 
    BYTE  cBlueBits; 
    BYTE  cBlueShift; 
    BYTE  cAlphaBits; 
    BYTE  cAlphaShift; 
    BYTE  cAccumBits; 
    BYTE  cAccumRedBits; 
    BYTE  cAccumGreenBits; 
    BYTE  cAccumBlueBits; 
    BYTE  cAccumAlphaBits; 
    BYTE  cDepthBits; 
    BYTE  cStencilBits; 
    BYTE  cAuxBuffers; 
    BYTE  iLayerType; 
    BYTE  bReserved; 
    DWORD dwLayerMask; 
    DWORD dwVisibleMask; 
    DWORD dwDamageMask; 
    } PIXELFORMATDESCRIPTOR; 
    */

    out.printf("#%d Format Description\n", format);
    out.printf("nSize:\t\t\t\t%d\n", pixelFormat.nSize);
    out.printf("nVersion:\t\t\t%d\n", pixelFormat.nVersion);
    std::string s =
        (std::string((pixelFormat.dwFlags&PFD_DRAW_TO_WINDOW) ? "PFD_DRAW_TO_WINDOW|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_DRAW_TO_BITMAP) ? "PFD_DRAW_TO_BITMAP|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_SUPPORT_GDI) ? "PFD_SUPPORT_GDI|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_SUPPORT_OPENGL) ? "PFD_SUPPORT_OPENGL|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_GENERIC_ACCELERATED) ? "PFD_GENERIC_ACCELERATED|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_GENERIC_FORMAT) ? "PFD_GENERIC_FORMAT|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_NEED_PALETTE) ? "PFD_NEED_PALETTE|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_NEED_SYSTEM_PALETTE) ? "PFD_NEED_SYSTEM_PALETTE|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_DOUBLEBUFFER) ? "PFD_DOUBLEBUFFER|" : "") + 
         std::string((pixelFormat.dwFlags&PFD_STEREO) ? "PFD_STEREO|" : "") +
         std::string((pixelFormat.dwFlags&PFD_SWAP_LAYER_BUFFERS) ? "PFD_SWAP_LAYER_BUFFERS" : ""));

    out.printf("dwFlags:\t\t\t%s\n", s.c_str());
    out.printf("iPixelType:\t\t\t%d\n", pixelFormat.iPixelType);
    out.printf("cColorBits:\t\t\t%d\n", pixelFormat.cColorBits);
    out.printf("cRedBits:\t\t\t%d\n", pixelFormat.cRedBits);
    out.printf("cRedShift:\t\t\t%d\n", pixelFormat.cRedShift);
    out.printf("cGreenBits:\t\t\t%d\n", pixelFormat.cGreenBits);
    out.printf("cGreenShift:\t\t\t%d\n", pixelFormat.cGreenShift);
    out.printf("cBlueBits:\t\t\t%d\n", pixelFormat.cBlueBits);
    out.printf("cBlueShift:\t\t\t%d\n", pixelFormat.cBlueShift);
    out.printf("cAlphaBits:\t\t\t%d\n", pixelFormat.cAlphaBits);
    out.printf("cAlphaShift:\t\t\t%d\n", pixelFormat.cAlphaShift);
    out.printf("cAccumBits:\t\t\t%d\n", pixelFormat.cAccumBits);
    out.printf("cAccumRedBits:\t\t\t%d\n", pixelFormat.cAccumRedBits);
    out.printf("cAccumGreenBits:\t\t%d\n", pixelFormat.cAccumGreenBits);
    out.printf("cAccumBlueBits:\t\t\t%d\n", pixelFormat.cAccumBlueBits);
    out.printf("cAccumAlphaBits:\t\t%d\n", pixelFormat.cAccumAlphaBits);
    out.printf("cDepthBits:\t\t\t%d\n", pixelFormat.cDepthBits);
    out.printf("cStencilBits:\t\t\t%d\n", pixelFormat.cStencilBits);
    out.printf("cAuxBuffers:\t\t\t%d\n", pixelFormat.cAuxBuffers);
    out.printf("iLayerType:\t\t\t%d\n", pixelFormat.iLayerType);
    out.printf("bReserved:\t\t\t%d\n", pixelFormat.bReserved);
    out.printf("dwLayerMask:\t\t\t%d\n", pixelFormat.dwLayerMask);
    out.printf("dwDamageMask:\t\t\t%d\n", pixelFormat.dwDamageMask);

    out.printf("\n");
}
#endif

LRESULT CALLBACK Win32Window::windowProc(HWND     window,
                                         UINT     message,
                                         WPARAM   wParam,
                                         LPARAM   lParam) {

    Win32Window* this_window = (Win32Window*)GetWindowLong(window, GWL_USERDATA);

    if (this_window != NULL && this_window->m_sysEventQueue != NULL) {
        GEvent e;

        switch (message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:

            // only tracking keys less than 256
            if (wParam < 256) {
                // if repeating (bit 30 set), only add key down event if
                // we haven't seen it already
                if (this_window->m_keyboardButtons[wParam] || ((lParam & 0x40000000) == 0)) {
                    e.key.type = GEventType::KEY_DOWN;
                    e.key.state = GButtonState::PRESSED;

                    makeKeyEvent(wParam, lParam, e);
                    this_window->m_keyboardButtons[wParam] = true;
                    
                    this_window->m_sysEventQueue->pushBack(e);
                }
            } else {
                // we are processing a key press over 256 which we aren't tracking
                // check to see if the VK_ mappings are valid and now high enough
                // that we need to track more (try using a map)
                debugAssert(wParam < 256);
            }
            return 0;


        case WM_KEYUP:
        case WM_SYSKEYUP:
            // only tracking keys less than 256
            if (wParam < 256) {
                e.key.type = GEventType::KEY_UP;
                e.key.state = GButtonState::RELEASED;

                makeKeyEvent(wParam, lParam, e);
                this_window->m_keyboardButtons[wParam] = false;

                this_window->m_sysEventQueue->pushBack(e);
            } else {
                // we are processing a key press over 256 which we aren't tracking
                // check to see if the VK_ mappings are valid and now high enough
                // that we need to track more (try using a map)
                debugAssert(wParam < 256);
            }
            return 0;

        case WM_MOUSEMOVE:
            e.motion.type = GEventType::MOUSE_MOTION;
            e.motion.which = 0; // TODO: mouse index
            e.motion.state = buttonsToUint8(this_window->m_mouseButtons);
            e.motion.x = GET_X_LPARAM(lParam);
            e.motion.y = GET_Y_LPARAM(lParam);
            e.motion.xrel = 0;
            e.motion.yrel = 0;
            this_window->m_sysEventQueue->pushBack(e);
            return 0;

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
            this_window->mouseButton(message, lParam, wParam);
            return 0;

        case WM_DROPFILES:
            {
                e.drop.type = GEventType::FILE_DROP;
                HDROP hDrop = (HDROP)wParam;
                POINT point;
                DragQueryPoint(hDrop, &point);
                e.drop.x = point.x;
                e.drop.y = point.y;

                enum {NUM_FILES=0xFFFFFFFF};

                int n = DragQueryFile(hDrop, NUM_FILES,NULL, 0);
                this_window->m_droppedFiles.clear();
                for (int i = 0; i < n; ++i) {
                    int numChars = DragQueryFile(hDrop, i, NULL, 0);
                    char* temp = (char*)System::malloc(numChars + 2);
                    DragQueryFileA(hDrop, i, temp, numChars + 1);
                    std::string s = temp;
                    this_window->m_droppedFiles.append(s);
                    System::free(temp);
                }
                DragFinish(hDrop);

                this_window->m_sysEventQueue->pushBack(e);
            }
            return 0;

        case WM_CLOSE:
            // handle close event
            e.type = GEventType::QUIT;
            this_window->m_sysEventQueue->pushBack(e);
            DestroyWindow(window);
            return 0;

        case WM_SIZE:
            // handle resize event (minimized is ignored)
            if ((wParam == SIZE_MAXIMIZED) ||
                (wParam == SIZE_RESTORED))
            {
                // Add a size event that will be returned next OSWindow poll
                e.type = GEventType::VIDEO_RESIZE;
                e.resize.w = LOWORD(lParam);
                e.resize.h = HIWORD(lParam);
                this_window->m_sysEventQueue->pushBack(e);
                this_window->handleResize(e.resize.w, e.resize.h);
            }
            return 0;

        case WM_KILLFOCUS:
            for (int i = 0; i < sizeof(this_window->m_keyboardButtons); ++i) {
                if (this_window->m_keyboardButtons[i]) {
                    ::PostMessage(window, WM_KEYUP, i, 0);
                }
            }
            return 0;

        }
    }

    return DefWindowProc(window, message, wParam, lParam);
}

/** Return the G3D window class, which owns a private DC. 
    See http://www.starstonesoftware.com/OpenGL/whyyou.htm
    for a discussion of why this is necessary. */
LPCTSTR Win32Window::g3dWndClass() {

    static LPCTSTR g3dWindowClassName = NULL;

    if (g3dWindowClassName == NULL) {

        WNDCLASS wndcls;

        wndcls.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
        wndcls.lpfnWndProc = Win32Window::windowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = ::GetModuleHandle(NULL);
        wndcls.hIcon = NULL;
        wndcls.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wndcls.hbrBackground = NULL;
        wndcls.lpszMenuName = NULL;
        wndcls.lpszClassName = _T("G3DWindow");

        if (!RegisterClass(&wndcls)) {
            Log::common()->printf("\n**** WARNING: could not create G3DWindow class ****\n");
            // error!  Return the default window class.
            return _T("window");
        }

        g3dWindowClassName = wndcls.lpszClassName;        
    }

    return g3dWindowClassName;
}

} // G3D namespace

#endif // G3D_WIN32

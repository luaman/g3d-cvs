/**
  @file GLG3D/SDLWindow.h

  @maintainer Morgan McGuire, morgan@graphics3d.com
  @created 2004-02-10
  @edited  2008-07-15

  Copyright 2000-2009, Morgan McGuire
  All rights reserved.
*/

#ifndef G3D_SDLWINDOW_H
#define G3D_SDLWINDOW_H

#include "G3D/platform.h"

#if defined(G3D_LINUX) || defined(G3D_FREEBSD)

#include "G3D/Queue.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/glcalls.h"

#include <SDL/SDL_events.h>
// A G3D-specific copy of SDL_events was previously used to break
// the dependence of G3D headers on SDL, since only the .cpp files
// actually need the full SDL headers.
//#include "GLG3D/G3D_SDL_event.h"

#if defined(G3D_OSX)
#    include "GLG3D/NSAutoreleasePoolWrapper.h"
#    include <Carbon/Carbon.h>
#    include <dlfcn.h>
#endif

namespace G3D {

/**
 An implementation of G3D::OSWindow that uses the Open Source SDL
 library.  Works on Windows, Linux, FreeBSD, and OS X.  Only built
 into the normal GLG3D library on Linux/FreeBSD because other
 platforms have native GWindows.
 */
class SDLWindow : public OSWindow {
private:

    /** Window title */
    std::string                 _caption;

    /** API version */
    std::string                 _version;

    /** The x, y fields are not updated when the window moves. */
    OSWindow::Settings           _settings;

    bool                        _inputCapture;

    Array< ::SDL_Joystick* >    joy;

    bool                        _mouseVisible;

    GLContext                   _glContext;

#   if defined(G3D_LINUX) || defined(G3D_FREEBSD)
        Display*                _X11Display;
        Window                  _X11Window;
        Window                  _X11WMWindow;
#   elif defined(G3D_WIN32)
        HDC                     _Win32HDC;
        HWND                    _Win32HWND;
#   elif defined(G3D_OSX)
        NSAutoreleasePoolWrapper* _pool;
#   endif

    Queue<GEvent>               m_eventQueue;

protected:
    
    virtual void reallyMakeCurrent() const;

    virtual bool pollOSEvent(GEvent& e);

    Array<std::string> m_droppedFiles;

    virtual void setMouseVisible(bool b);

    virtual void setInputCapture(bool c);

public:

    static SDLWindow* create(const OSWindow::Settings& settings = OSWindow::Settings());

    SDLWindow(const OSWindow::Settings& settings);

    virtual ~SDLWindow();

    virtual void getSettings(OSWindow::Settings& settings) const;

    virtual int width() const;

    virtual int height() const ;

    virtual Rect2D dimensions() const;

    virtual void setDimensions(const Rect2D& dims);

    virtual void setPosition(int x, int y);

    virtual bool hasFocus() const;

    virtual std::string getAPIVersion() const;

    virtual std::string getAPIName() const;

    virtual void setGammaRamp(const Array<uint16>& gammaRamp);

    virtual void setCaption(const std::string& caption);

    virtual std::string caption();

    virtual void setIcon(const GImage& image);

    virtual void swapGLBuffers();

    virtual void notifyResize(int w, int h);

    virtual int numJoysticks() const;

    virtual std::string joystickName(unsigned int sticknum);

    virtual void getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button);

    virtual void setRelativeMousePosition(double x, double y);

    virtual void setRelativeMousePosition(const Vector2& p);

    virtual void getDroppedFilenames(Array<std::string>& files);

    virtual void getRelativeMouseState(Vector2& p, uint8& mouseButtons) const;
    virtual void getRelativeMouseState(int& x, int& y, uint8& mouseButtons) const;
    virtual void getRelativeMouseState(double& x, double& y, uint8& mouseButtons) const;
    /** Returns the underlying SDL joystick pointer */
    ::SDL_Joystick* getSDL_Joystick(unsigned int num) const;

    #if defined(G3D_LINUX)
        Window   x11Window() const;
        Display* x11Display() const;
    #elif defined(G3D_WIN32)
        HDC      win32HDC() const;
        HWND     win32HWND() const;
    #endif

//    virtual void makeCurrent() const;
};

} // namespace

#endif // G3D_WIN32
#endif

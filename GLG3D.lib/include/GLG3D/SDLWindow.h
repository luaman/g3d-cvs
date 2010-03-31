/**
  @file GLG3D/SDLWindow.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
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
    std::string                 m_caption;

    /** API version */
    std::string                 m_version;

    bool                        m_inputCapture;

    Array< ::SDL_Joystick* >    m_joy;

    bool                        m_mouseVisible;

    GLContext                   m_glContext;

    Display*                    m_X11Display;
    Window                      m_X11Window;
    Window                      m_X11WMWindow;

    int                         m_videoFlags;

    Queue<GEvent>               m_eventQueue;

protected:

    virtual void reallyMakeCurrent() const;

    virtual void getOSEvents(Queue<GEvent>& events);

    Array<std::string> m_droppedFiles;

    virtual void setMouseVisible(bool b);

    virtual void setInputCapture(bool c);

public:

    /** \copydoc OSWindow::primaryDisplaySize */
    static Vector2 primaryDisplaySize();

    /** \copydoc OSWindow::virtualDisplaySize */
    static Vector2 virtualDisplaySize();

    /** \copydoc OSWindow::primaryDisplayWindowSize */
    static Vector2 primaryDisplayWindowSize();

    /** \copydoc OSWindow::numDisplays */
    static int numDisplays();

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

    Window   x11Window() const;
    Display* x11Display() const;

//    virtual void makeCurrent() const;
};

extern bool sdlAlreadyInitialized;

} // namespace

#endif // G3D_WIN32
#endif

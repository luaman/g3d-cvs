/**
 @file Win32Window.h
  
 A OSWindow that uses the Win32 API.

 @maintainer Morgan McGuire
 @created 	  2004-05-21
 @edited  	  2007-05-30
    
 Copyright 2000-2007, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_WIN32WINDOW_H
#define G3D_WIN32WINDOW_H

// This file is only used on Windows
#ifdef G3D_WIN32

#include <string>
#include "G3D/platform.h"
#include "G3D/Set.h"
#include "G3D/Queue.h"
#include "G3D/Rect2D.h"
#include "GLG3D/OSWindow.h"

namespace G3D {

// Forward declaration so directinput8.h is included in cpp
namespace _internal { class _DirectInput; }
using _internal::_DirectInput;


class Win32Window : public OSWindow {
private:
	
    Vector2              m_clientRectOffset;
	std::string			 m_title;
    HDC                  m_hDC;
	HGLRC				 m_glContext;
	bool				 m_mouseVisible;
	bool				 m_inputCapture;

    /** Mouse Button State Array: false - up, true - down
        [0] - left, [1] - middle, [2] - right, [3] - X1,  [4] - X2 */
    bool                 m_mouseButtons[8];
    bool                 m_keyboardButtons[256];

    mutable _DirectInput* m_diDevices;

    G3D::Set< int >      m_usedIcons;

    /** Coordinates of the client area in screen coordinates */
    int		             m_clientX;
    int			         m_clientY;
    
    /** Only one thread allowed for use with Win32Window::makeCurrent */
    HANDLE				 m_thread;
    
    Array<std::string>   m_droppedFiles;

    HWND                 m_window;
    const bool		     createdWindow;

    /** Called from all constructors */
	void init(HWND hwnd, bool creatingShareWindow = false);

    // Pointer to current queue passed to getOSEvents() for window proc to use
    Queue<GEvent>*      m_sysEventQueue;

	static std::auto_ptr<Win32Window>& shareWindow();

	/** OpenGL technically does not allow sharing of resources between
	  multiple windows (although this tends to work most of the time
	  in practice), so we create an invisible HDC and context with which
	  to explicitly share all resources. 
	  
	  @param s The settings describing the pixel format of the windows with which
	  resources will be shared.  Sharing may fail if all windows do not have the
	  same format.*/ 
	static void createShareWindow(OSWindow::Settings s);

    /** Initializes the WGL extensions by creating and then destroying a window.  
        Also registers our window class.  
    
        It is necessary to create a dummy window to avoid a catch-22 in the Win32
        API: fsaa window creation is supported through a WGL extension, but WGL 
        extensions can't be called until after a window has already been created. */
    static void initWGL();

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    static LPCTSTR g3dWndClass();

    /** 
    Configures a mouse up/down event
    */
    void mouseButton(UINT mouseMessage, DWORD lParam, DWORD wParam);

    /** Constructs from a new window */
    explicit Win32Window(const OSWindow::Settings& settings, bool creatingShareWindow = false);
    
    /** Constructs from an existing window */
    explicit Win32Window(const OSWindow::Settings& settings, HWND hwnd);
    
    /** Constructs from an existing window */
    explicit Win32Window(const OSWindow::Settings& settings, HDC hdc);
    
    // Disallow copy constructor (made private)
    Win32Window& operator=(const Win32Window& other);

public:

    /** \copydoc OSWindow::primaryDisplaySize */
    static Vector2 primaryDisplaySize();

    /** \copydoc OSWindow::virtualDisplaySize */
    static Vector2 virtualDisplaySize();

    /** \copydoc OSWindow::primaryDisplayWindowSize */
    static Vector2 primaryDisplayWindowSize();

    /** \copydoc OSWindow::numDisplays */
    static int numDisplays();

    /** Different subclasses will be returned depending on
        whether DirectInput8 is available. You must delete 
        the window returned when you are done with it. */
    static Win32Window* create(const OSWindow::Settings& settings = OSWindow::Settings());

    static Win32Window* create(const OSWindow::Settings& settings, HWND hwnd);

    /** The HDC should be a private CS_OWNDC device context because it is assumed to
        be perisistant.*/
    static Win32Window* create(const OSWindow::Settings& settings, HDC hdc);
	
    virtual ~Win32Window();
	
    //virtual Array<Settings> enumerateAvailableSettings();

    virtual void getDroppedFilenames(Array<std::string>& files);

    void close();
	
    inline HWND hwnd() const {
        return m_window;
    }

    inline HDC hdc() const {
        return m_hDC;
    }
    
    void getSettings(OSWindow::Settings& settings) const;

    virtual int width() const;
	
    virtual int height() const;
	
    virtual Rect2D dimensions() const;
	
    virtual void setDimensions(const Rect2D& dims);
	
    virtual void setPosition(int x, int y) {
        setDimensions( Rect2D::xywh((float)x, (float)y, (float)m_settings.width, (float)m_settings.height) );
    }
	
    virtual bool hasFocus() const;
	
    virtual std::string getAPIVersion() const;
	
    virtual std::string getAPIName() const;
	
    virtual void setGammaRamp(const Array<uint16>& gammaRamp);
    
    virtual void setCaption(const std::string& caption);
	
    virtual int numJoysticks() const;
	
    virtual std::string joystickName(unsigned int sticknum);
	
    virtual std::string caption();
	
    virtual void setIcon(const GImage& image);
	
    virtual void swapGLBuffers();
	
    virtual void setRelativeMousePosition(double x, double y);
	
    virtual void setRelativeMousePosition(const Vector2& p);
		
    virtual void getRelativeMouseState(Vector2& position, uint8& mouseButtons) const;
	
    virtual void getRelativeMouseState(int& x, int& y, uint8& mouseButtons) const;
	
    virtual void getRelativeMouseState(double& x, double& y, uint8& mouseButtons) const;
	
    virtual void getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button);
	
    virtual void setInputCapture(bool c);
	
    virtual bool inputCapture() const;
	
    virtual void setMouseVisible(bool b);
	
    virtual bool mouseVisible() const;
	
    virtual bool requiresMainLoop() const;

protected:
    virtual void reallyMakeCurrent() const;

    virtual void getOSEvents(Queue<GEvent>& events);

private:
	void enableDirectInput() const;
};


} // namespace G3D


#endif // G3D_WIN32

#endif // G3D_WIN32WINDOW_H

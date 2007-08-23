/**
  @file CarbonWindow.h
  
  @maintainer Casey O'Donnell, caseyodonnell@gmail.com
  @created 2006-08-20
  @edited  2007-08-23
*/

#ifndef G3D_CARBONWINDOW_H
#define G3D_CARBONWINDOW_H

#include "G3D/platform.h"
#include "G3D/Set.h"
#include "G3D/Rect2D.h"

#ifdef G3D_OSX

#include "GLG3D/GWindow.h"
#include "GLG3D/glcalls.h"

#include <Carbon/Carbon.h>
#include <OpenGL/Opengl.h>
#include <OpenGL/CGLTypes.h>
#include <AGL/agl.h>

namespace G3D {

namespace _internal {
pascal OSStatus OnWindowSized(EventHandlerCallRef handlerRef, EventRef event, void *userData);
pascal OSStatus OnWindowClosed(EventHandlerCallRef handlerRef, EventRef event, void *userData);
pascal OSErr OnDragReceived(WindowRef theWindow, void *userData, DragRef theDrag);
}

class CarbonWindow : public GWindow {
private:
	// Window Settings
	Vector2			_clientRectOffset;
	Vector2			_clientXY;
	Settings		_settings;
	std::string		_title;
	CFStringRef		_titleRef;
	
	// Process to Front Initialization
	static bool		_ProcessBroughtToFront;
	
	// State Information
	bool			_mouseVisible;
	bool			_inputCapture;
	bool			_windowActive;
	
	bool			_receivedCloseEvent;

    /** Mouse Button State Array: false - up, true - down
        [0] - left, [1] - middle, [2] - right, [3] - X1,  [4] - X2 */
	bool			_mouseButtons[8];
	bool			_keyboardButtons[256];
	
	// TODO: Add Input Device Array
	
	// Carbon Window Data
	WindowRef 		_window;
	AGLContext		_glContext;
	AGLDrawable		_glDrawable;
	
	const bool		_createdWindow;

	// Make Event Handlers Capable of Seeing Private Parts
	friend pascal OSStatus _internal::OnWindowSized(EventHandlerCallRef handlerRef, EventRef event, void *userData);
	friend pascal OSStatus _internal::OnWindowClosed(EventHandlerCallRef handlerRef, EventRef event, void *userData);
	friend pascal OSErr _internal::OnDragReceived(WindowRef theWindow, void *userData, DragRef theDrag);
	
	static EventTypeSpec _resizeSpec[];
	static EventTypeSpec _closeSpec;
	
	Array<GEvent>       _sizeEventInjects;
	Array<GEvent>		_dropEventInjects;
	Array<std::string>	_droppedFiles;

	void injectSizeEvent(int width, int height) {
		GEvent e;
		e.type = GEventType::VIDEO_RESIZE;
		e.resize.w = width;
		e.resize.h = height;
		_sizeEventInjects.append(e);
	}
	
	void injectDropEvent(int x, int y) {
		GEvent e;
		e.type = GEventType::FILE_DROP;
		e.drop.x = x;
		e.drop.y = y;
		_dropEventInjects.append(e);
	}
	
	bool makeMouseEvent(EventRef theEvent, GEvent& e);
	
	/** Called from all constructors */
	void init(WindowRef window, bool creatingShareWindow = false);
	
	static std::auto_ptr<CarbonWindow> _shareWindow;
	
	static void createShareWindow(GWindow::Settings s);
	
	/** Constructs from a new window*/
	explicit CarbonWindow(const GWindow::Settings& settings, bool creatingShareWindow = false);
	
	/** Constructs from an existing window */
	explicit CarbonWindow(const GWindow::Settings& settings, WindowRef window);
	
	CarbonWindow& operator=(const CarbonWindow& other);

public:

	static CarbonWindow* create(const GWindow::Settings& settings = GWindow::Settings());
	
	static CarbonWindow* create(const GWindow::Settings& settings, WindowRef window);
	
	virtual ~CarbonWindow();
	
	std::string getAPIVersion() const;
	std::string getAPIName() const;
	
	/** The WindowRef of this Object */
	inline  WindowRef windowref() const {
		return _window;
	}
	
	virtual void getSettings(GWindow::Settings& settings) const;
	
	virtual int width() const;
	virtual int height() const;
	
	virtual Rect2D dimensions() const;
	virtual void setDimensions(const Rect2D &dims);
	
	virtual void getDroppedFilenames(Array<std::string>& files);
	
	virtual void setPosition(int x, int y) {
		setDimensions( Rect2D::xywh((float)x, (float)y, (float)_settings.width, (float)_settings.height) );
	}
	
	virtual bool hasFocus() const;
	
	virtual void setGammaRamp(const Array<uint16>& gammaRamp);
	
	virtual void setCaption(const std::string& title);
	virtual std::string caption() ;
	
	virtual int numJoysticks() const;
	
	virtual std::string joystickName(unsigned int stickNum);
	
	virtual void notifyResize(int w, int h);
	
	virtual void setRelativeMousePosition(double x, double y);
	virtual void setRelativeMousePosition(const Vector2 &p);
	virtual void getRelativeMouseState(Vector2 &position, uint8 &mouseButtons) const;
	virtual void getRelativeMouseState(int &x, int &y, uint8 &mouseButtons) const;
	virtual void getRelativeMouseState(double &x, double &y, uint8 &mouseButtons) const;
	virtual void getJoystickState(unsigned int stickNum, Array<float> &axis, Array<bool> &buttons);
	
	virtual void setInputCapture(bool c);
	virtual bool inputCapture() const;
	
	virtual void setMouseVisible(bool b);
	virtual bool mouseVisible() const;
	
	virtual bool requiresMainLoop() const {
		return false;
	}
	
	virtual void swapGLBuffers();

protected:

	virtual bool pollOSEvent(GEvent& e);
	
}; // CarbonWindow

} // namespace G3D


#endif // G3D_OSX

#endif // G3D_CARBONWINDOW_H

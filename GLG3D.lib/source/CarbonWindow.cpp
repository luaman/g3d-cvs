/**
  @file CarbonWindow.h
  
  @maintainer Casey O'Donnell, caseyodonnell@gmail.com
  @created 2006-08-24
  @edited  2007-04-03
*/

#include "G3D/platform.h"
#include "GLG3D/GLCaps.h"

// This file is ignored on other platforms
#ifdef G3D_OSX

#include "G3D/Log.h"
#include "GLG3D/CarbonWindow.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/UserInput.h"

#define OSX_MENU_BAR_HEIGHT 45
#define OSX_WINDOW_TITLE_HEIGHT 22
#define OSX_RESIZE_DIMS 10

namespace G3D {

#pragma mark Private:

std::auto_ptr<CarbonWindow> CarbonWindow::_shareWindow(NULL);

namespace _internal {
pascal OSStatus OnWindowSized(EventHandlerCallRef handlerRef, EventRef event, void *userData) {
    CarbonWindow* pWindow = (CarbonWindow*)userData;
    
    if(pWindow) {
        WindowRef win = NULL;
        if(GetEventParameter(event,kEventParamDirectObject,typeWindowRef,NULL,sizeof(WindowRef),NULL,&win)==noErr) {
            Rect rect;
            if(GetWindowBounds(win, kWindowContentRgn, &rect)==noErr) {
                pWindow->injectSizeEvent(rect.right-rect.left, rect.bottom-rect.top);
            }
        }
    }
	
    return eventNotHandledErr;
}

pascal OSStatus OnWindowClosed(EventHandlerCallRef handlerRef, EventRef event, void *userData) {
    CarbonWindow* pWindow = (CarbonWindow*)userData;
    
    if(pWindow) {
        pWindow->_receivedCloseEvent = true;
    }
    
    return eventNotHandledErr;
}
} // namespace _internal

// Static Variables for Standard Event Handler Installation
bool CarbonWindow::_runApplicationEventLoopCalled = false;
EventLoopTimerRef CarbonWindow::_timer = NULL;
EventLoopTimerUPP CarbonWindow::_timerUPP = NULL;

// Static Event Type Specs
EventTypeSpec CarbonWindow::_resizeSpec[] = {{kEventClassWindow, kEventWindowResizeCompleted},{kEventClassWindow, kEventWindowZoomed}};
EventTypeSpec CarbonWindow::_closeSpec = {kEventClassWindow, kEventWindowClose};

// Static Helper Functions Prototypes
static unsigned char makeKeyEvent(EventRef,GEvent&);
static uint8 buttonsToUint8(const bool*);

pascal void CarbonWindow::quitApplicationEventLoop(EventLoopTimerRef inTimer, void* inUserData) {
	QuitApplicationEventLoop();
}

void CarbonWindow::init(WindowRef window, bool creatingShareWindow /*= false*/) {
    // Initialize mouse buttons to up
    _mouseButtons[0] = _mouseButtons[1] = _mouseButtons[2] = false;

    // Clear all keyboard buttons to up (not down)
    memset(_keyboardButtons, 0, sizeof(_keyboardButtons));

    if (! creatingShareWindow) {
        GLCaps::init();
        setCaption(_settings.caption);
    }
}

void CarbonWindow::createShareWindow(GWindow::Settings s) {
	static bool hasInited = false;
	
	if (hasInited) {
		return;
	}
	
	hasInited = true;

	// We want a small (low memory), invisible window
	s.visible = false;
	s.width = 16;
	s.height = 16;
	s.framed = false;

	_shareWindow.reset(new CarbonWindow(s, true));
}

CarbonWindow::CarbonWindow(const GWindow::Settings& s, bool creatingShareWindow /*= false*/) : _createdWindow(true) {
	// If we haven't yet initialized our standard event hanlders
	// by having RunApplicationEventLoopCalled, we have to do that
	// now.
	if(!_runApplicationEventLoopCalled) {
		EventLoopRef mainLoop;
		
		mainLoop = GetMainEventLoop();
		
		_timerUPP = NewEventLoopTimerUPP(quitApplicationEventLoop);
		InstallEventLoopTimer(mainLoop,kEventDurationMicrosecond,kEventDurationForever,_timerUPP,NULL,&_timer);
		
		RunApplicationEventLoop();
		
		// Hack to get our window/process to the front...
		ProcessSerialNumber psn = { 0, kCurrentProcess};    
		TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		SetFrontProcess (&psn);

		RemoveEventLoopTimer(_timer);
		_timer=NULL;
		DisposeEventLoopTimerUPP(_timerUPP);
		_timerUPP = NULL;
		
		_runApplicationEventLoopCalled = true;
	}
	
	OSStatus osErr = noErr;

	_inputCapture = false;
	_mouseVisible = true;
	_receivedCloseEvent = false;
	_windowActive = false;
	_settings = s;	
	
	GLint attribs[100];
	int i = 0;
	
	attribs[i++] = AGL_RED_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_GREEN_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_BLUE_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_ALPHA_SIZE;			attribs[i++] = _settings.alphaBits;

	attribs[i++] = AGL_DEPTH_SIZE;			attribs[i++] = _settings.depthBits;
	attribs[i++] = AGL_STENCIL_SIZE;		attribs[i++] = _settings.stencilBits;

	if (_settings.stereo)					attribs[i++] = AGL_STEREO;

	// use component colors, not indexed colors
	attribs[i++] = AGL_RGBA;				attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_WINDOW;				attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_DOUBLEBUFFER;		attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_PBUFFER;				attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_NO_RECOVERY;			attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_NONE;
	attribs[i++] = NULL;
	
	AGLPixelFormat format;
	
	// If the user is creating a windowed application that is above the menu
	// bar, it will be confusing. We'll move it down for them so it makes more
	// sense. ? ? Possible to GetMenuHeight() ?
	if(! _settings.fullScreen) {
		if(_settings.y <= OSX_MENU_BAR_HEIGHT)
			_settings.y = OSX_MENU_BAR_HEIGHT;
	}
	else {
		// TODO: Add fullscreen code.
	}

	Rect rWin = {_settings.y, _settings.x, _settings.height+_settings.y, _settings.width+_settings.x};
	
	_title = _settings.caption;
	_titleRef = NULL;

	_titleRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingMacRoman);
	
	_window = NewCWindow(NULL, &rWin, CFStringGetPascalStringPtr(_titleRef, kCFStringEncodingMacRoman), _settings.visible, zoomDocProc, (WindowPtr) -1L, true, 0);

	osErr = InstallStandardEventHandler(GetWindowEventTarget(_window));
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowSized), GetEventTypeCount(_resizeSpec), &_resizeSpec[0], this, NULL);
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowClosed), GetEventTypeCount(_closeSpec), &_closeSpec, this, NULL);
	
	_glDrawable = (AGLDrawable) GetWindowPort(_window);
	
	format = aglChoosePixelFormat(NULL,0,attribs);
	
	_glContext = aglCreateContext(format,NULL/*TODO: Share context*/);
	
	aglSetDrawable(_glContext, _glDrawable);
	aglSetCurrentContext(_glContext);

	init(_window);
}

CarbonWindow::CarbonWindow(const GWindow::Settings& s, WindowRef window) : _createdWindow(false) {
	// TODO: Fill with code!
}

#pragma mark Public:

CarbonWindow* CarbonWindow::create(const GWindow::Settings& s /*= GWindow::Settings()*/) {
	return new CarbonWindow(s);
}

CarbonWindow* CarbonWindow::create(const GWindow::Settings& s, WindowRef window) {
	return new CarbonWindow(s, window);
}

CarbonWindow::~CarbonWindow() {
	aglSetCurrentContext(NULL);
	aglDestroyContext(_glContext);
	
	if(NULL == _titleRef)
		CFRelease(_titleRef);
	
	if(_createdWindow)
		DisposeWindow(_window);
}

std::string CarbonWindow::getAPIVersion() const {
	return "0.2";
}

std::string CarbonWindow::getAPIName() const {
	return "Carbon Window";
}

void CarbonWindow::getSettings(GWindow::Settings& s) const {
	s = _settings;
}

int CarbonWindow::width() const {
	return _settings.width;
}

int CarbonWindow::height() const {
	return _settings.height;
}

Rect2D CarbonWindow::dimensions() const {
	return Rect2D::xyxy(_settings.x,_settings.y,_settings.x+_settings.width,_settings.y+_settings.height);
}

void CarbonWindow::setDimensions(const Rect2D &dims) {
	// TODO: Fill in with code to set window dimensions.
}

void CarbonWindow::getDroppedFilenames(Array<std::string>& files) {
	// TODO: Fill in with code to get names of files dropped on window.
}

bool CarbonWindow::hasFocus() const {
	// TODO: Fill in with code to determin focus.
	return true;
}

void CarbonWindow::setGammaRamp(const Array<uint16>& gammaRamp) {
	// TODO: Fill in with gamma adjustement code.
}

void CarbonWindow::setCaption(const std::string& title) {
	_title = title;

	if(NULL == _titleRef)
		CFRelease(_titleRef);

	_titleRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingMacRoman);
	
	SetWindowTitleWithCFString(_window,_titleRef);
}

std::string CarbonWindow::caption() {
	return _title;
}

int CarbonWindow::numJoysticks() const {
	// TODO: Fill in with joystick code.
	return 0;
}

std::string CarbonWindow::joystickName(unsigned int stickNum) {
	// TODO: Fill in with joystick code.
	return "";
}

void CarbonWindow::notifyResize(int w, int h) {
	_settings.width = w;
	_settings.height = h;
}

void CarbonWindow::setRelativeMousePosition(double x, double y) {
	CGPoint point;
	point.x = x;
	point.y = y;
	CGDisplayMoveCursorToPoint(0,point);
}

void CarbonWindow::setRelativeMousePosition(const Vector2 &p) {
	setRelativeMousePosition(p.x,p.y);
}

void CarbonWindow::getRelativeMouseState(Vector2 &position, uint8 &mouseButtons) const {
	int x, y;
	getRelativeMouseState(x,y,mouseButtons);
	position.x = x;
	position.y = y;
}

void CarbonWindow::getRelativeMouseState(int &x, int &y, uint8 &mouseButtons) const {
	Point point;
	GetGlobalMouse(&point);
	
	if(!_settings.fullScreen) {
		x = point.h - _settings.x;
		y = point.v - _settings.y - OSX_WINDOW_TITLE_HEIGHT;
	}
	else {
		x = point.h;
		y = point.v;
	}
	
	mouseButtons = buttonsToUint8(_mouseButtons);
}

void CarbonWindow::getRelativeMouseState(double &x, double &y, uint8 &mouseButtons) const {
	int ix, iy;
	getRelativeMouseState(ix,iy,mouseButtons);
	x = ix;
	y = iy;
}

void CarbonWindow::getJoystickState(unsigned int stickNum, Array<float> &axis, Array<bool> &buttons) {
	// TODO: Fill in with joystick code.
}

void CarbonWindow::setInputCapture(bool c) {
	// TODO: Fill in with input capturing code.
}

bool CarbonWindow::inputCapture() const {
	return _inputCapture;
}

void CarbonWindow::setMouseVisible(bool b) {
	// TODO: Fill in with mouse invisible code.
	// CGCursorIsVisible()
}

bool CarbonWindow::mouseVisible() const {
	return _mouseVisible;
}

void CarbonWindow::swapGLBuffers() {
	if(_glContext) {
		aglSetCurrentContext(_glContext);
		aglSwapBuffers(_glContext);
	}
}

bool CarbonWindow::makeMouseEvent(EventRef theEvent, GEvent& e) {
	UInt32 eventKind = GetEventKind(theEvent);
	HIPoint point;
	EventMouseButton button;
	Rect rect;
	
	GetEventParameter(theEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &point);

	if(GetWindowBounds(_window, kWindowContentRgn, &rect)==noErr) {
		// If the mouse event didn't happen in our content region, then
		// we want to punt it to other event handlers. (Return FALSE)
		if((point.x >= rect.left) && (point.y >= rect.top) && (point.x <= rect.right) && (point.y <= rect.bottom)) {
			// If the user wants to resize, we should allow them to.
			// TODO: Use GetWindowBounds with kWindowGrowRgn rather than my hack.
			if(!_settings.fullScreen && _settings.resizable && ((point.x >= (rect.right - OSX_RESIZE_DIMS)) && (point.y >= (rect.bottom - OSX_RESIZE_DIMS))))
				return false;
			
			GetEventParameter(theEvent, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
		
			switch (eventKind) {
				case kEventMouseDown:
				case kEventMouseUp:
					e.type = (eventKind == kEventMouseDown) ? GEventType::MOUSE_BUTTON_DOWN : GEventType::MOUSE_BUTTON_UP;
					e.button.x = point.x-rect.left;
					e.button.y = point.y-rect.top;

					// Mouse button index
					e.button.which = 0;		// TODO: Which Mouse is Being Used?
					e.button.state = true;
					e.button.button = button - 1;

					_mouseButtons[button - 1] = (eventKind == kEventMouseDown);
					return true;
				case kEventMouseDragged:
				case kEventMouseMoved:
					e.motion.type = GEventType::MOUSE_MOTION;
					e.motion.which = 0;
					e.motion.state = buttonsToUint8(_mouseButtons);
					e.motion.x = point.x-rect.left;
					e.motion.y = point.y-rect.top;
					e.motion.xrel = point.x-rect.left;
					e.motion.yrel = point.x-rect.left;
					return true;
				case kEventMouseWheelMoved:
				default:
					break;
			}
		}
	}

	return false;
}

#pragma mark Protected:

bool CarbonWindow::pollOSEvent(GEvent &e) {
	EventRef		theEvent;
	EventTargetRef	theTarget;
	OSStatus osErr = noErr;
	
	osErr = ReceiveNextEvent(0, NULL,kEventDurationNanosecond,true, &theEvent);

	// If we've gotten no event, we should just return false so that
	// a render pass can occur.
	if(osErr == eventLoopTimedOutErr)
		return false;

	// If we've recieved an event, then we need to convert it into the G3D::GEvent
	// equivalent. This is going to get messy.
	UInt32 eventClass = GetEventClass(theEvent);
	UInt32 eventKind = GetEventKind(theEvent);
	
	switch(eventClass) { 
		case kEventClassMouse:
			// makeMouseEvent will only return true if we need to handle
			// the mouse event. Otherwise it will return false and allow
			// subsequent handlers to deal with the event.
			if(makeMouseEvent(theEvent, e))
				return true;
			break;
		case kEventClassKeyboard:
			switch (eventKind) {
				case kEventRawKeyDown:
				case kEventRawKeyModifiersChanged:
				case kEventRawKeyRepeat:
					e.key.type = GEventType::KEY_DOWN;
					e.key.state = SDL_PRESSED;
					
					_keyboardButtons[makeKeyEvent(theEvent, e)] = true;
					return true;
				case kEventRawKeyUp:
					e.key.type = GEventType::KEY_UP;
					e.key.state = SDL_RELEASED;
					
					_keyboardButtons[makeKeyEvent(theEvent, e)] = false;
					return true;
				case kEventHotKeyPressed:
				case kEventHotKeyReleased:
				default:
					break;
			} break;
		case kEventClassWindow:
			switch (eventKind) {
				case kEventWindowCollapsing:
				case kEventWindowActivated:
				case kEventWindowDrawContent:
				case kEventWindowShown:
				default:
					break;
			}  break;
		case kEventClassApplication:
			switch (eventKind) {
				case kEventAppActivated:
				case kEventAppActiveWindowChanged:
				case kEventAppDeactivated:
				case kEventAppGetDockTileMenu:
				case kEventAppHidden:
				case kEventAppQuit:
				case kEventAppTerminated:
				default:
					break;
			} break;
		case kEventClassCommand:
			switch (eventKind) {
				case kEventCommandProcess:
				case kEventCommandUpdateStatus:
				default:
					break;
			} break;
		case kEventClassTablet:
		case kEventClassMenu:
		case kEventClassTextInput:
		case kEventClassAppleEvent:
		case kEventClassControl:
		case kEventClassVolume:
		case kEventClassAppearance:
		case kEventClassService:
		case kEventClassToolbar:
		case kEventClassToolbarItem:
		case kEventClassToolbarItemView:
		case kEventClassAccessibility:
		case kEventClassSystem:
		default:
			break;
	}
	
	if(_receivedCloseEvent) {
		_receivedCloseEvent = false;
		e.type = GEventType::QUIT;
		return true;
	}
	
	Rect rect;
	if(GetWindowBounds(_window, kWindowStructureRgn, &rect)==noErr) {
		_settings.x = rect.left;
		_settings.y = rect.top;
	}
	
	if(GetWindowBounds(_window, kWindowContentRgn, &rect)==noErr) {
		_settings.width = rect.right-rect.left;
		_settings.height = rect.bottom-rect.top;
	}
	
    if (_sizeEventInjects.size() > 0) {
        e = _sizeEventInjects.last();
        _sizeEventInjects.clear();
		aglSetCurrentContext(_glContext);
		aglUpdateContext(_glContext);
        return true;
    }

	if(osErr == noErr) {
		theTarget = GetEventDispatcherTarget();	
		SendEventToEventTarget (theEvent, theTarget);
		ReleaseEvent(theEvent);
	}
	
	return false;
}

static unsigned char makeKeyEvent(EventRef theEvent, GEvent& e) {
	UniChar uc;
	unsigned char c;
	UInt32 key;
	UInt32 modifiers;
	
	GetEventParameter(theEvent, kEventParamKeyUnicodes, typeUnicodeText, NULL, sizeof(uc), NULL, &uc);
	GetEventParameter(theEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
	GetEventParameter(theEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(key), NULL, &key);
	GetEventParameter(theEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
	
	e.key.keysym.sym = (GKey::Value)c;
	e.key.keysym.scancode = key;
	e.key.keysym.unicode = uc;
	e.key.keysym.mod = (GKeyMod)0;
	
	if(modifiers & cmdKey) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LMETA);
	}
	
	if(modifiers & shiftKey) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LSHIFT);
	}
	
	if(modifiers & alphaLock) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_CAPS);
	}

	if(modifiers & optionKey) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LALT);
	}

	if(modifiers & controlKey) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LCTRL);
	}

	if(modifiers & kEventKeyModifierNumLockMask) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_NUM);
	}

//	if(modifiers & kEventKeyModifierFnMask) {
//		e.key.keysym.mod = e.key.keysym.mod | GKEYMOD_;
//	}

	return c;
}

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

} // namespace G3D

#endif // G3D_OSX

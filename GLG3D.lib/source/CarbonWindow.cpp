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

#define OSX_MENU_WINDOW_TITLE_HEIGHT 45

namespace G3D {

#pragma mark Private:

std::auto_ptr<CarbonWindow> CarbonWindow::_shareWindow(NULL);

namespace _internal {
static pascal OSStatus OnWindowSized(EventHandlerCallRef handlerRef, EventRef event, void *userData) {
    CarbonWindow* pWindow = (CarbonWindow*)userData;
    
    if(pWindow) {
        WindowRef win = NULL;
        if(GetEventParameter(event,kEventParamDirectObject,typeWindowRef,NULL,sizeof(WindowRef),NULL,&win)==noErr) {
            Rect rect;
            if(GetWindowBounds(win, kWindowContentRgn, &rect)==noErr) {
                pWindow->injectSizeEvent(rect.right-rect.left, rect.bottom-rect.top);
                std::cout << "Resize Event (" << rect.right-rect.left << "," << rect.bottom-rect.top << ")\n";
            }
        }
    }
	
    return eventNotHandledErr;
}

static pascal OSStatus OnWindowClosed(EventHandlerCallRef handlerRef, EventRef event, void *userData) {
    CarbonWindow* pWindow = (CarbonWindow*)userData;
    
    if(pWindow) {
        pWindow->_receivedCloseEvent = true;
        std::cout << "Close Event\n";
    }
    
    return eventNotHandledErr;
}
} // namespace _internal

// Static Variables for Standard Event Handler Installation
bool CarbonWindow::_runApplicationEventLoopCalled = false;
EventLoopTimerRef CarbonWindow::_timer = NULL;
EventLoopTimerUPP CarbonWindow::_timerUPP = NULL;

// Static Event Type Specs
EventTypeSpec CarbonWindow::_closeSpec = {kEventClassWindow, kEventWindowClose};
EventTypeSpec CarbonWindow::_resizeSpec = {kEventClassWindow, kEventWindowResizeCompleted};

// Static Helper Functions Prototypes
static unsigned char makeKeyEvent(EventRef,GEvent&);

pascal void CarbonWindow::quitApplicationEventLoop(EventLoopTimerRef inTimer, void* inUserData) {
	QuitApplicationEventLoop();
}

void CarbonWindow::init(WindowRef window, bool creatingShareWindow /*= false*/) {
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
	// sense.
	if(! _settings.fullScreen) {
		if(_settings.y <= OSX_MENU_WINDOW_TITLE_HEIGHT)
			_settings.y = OSX_MENU_WINDOW_TITLE_HEIGHT;
	}

	Rect rWin = {_settings.y, _settings.x, _settings.height+_settings.y, _settings.width+_settings.x};
	
	_title = _settings.caption;

	CFStringRef strRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingUnicode);
	
	_window = NewCWindow(NULL, &rWin, CFStringGetPascalStringPtr(strRef, kCFStringEncodingUnicode), _settings.visible, zoomDocProc, (WindowPtr) -1L, true, 0);

	osErr = InstallStandardEventHandler(GetWindowEventTarget(_window));
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowSized), GetEventTypeCount(_resizeSpec), &_resizeSpec, this, NULL);
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowClosed), GetEventTypeCount(_closeSpec), &_closeSpec, this, NULL);
	
	CFRelease(strRef);

	_glDrawable = (AGLDrawable) GetWindowPort(_window);
	
	format = aglChoosePixelFormat(NULL,0,attribs);
	
	_glContext = aglCreateContext(format,NULL/*TODO: Share context*/);
	
	aglSetDrawable(_glContext, _glDrawable);
	aglSetCurrentContext(_glContext);
}

CarbonWindow::CarbonWindow(const GWindow::Settings& s, WindowRef window) : _createdWindow(false) {
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
	
	if(_createdWindow)
		DisposeWindow(_window);
}

std::string CarbonWindow::getAPIVersion() const {
	return "0.1";
}

std::string CarbonWindow::getAPIName() const {
	return "Carbon";
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
}

void CarbonWindow::getDroppedFilenames(Array<std::string>& files) {
}

bool CarbonWindow::hasFocus() const {
	return true;
}

void CarbonWindow::setGammaRamp(const Array<uint16>& gammaRamp) {
}

void CarbonWindow::setCaption(const std::string& title) {
	_title = title;
	CFStringRef strRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingUnicode);
	
	SetWindowTitleWithCFString(_window,strRef);

	CFRelease(strRef);
}

std::string CarbonWindow::caption() {
	return _title;
}

int CarbonWindow::numJoysticks() const {
	return 0;
}

std::string CarbonWindow::joystickName(unsigned int stickNum) {
	return "";
}

void CarbonWindow::notifyResize(int w, int h) {
	_settings.width = w;
	_settings.height = h;
}

void CarbonWindow::setRelativeMousePosition(double x, double y) {
}

void CarbonWindow::setRelativeMousePosition(const Vector2 &p) {
}

void CarbonWindow::getRelativeMouseState(Vector2 &position, uint8 &mouseButtons) const {
}

void CarbonWindow::getRelativeMouseState(int &x, int &y, uint8 &mouseButtons) const {
}

void CarbonWindow::getRelativeMouseState(double &x, double &y, uint8 &mouseButtons) const {
}

void CarbonWindow::getJoystickState(unsigned int stickNum, Array<float> &axis, Array<bool> &buttons) {
}

void CarbonWindow::setInputCapture(bool c) {
}

bool CarbonWindow::inputCapture() const {
	return _inputCapture;
}

void CarbonWindow::setMouseVisible(bool b) {
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

	UInt32 eventClass = GetEventClass(theEvent);
	UInt32 eventKind = GetEventKind(theEvent);
	
	switch(eventClass) { 
		case kEventClassMouse:	
//			OSStatus result = CallNextEventHandler(handlerRef, event);	
//			if (eventNotHandledErr != result) return result;
			/*else*/
			switch (eventKind) {
				case kEventMouseDown:
				case kEventMouseUp:
				case kEventMouseDragged:
				case kEventMouseMoved:
				case kEventMouseWheelMoved:
				default:
					break;
			} break;
		case kEventClassKeyboard:
			switch (eventKind) {
				case kEventRawKeyDown:
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
				
				case kEventRawKeyModifiersChanged:
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
				case kEventWindowClose:
				case kEventWindowShown:
				case kEventWindowBoundsChanged:
				case kEventWindowZoomed:
				default:
					break;
			}  break;
		case kEventClassCommand:
			switch (eventKind) {
				case kEventCommandProcess:
				case kEventCommandUpdateStatus:
				default:
					break;
			} break;
		case kEventClassTablet:
		case kEventClassApplication:
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
		case kEventClassInk:
		case kEventClassTSMDocumentAccess:
		default:
			break;
	}
	
	if(_receivedCloseEvent)
	{
		_receivedCloseEvent = false;
		e.type = GEventType::QUIT;
		std::cout << "Close Event Sent\n";
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
		std::cout << "Resize Event Sent (" << e.resize.w << "," << e.resize.h << ")\n";
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
	UInt32 modifier;
}

} // namespace G3D

#endif // G3D_OSX

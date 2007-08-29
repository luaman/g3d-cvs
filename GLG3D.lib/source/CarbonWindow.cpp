/**
  @file CarbonWindow.h
  
  @maintainer Casey O'Donnell, caseyodonnell@gmail.com
  @created 2006-08-24
  @edited  2007-08-23
*/

#include "G3D/platform.h"
#include "GLG3D/GLCaps.h"

// This file is ignored on other platforms
#ifdef G3D_OSX

#include "G3D/Log.h"
#include "GLG3D/CarbonWindow.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/UserInput.h"

// TODO: Replace these with proper calls.
#define OSX_MENU_BAR_HEIGHT 45

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

pascal OSErr OnDragReceived(WindowRef theWindow, void *userData, DragRef theDrag) {
	CarbonWindow* pWindow = (CarbonWindow*)userData;
	pWindow->_droppedFiles.clear();

	OSErr osErr = noErr;
	Point point;
	UInt16 iNumItems = 0;
	
	osErr = CountDragItems(theDrag, &iNumItems);
	osErr = GetDragMouse(theDrag, &point, NULL);
	
	for(UInt16 i = 1; i <= iNumItems; i++) {
		DragItemRef itemRef = 0;
		UInt16 iNumFlavors = 0;
		
		osErr = GetDragItemReferenceNumber(theDrag,i,&itemRef);
		osErr = CountDragItemFlavors(theDrag,itemRef,&iNumFlavors);
		
		for(UInt16 j = 1; j <= iNumFlavors; j++) {
			FlavorType flavor;
			
			osErr = GetFlavorType(theDrag,itemRef,j,&flavor);
			
			if(kDragFlavorTypeHFS == flavor) {
				HFSFlavor flavorData;
				Size size = sizeof(flavorData);
				
				osErr = GetFlavorData(theDrag,itemRef,flavorTypeHFS,&flavorData,&size,0);
				
				if(sizeof(flavorData) == size) {
					UInt8 path[2024];
					FSRef fsRef;
					
					osErr = FSpMakeFSRef(&flavorData.fileSpec,&fsRef);
					osErr = FSRefMakePath(&fsRef,&path[0],sizeof(path));
					
					std::string szPath = (const char*)path;
					pWindow->_droppedFiles.append(szPath);
				}
			}
		}
	}
	
	if(pWindow->_droppedFiles.size() > 0) {
		pWindow->injectDropEvent(point.h,point.v);
		return noErr;
	}
	
	return dragNotAcceptedErr;
}
} // namespace _internal

// Static Variables for Brining Process to Front
bool CarbonWindow::_ProcessBroughtToFront = false;

// Static Event Type Specs
EventTypeSpec CarbonWindow::_resizeSpec[] = {{kEventClassWindow, kEventWindowResizeCompleted},{kEventClassWindow, kEventWindowZoomed}};
EventTypeSpec CarbonWindow::_closeSpec = {kEventClassWindow, kEventWindowClose};

// Static Helper Functions Prototypes
static unsigned char makeKeyEvent(EventRef,GEvent&);
static uint8 buttonsToUint8(const bool*);
static OSStatus aglReportError();

void CarbonWindow::init(WindowRef window, bool creatingShareWindow /*= false*/) {
	// Initialize mouse buttons to up
	_mouseButtons[0] = _mouseButtons[1] = _mouseButtons[2] = false;

	// Clear all keyboard buttons to up (not down)
	memset(_keyboardButtons, 0, sizeof(_keyboardButtons));
	memset(_mouseButtons, 0, sizeof(_mouseButtons));

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
	if(!_ProcessBroughtToFront) {
		// Hack to get our window/process to the front...
		ProcessSerialNumber psn = { 0, kCurrentProcess};
		TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		SetFrontProcess (&psn);

		_ProcessBroughtToFront = true;
	}
	
	OSStatus osErr = noErr;

	_inputCapture = false;
	_mouseVisible = true;
	_receivedCloseEvent = false;
	_windowActive = true;
	_settings = s;
	_glDrawable = 0;
	
	GLint attribs[100];
	memset(attribs,AGL_NONE,sizeof(attribs));
	int i = 0;
	
	// use component colors, not indexed colors
	attribs[i++] = AGL_RGBA;				attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_DOUBLEBUFFER;		attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_PBUFFER;				attribs[i++] = GL_TRUE;
	attribs[i++] = AGL_NO_RECOVERY;			attribs[i++] = GL_TRUE;

	if (_settings.fullScreen) {
		attribs[i++] = AGL_FULLSCREEN;		attribs[i++] = GL_TRUE;
	} else {
		attribs[i++] = AGL_WINDOW;			attribs[i++] = GL_TRUE;
	}
	
	if (_settings.stereo) {
		attribs[i++] = AGL_STEREO;			attribs[i++] = GL_TRUE;
	}
	
	attribs[i++] = AGL_RED_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_GREEN_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_BLUE_SIZE;			attribs[i++] = _settings.rgbBits;
	attribs[i++] = AGL_ALPHA_SIZE;			attribs[i++] = _settings.alphaBits;

	attribs[i++] = AGL_DEPTH_SIZE;			attribs[i++] = _settings.depthBits;
	attribs[i++] = AGL_STENCIL_SIZE;		attribs[i++] = _settings.stencilBits;

	attribs[i++] = AGL_NONE;
	
	AGLPixelFormat format;
	
	// If the user is creating a windowed application that is above the menu
	// bar, it will be confusing. We'll move it down for them so it makes more
	// sense.
	if(! _settings.fullScreen) {
		if(_settings.y <= OSX_MENU_BAR_HEIGHT)
			_settings.y = OSX_MENU_BAR_HEIGHT;
	}
	
	// If the user wanted the window centered, then it is likely that our
	// settings.x and .y are not right. We should set those now:
	GDHandle displayHandle;
	CGRect rScreen = CGDisplayBounds(kCGDirectMainDisplay);

	if(_settings.fullScreen) {
		_settings.x = rScreen.origin.x;
		_settings.y = rScreen.origin.y;
		_settings.width = rScreen.size.width;
		_settings.height = rScreen.size.height;

		CGDisplayCapture(kCGDirectMainDisplay);
		osErr = DMGetGDeviceByDisplayID((DisplayIDType)kCGDirectMainDisplay,&displayHandle,false);
	}
	
	if(!_settings.fullScreen && _settings.center) {
		_settings.x = (rScreen.size.width - rScreen.origin.x)/2;
		_settings.x = _settings.x - (_settings.width/2);
		_settings.y = (rScreen.size.height - rScreen.origin.y)/2;
		_settings.y = _settings.y - (_settings.height/2);
	}
	
	Rect rWin = {_settings.y, _settings.x, _settings.height+_settings.y, _settings.width+_settings.x};
	
	osErr = CreateNewWindow(kDocumentWindowClass,kWindowStandardDocumentAttributes|kWindowStandardHandlerAttribute,&rWin,&_window);

	alwaysAssertM(_window != NULL, "Could Not Create Window.");

	_title = _settings.caption;
	CFStringRef titleRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingMacRoman);
	
	osErr = SetWindowTitleWithCFString(_window,titleRef);
	
	CFRelease(titleRef);
	
	// Set default icon if available
	if(_settings.defaultIconFilename != "nodefault") {
		try {
			GImage defaultIcon;
			defaultIcon.load(_settings.defaultIconFilename);

			setIcon(defaultIcon);
		} catch (const GImage::Error& e) {
			// Throw away default icon
			debugPrintf("GWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());
			Log::common()->printf("GWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());
		}
	}
	
	ShowWindow(_window);
	
	osErr = InstallStandardEventHandler(GetWindowEventTarget(_window));
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowSized), GetEventTypeCount(_resizeSpec), &_resizeSpec[0], this, NULL);
	osErr = InstallWindowEventHandler(_window, NewEventHandlerUPP(_internal::OnWindowClosed), GetEventTypeCount(_closeSpec), &_closeSpec, this, NULL);
	osErr = InstallReceiveHandler(NewDragReceiveHandlerUPP(_internal::OnDragReceived),_window,this);
	
	_glDrawable = (AGLDrawable) GetWindowPort(_window);

	if(!_settings.fullScreen)
		format = aglChoosePixelFormat(NULL,0,attribs);
	else
		format = aglChoosePixelFormat(&displayHandle,1,attribs);
	
	osErr = aglReportError();
	
	alwaysAssertM(format != 0, "Unsupported Pixel Format.");
	
	_glContext = aglCreateContext(format,NULL/*TODO: Share context*/);
	
	alwaysAssertM(_glContext != NULL, "Failed to create OpenGL Context.");

	aglDestroyPixelFormat(format);

	if(_settings.fullScreen)
		aglEnable(_glContext, AGL_FS_CAPTURE_SINGLE);
	else
		aglSetDrawable(_glContext, _glDrawable);
		
	osErr = aglReportError();
	
	alwaysAssertM(osErr == noErr, "Error Encountered Enabling Full Screen or Setting Drawable.");
	
	aglSetCurrentContext(_glContext);
	
	if(_settings.fullScreen)
		aglSetFullScreen(_glContext,rScreen.size.width,rScreen.size.height,0,0);

	osErr = aglReportError();
	
	alwaysAssertM(osErr == noErr, "Error Encountered Entering Full Screen Context Rendering.");

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
	
	if(_settings.fullScreen)
		CGDisplayRelease(kCGDirectMainDisplay);
	
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
	CGRect rScreen = CGDisplayBounds(kCGDirectMainDisplay);
	int W = rScreen.size.width;
	int H = rScreen.size.height;
	
	int x = iClamp((int)dims.x0(), 0, W);
	int y = iClamp((int)dims.y0(), 0, H);
	int w = iClamp((int)dims.width(), 1, W);
	int h = iClamp((int)dims.height(), 1, H);

	// Set dimensions and repaint.
	MoveWindow(_window,x,y,false);
	SizeWindow(_window,w,h,true);
}

void CarbonWindow::getDroppedFilenames(Array<std::string>& files) {
	files.fastClear();
	if(_droppedFiles.size() > 0) {
		files.append(_droppedFiles);
	}
}

bool CarbonWindow::hasFocus() const {
	return _windowActive;
}

void CarbonWindow::setGammaRamp(const Array<uint16>& gammaRamp) {
	// Soon to be depricated.
}

void CarbonWindow::setCaption(const std::string& title) {
	_title = title;

	CFStringRef titleRef = CFStringCreateWithCString(kCFAllocatorDefault, _title.c_str(), kCFStringEncodingMacRoman);

	SetWindowTitleWithCFString(_window,titleRef);
	
	CFRelease(titleRef);
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

void CarbonWindow::setIcon(const GImage& image) {
	// SetApplicationDockTileImage
	CGImageRef dockImage = NULL;
	CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGDataProviderRef dataProviderRef = CGDataProviderCreateWithData(NULL,image.byte(),image.sizeInMemory(),NULL);
	
	size_t bPC = (image.channels == 4) ? 8 : 6;
	CGBitmapInfo bmpInfo = (image.channels == 4) ? kCGBitmapByteOrderDefault|kCGImageAlphaLast : kCGBitmapByteOrderDefault;
	
	if((NULL != colorSpaceRef) && (NULL != dataProviderRef)) {
		dockImage = CGImageCreate(	image.width,
									image.height,
									bPC,
									bPC*image.channels,
									image.width * image.channels,
									colorSpaceRef,
									bmpInfo,
									dataProviderRef,
									NULL,
									true,
									kCGRenderingIntentDefault);
	}
	
	if(NULL != colorSpaceRef)
		CGColorSpaceRelease(colorSpaceRef);
	
	if(NULL != dataProviderRef)
		CGDataProviderRelease(dataProviderRef);

	if(NULL != dockImage)
		SetApplicationDockTileImage(dockImage);
	
	if(NULL != dockImage) {
		CGImageRelease(dockImage);
		dockImage = NULL;
	}
}

void CarbonWindow::notifyResize(int w, int h) {
	_settings.width = w;
	_settings.height = h;
}

void CarbonWindow::setRelativeMousePosition(double x, double y) {
	CGPoint point;

	point.x = x + _settings.x;
	point.y = y + _settings.y;
	
	CGSetLocalEventsSuppressionInterval(0.0);
	CGWarpMouseCursorPosition(point);
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
	
	x = point.h - _settings.x;
	y = point.v - _settings.y;

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
	if(_inputCapture == c)
		return;
	
	_inputCapture = c;
}

bool CarbonWindow::inputCapture() const {
	return _inputCapture;
}

void CarbonWindow::setMouseVisible(bool b) {
	if(_mouseVisible == b)
		return;

	_mouseVisible = b;
	
	if(_mouseVisible)
		CGDisplayShowCursor(kCGDirectMainDisplay);
	else
		CGDisplayHideCursor(kCGDirectMainDisplay);
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

#pragma mark Private:

bool CarbonWindow::makeMouseEvent(EventRef theEvent, GEvent& e) {
	UInt32 eventKind = GetEventKind(theEvent);
	HIPoint point;
	EventMouseButton button;
	Rect rect, rectGrow;
	
	GetEventParameter(theEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &point);

	if(GetWindowBounds(_window, kWindowContentRgn, &rect)==noErr) {
		// If we've captured the mouse, we want to force the mouse to stay in our window region.
		if(_inputCapture) {
			bool reposition = false;
			CGPoint newPoint = point;
			
			if(point.x < rect.left) {
				newPoint.x = rect.left;
				reposition = true;
			}
			if(point.x > rect.right) {
				newPoint.x = rect.right;
				reposition = true;
			}
			if(point.y < rect.top) {
				newPoint.y = rect.top;
				reposition = true;
			}
			if(point.y > rect.bottom) {
				newPoint.y = rect.bottom;
				reposition = true;
			}
			
			if(reposition) {
				CGWarpMouseCursorPosition(newPoint);
				point = newPoint;
			}
		}
		
		// If the mouse event didn't happen in our content region, then
		// we want to punt it to other event handlers. (Return FALSE)
		if((point.x >= rect.left) && (point.y >= rect.top) && (point.x <= rect.right) && (point.y <= rect.bottom)) {
			// If the user wants to resize, we should allow them to.
			GetWindowBounds(_window, kWindowGrowRgn, &rectGrow);
			if(!_settings.fullScreen && _settings.resizable && ((point.x >= rectGrow.left) && (point.y >= rectGrow.top)))
				return false;
			
			GetEventParameter(theEvent, kEventParamMouseButton, typeMouseButton, NULL, sizeof(button), NULL, &button);
		
			switch (eventKind) {
				case kEventMouseDown:
				case kEventMouseUp:
					e.type = ((eventKind == kEventMouseDown) || (eventKind == kEventMouseDragged)) ? GEventType::MOUSE_BUTTON_DOWN : GEventType::MOUSE_BUTTON_UP;
					e.button.x = point.x-rect.left;
					e.button.y = point.y-rect.top;

					// Mouse button index
					e.button.which = 0;		// TODO: Which Mouse is Being Used?
					e.button.state = true;

					if(kEventMouseButtonPrimary == button) {
						e.button.button = 0;
						_mouseButtons[0] = (eventKind == kEventMouseDown);
					} else if(kEventMouseButtonTertiary == button) {
						e.button.button = 1;
						_mouseButtons[2] = (eventKind == kEventMouseDown);
					} else if (kEventMouseButtonSecondary == button) {
						e.button.button = 2;
						_mouseButtons[1] = (eventKind == kEventMouseDown);
					}
					return true;
				case kEventMouseDragged:
				case kEventMouseMoved:
					e.motion.type = GEventType::MOUSE_MOTION;
					e.motion.which = 0;		// TODO: Which Mouse is Being Used?
					e.motion.state = buttonsToUint8(_mouseButtons);
					e.motion.x = point.x-rect.left;
					e.motion.y = point.y-rect.top;
					e.motion.xrel = 0;
					e.motion.yrel = 0;
					return true;
				case kEventMouseWheelMoved:
				default:
					break;
			}
		} else if(!_settings.fullScreen && eventKind == kEventMouseDown) {
		// We want to indentify and handle menu events too
		// because we don't have the standard event handlers.
			Point thePoint;
			WindowRef theWindow;
			thePoint.v = point.y;
			thePoint.h = point.x;
			WindowPartCode partCode = FindWindow(thePoint, &theWindow);
			if(partCode == inMenuBar) {
				long iVal = MenuSelect(thePoint);
				short iID = HiWord(iVal);
				short iItem = LoWord(iVal);
				HiliteMenu(iID);
				
				if(iItem == 9)
					_receivedCloseEvent = true;
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
	bool			puntEvent = false;
	
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
			if(_windowActive) {
				if(makeMouseEvent(theEvent, e))
					return true;
			}
			break;
		case kEventClassKeyboard:
			if(_windowActive) {
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
				}
			} break;
		case kEventClassWindow:
			switch (eventKind) {
				case kEventWindowCollapsing:
					_windowActive = false;
					e.active.type = GEventType::ACTIVE;
					e.active.gain = 0;
					e.active.state = SDL_APPMOUSEFOCUS|SDL_APPINPUTFOCUS;
					puntEvent = true;
					break;
				case kEventWindowActivated:
					_windowActive = true;
					e.active.type = GEventType::ACTIVE;
					e.active.gain = 1;
					e.active.state = SDL_APPMOUSEFOCUS|SDL_APPINPUTFOCUS;
					puntEvent = true;
					break;
				case kEventWindowDrawContent:
				case kEventWindowShown:
					return false;
				default:
					break;
			}  break;
		case kEventClassApplication:
			switch (eventKind) {
				case kEventAppActivated:
					_windowActive = true;
					e.active.type = GEventType::ACTIVE;
					e.active.gain = 1;
					e.active.state = SDL_APPACTIVE;
					puntEvent = true;
					break;
				case kEventAppDeactivated:
				case kEventAppHidden:
					_windowActive = false;
					e.active.type = GEventType::ACTIVE;
					e.active.gain = 0;
					e.active.state = SDL_APPACTIVE;
					puntEvent = true;
					break;
				case kEventAppActiveWindowChanged:
					break;
				case kEventAppGetDockTileMenu:
					break;
				case kEventAppQuit:
				case kEventAppTerminated:
					_receivedCloseEvent = true;
				default:
					break;
			} break;
		case kEventClassCommand:
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
	
	if(GetWindowBounds(_window, kWindowContentRgn, &rect)==noErr) {
		_settings.x = rect.left;
		_settings.y = rect.top;
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
	
	if (_dropEventInjects.size() > 0) {
		e = _dropEventInjects.last();
		_dropEventInjects.pop_back();
		return true;
	}

	if(osErr == noErr) {
		theTarget = GetEventDispatcherTarget();	
		SendEventToEventTarget (theEvent, theTarget);
		ReleaseEvent(theEvent);
	}
	
	return puntEvent;
}

static unsigned char makeKeyEvent(EventRef theEvent, GEvent& e) {
	static GKeyMod lastMod = GKEYMOD_NONE;
	UniChar uc;
	unsigned char c;
	UInt32 key;
	UInt32 modifiers;
	
	KeyMap keyMap;
	unsigned char * keyBytes;
	enum {
			/* modifier keys (TODO: need to determine right command key value) */
		kVirtualLShiftKey = 0x038,
		kVirtualLControlKey = 0x03B,
		kVirtualLOptionKey = 0x03A,
		kVirtualRShiftKey = 0x03C,
		kVirtualRControlKey = 0x03E,
		kVirtualROptionKey = 0x03D,
		kVirtualCommandKey = 0x037
	};
	
	GetEventParameter(theEvent, kEventParamKeyUnicodes, typeUnicodeText, NULL, sizeof(uc), NULL, &uc);
	GetEventParameter(theEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
	GetEventParameter(theEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(key), NULL, &key);
	GetEventParameter(theEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
	
	GetKeys(keyMap);
	keyBytes = (unsigned char *)keyMap;
	
	e.key.keysym.scancode = key;
	e.key.keysym.unicode = uc;
	e.key.keysym.mod = (GKeyMod)0;
	
	if(modifiers & shiftKey) {
		if(keyBytes[kVirtualLShiftKey>>3] & (1 << (kVirtualLShiftKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LSHIFT);

		if(keyBytes[kVirtualRShiftKey>>3] & (1 << (kVirtualRShiftKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_RSHIFT);		
	}
	
	if(modifiers & controlKey) {
		if(keyBytes[kVirtualLControlKey>>3] & (1 << (kVirtualLControlKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LCTRL);

		if(keyBytes[kVirtualRControlKey>>3] & (1 << (kVirtualRControlKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_RCTRL);
	}

	if(modifiers & optionKey) {
		if(keyBytes[kVirtualLOptionKey>>3] & (1 << (kVirtualLOptionKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LALT);

		if(keyBytes[kVirtualROptionKey>>3] & (1 << (kVirtualROptionKey&7)))
			e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_RALT);
	}

	if(modifiers & cmdKey) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_LMETA);
	}
	
	if(modifiers & kEventKeyModifierFnMask) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_MODE);
	}

	if(modifiers & alphaLock) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_CAPS);
	}

	if(modifiers & kEventKeyModifierNumLockMask) {
		e.key.keysym.mod = (GKeyMod)(e.key.keysym.mod | GKEYMOD_NUM);
	}
	
	// If c is 0, then we've actually recieved a modifier key event,
	// which takes a little more logic to figure out. Especially since
	// under Carbon there isn't a distinction between right/left hand
	// versions of keys.
	if(c != 0)
		e.key.keysym.sym = (GKey::Value)c;
	else {
		// We must now determine what changed since last time and see
		// if we've got a key-up or key-down. :-p The assumption is a
		// key down, so we must only set e.key.type = GEventType::KEY_UP
		// if we've lost a modifier.
		if(lastMod == GKEYMOD_NONE)
			lastMod = e.key.keysym.mod;
		
		if(lastMod > e.key.keysym.mod) {
			e.key.type = GEventType::KEY_UP;

			switch(e.key.keysym.mod | lastMod) {
				case GKEYMOD_LSHIFT:
					e.key.keysym.sym = GKey::LSHIFT;
				break;
				case GKEYMOD_RSHIFT:
					e.key.keysym.sym = GKey::RSHIFT;
					break;
				case GKEYMOD_LCTRL:
					e.key.keysym.sym = GKey::LCTRL;
					break;
				case GKEYMOD_RCTRL:
					e.key.keysym.sym = GKey::RCTRL;
					break;
				case GKEYMOD_LALT:
					e.key.keysym.sym = GKey::LALT;
					break;
				case GKEYMOD_RALT:
					e.key.keysym.sym = GKey::RALT;
					break;
			}
		} else {
			switch(e.key.keysym.mod & lastMod) {
				case GKEYMOD_LSHIFT:
					e.key.keysym.sym = GKey::LSHIFT;
				break;
				case GKEYMOD_RSHIFT:
					e.key.keysym.sym = GKey::RSHIFT;
					break;
				case GKEYMOD_LCTRL:
					e.key.keysym.sym = GKey::LCTRL;
					break;
				case GKEYMOD_RCTRL:
					e.key.keysym.sym = GKey::RCTRL;
					break;
				case GKEYMOD_LALT:
					e.key.keysym.sym = GKey::LALT;
					break;
				case GKEYMOD_RALT:
					e.key.keysym.sym = GKey::RALT;
					break;
			}
		}
	}

	lastMod = e.key.keysym.mod;

	return e.key.keysym.sym;
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

static OSStatus aglReportError (void) {
	GLenum err = aglGetError();
	if (AGL_NO_ERROR != err) {
		char errStr[256];
		sprintf (errStr, "AGL Error: %s",(char *) aglErrorString(err));
		std::cout << errStr << std::endl;
	}

	if (err == AGL_NO_ERROR)
		return noErr;
	else
		return (OSStatus) err;
}

} // namespace G3D

#endif // G3D_OSX

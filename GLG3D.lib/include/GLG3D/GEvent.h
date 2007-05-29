/**
  @file GEvent.h

  @maintainer Morgan McGuire, morgan@graphics3d.com
  @created 2006-10-20
  @edited  2007-01-27
*/
#ifndef G3D_GEVENT_H
#define G3D_GEVENT_H

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "GLG3D/GKey.h"

namespace G3D {


/* General keyboard/mouse state definitions */
enum { SDL_PRESSED = 0x01, SDL_RELEASED = 0x00 };

//////////////////////////////////////////////////////////

// SDL_joystick.h

/*
 * Get the current state of a POV hat on a joystick
 * The return value is one of the following positions:
 */
#define SDL_HAT_CENTERED	0x00
#define SDL_HAT_UP          0x01
#define SDL_HAT_RIGHT		0x02
#define SDL_HAT_DOWN		0x04
#define SDL_HAT_LEFT		0x08
#define SDL_HAT_RIGHTUP		(SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN	(SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP		(SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN	(SDL_HAT_LEFT|SDL_HAT_DOWN)

/* The joystick structure used to identify an SDL joystick */
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;


/* Enumeration of valid key mods (possibly OR'd together) */
typedef enum {
	GKEYMOD_NONE  = 0x0000,
	GKEYMOD_LSHIFT= 0x0001,
	GKEYMOD_RSHIFT= 0x0002,
	GKEYMOD_LCTRL = 0x0040,
	GKEYMOD_RCTRL = 0x0080,
	GKEYMOD_LALT  = 0x0100,
	GKEYMOD_RALT  = 0x0200,
	GKEYMOD_LMETA = 0x0400,
	GKEYMOD_RMETA = 0x0800,
	GKEYMOD_NUM   = 0x1000,
	GKEYMOD_CAPS  = 0x2000,
	GKEYMOD_MODE  = 0x4000,
	GKEYMOD_RESERVED = 0x8000
} GKeyMod;

#define GKEYMOD_CTRL	(GKEYMOD_LCTRL  | GKEYMOD_RCTRL)
#define GKEYMOD_SHIFT	(GKEYMOD_LSHIFT | GKEYMOD_RSHIFT)
#define GKEYMOD_ALT	    (GKEYMOD_LALT   | GKEYMOD_RALT)
#define GKEYMOD_META	(GKEYMOD_LMETA  | GKEYMOD_RMETA)


/* Keysym structure
   - The scancode is hardware dependent, and should not be used by general
     applications.  If no hardware scancode is available, it will be 0.

   - The 'unicode' translated character is only available when character
     translation is enabled by the SDL_EnableUNICODE() API.  If non-zero,
     this is a UNICODE character corresponding to the keypress.  If the
     high 9 bits of the character are 0, then this maps to the equivalent
     ASCII character:
	char ch;
	if ( (keysym.unicode & 0xFF80) == 0 ) {
		ch = keysym.unicode & 0x7F;
	} else {
		An international character..
	}
 */
class SDL_keysym {
public:
    /** Hardware specific scancode */
    uint8           scancode;			

    /** G3D virtual raw key code */
    GKey::Value     sym;		

    /** Current key modifiers */
    GKeyMod         mod;	

    /** Translated character */
    uint16          unicode;
};

/* This is the mask which refers to all hotkey bindings */
#define SDL_ALL_HOTKEYS		0xFFFFFFFF

///////////////////////////////////////////////////////////

/** Event code enumerations */
class GEventType {
public:
    enum Value { 
       NOEVENT = 0,		/* Unused (do not remove) */
       ACTIVEEVENT,		/* Application loses/gains visibility */
       KEYDOWN,			/* Keys pressed */
       KEYUP,			/* Keys released */
       MOUSEMOTION,		/* Mouse moved */
       MOUSEBUTTONDOWN,		/* Mouse button pressed */
       MOUSEBUTTONUP,		/* Mouse button released */
       JOYAXISMOTION,		/* Joystick axis motion */
       JOYBALLMOTION,		/* Joystick trackball motion */
       JOYHATMOTION,		/* Joystick hat position change */
       JOYBUTTONDOWN,		/* Joystick button pressed */
       JOYBUTTONUP,		/* Joystick button released */
       QUIT,			/* User-requested quit */
       SYSWMEVENT,		/* System specific event */
       EVENT_RESERVEDA,		/* Reserved for future use.. */
       EVENT_RESERVEDB,		/* Reserved for future use.. */
       VIDEORESIZE,		/* User resized video mode */
       VIDEOEXPOSE,		/* Screen needs to be redrawn */
       EVENT_RESERVED2,		/* Reserved for future use.. */
       EVENT_RESERVED3,		/* Reserved for future use.. */
       EVENT_RESERVED4,		/* Reserved for future use.. */
       EVENT_RESERVED5,		/* Reserved for future use.. */
       EVENT_RESERVED6,		/* Reserved for future use.. */
       EVENT_RESERVED7,		/* Reserved for future use.. */
       /* Events USEREVENT through MAXEVENTS-1 are for your use */
       USEREVENT = 24,
       /* This last event is only for bounding internal arrays
  	     It is the number of bits in the event mask datatype -- uint32
        */
       NUMEVENTS = 32
    };
private:

    Value value;

public:

    G3D_DECLARE_ENUM_CLASS_METHODS(GEventType);
};

}

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GEventType);

namespace G3D {

/* Predefined event masks */
#define EVENTMASK(X)	(1<<(X))
enum {
    SDL_ACTIVEEVENTMASK	    = EVENTMASK(GEventType::KEYDOWN),
    SDL_KEYUPMASK		    = EVENTMASK(GEventType::KEYUP),
    SDL_MOUSEMOTIONMASK	    = EVENTMASK(GEventType::MOUSEMOTION),
    SDL_MOUSEBUTTONDOWNMASK	= EVENTMASK(GEventType::MOUSEBUTTONDOWN),
    SDL_MOUSEBUTTONUPMASK	= EVENTMASK(GEventType::MOUSEBUTTONUP),
    SDL_MOUSEEVENTMASK	    = EVENTMASK(GEventType::MOUSEMOTION)|
                              EVENTMASK(GEventType::MOUSEBUTTONDOWN)|
	                          EVENTMASK(GEventType::MOUSEBUTTONUP),
    SDL_JOYAXISMOTIONMASK	= EVENTMASK(GEventType::JOYAXISMOTION),
    SDL_JOYBALLMOTIONMASK	= EVENTMASK(GEventType::JOYBALLMOTION),
    SDL_JOYHATMOTIONMASK	= EVENTMASK(GEventType::JOYHATMOTION),
    SDL_JOYBUTTONDOWNMASK	= EVENTMASK(GEventType::JOYBUTTONDOWN),
    SDL_JOYBUTTONUPMASK	    = EVENTMASK(GEventType::JOYBUTTONUP),
    SDL_JOYEVENTMASK	    = EVENTMASK(GEventType::JOYAXISMOTION)|
                               EVENTMASK(GEventType::JOYBALLMOTION)|
                                EVENTMASK(GEventType::JOYHATMOTION)|
	                          EVENTMASK(GEventType::JOYBUTTONDOWN)|
	                          EVENTMASK(GEventType::JOYBUTTONUP),
    SDL_VIDEORESIZEMASK	    = EVENTMASK(GEventType::VIDEORESIZE),
    SDL_VIDEOEXPOSEMASK	    = EVENTMASK(GEventType::VIDEOEXPOSE),
    SDL_QUITMASK		    = EVENTMASK(GEventType::QUIT),
    SDL_SYSWMEVENTMASK	    = EVENTMASK(GEventType::SYSWMEVENT)
};
#define SDL_ALLEVENTS		0xFFFFFFFF
#undef EVENTMASK

/* Application visibility event structure */
typedef struct SDL_ActiveEvent {
    uint8 type;	/* SDL_ACTIVEEVENT */
    uint8 gain;	/* Whether given states were gained or lost (1/0) */
    uint8 state;	/* A mask of the focus states */
} SDL_ActiveEvent;

/* Keyboard event structure */
typedef struct SDL_KeyboardEvent {
    uint8 type;	/* SDL_KEYDOWN or SDL_KEYUP */
    uint8 which;	/* The keyboard device index */
    uint8 state;	/* SDL_PRESSED or SDL_RELEASED */
    SDL_keysym keysym;
} SDL_KeyboardEvent;

/* Mouse motion event structure */
typedef struct SDL_MouseMotionEvent {
    uint8 type;	/* SDL_MOUSEMOTION */
    uint8 which;	/* The mouse device index */
    uint8 state;	/* The current button state */
    uint16 x, y;	/* The X/Y coordinates of the mouse relative to the window. */
    int16 xrel;	/* The relative motion in the X direction.  Not supported on all platforms. */
    int16 yrel;	/* The relative motion in the Y direction.  Not supported on all platforms. */
} SDL_MouseMotionEvent;

/* Mouse button event structure */
typedef struct SDL_MouseButtonEvent {
	uint8 type;	/* SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP */
	uint8 which;	/* The mouse device index */
	uint8 button;	/* The mouse button index */
	uint8 state;	/* SDL_PRESSED or SDL_RELEASED */
	uint16 x, y;	/* The X/Y coordinates of the mouse at press time */

    // TODO: add     /** Current key modifiers */    GKeyMod         mod;	

} SDL_MouseButtonEvent;

/* Joystick axis motion event structure */
typedef struct SDL_JoyAxisEvent {
	uint8 type;	/* SDL_JOYAXISMOTION */
	uint8 which;	/* The joystick device index */
	uint8 axis;	/* The joystick axis index */
	int16 value;	/* The axis value (range: -32768 to 32767) */
} SDL_JoyAxisEvent;

/* Joystick trackball motion event structure */
typedef struct SDL_JoyBallEvent {
	uint8 type;	/* SDL_JOYBALLMOTION */
	uint8 which;	/* The joystick device index */
	uint8 ball;	/* The joystick trackball index */
	int16 xrel;	/* The relative motion in the X direction */
	int16 yrel;	/* The relative motion in the Y direction */
} SDL_JoyBallEvent;

/* Joystick hat position change event structure */
typedef struct SDL_JoyHatEvent {
	uint8 type;	/* SDL_JOYHATMOTION */
	uint8 which;	/* The joystick device index */
	uint8 hat;	/* The joystick hat index */
	uint8 value;	/* The hat position value:
			    SDL_HAT_LEFTUP   SDL_HAT_UP       SDL_HAT_RIGHTUP
			    SDL_HAT_LEFT     SDL_HAT_CENTERED SDL_HAT_RIGHT
			    SDL_HAT_LEFTDOWN SDL_HAT_DOWN     SDL_HAT_RIGHTDOWN
			   Note that zero means the POV is centered.
			*/
} SDL_JoyHatEvent;

/* Joystick button event structure */
typedef struct SDL_JoyButtonEvent {
	uint8 type;	    /* SDL_JOYBUTTONDOWN or SDL_JOYBUTTONUP */
	uint8 which;	/* The joystick device index */
	uint8 button;	/* The joystick button index */
	uint8 state;	/* SDL_PRESSED or SDL_RELEASED */
} SDL_JoyButtonEvent;

/* The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
typedef struct SDL_ResizeEvent {
	uint8 type;	/* SDL_VIDEORESIZE */
	int w;		/* New width */
	int h;		/* New height */
} SDL_ResizeEvent;

/* The "screen redraw" event */
typedef struct SDL_ExposeEvent {
	uint8 type;	/* SDL_VIDEOEXPOSE */
} SDL_ExposeEvent;

/* The "quit requested" event */
struct GQuitEvent {
	uint8 type;	/* GQUIT */
};

/* A user-defined event type */
typedef struct SDL_UserEvent {
	uint8 type;	/* SDL_USEREVENT through SDL_NUMEVENTS-1 */
	int code;	/* User defined event code */
	void *data1;	/* User defined data pointer */
	void *data2;	/* User defined data pointer */
} SDL_UserEvent;

/* If you want to use this event, you should include SDL_syswm.h */
struct SDL_SysWMmsg;
typedef struct SDL_SysWMmsg SDL_SysWMmsg;
typedef struct SDL_SysWMEvent {
	uint8 type;
	SDL_SysWMmsg *msg;
} SDL_SysWMEvent;

/** 
  General low-level event structure.
  
  Most event processing code looks like:

  <pre>
  switch (event.type) {
  case GEventType::MOUSEBUTTONDOWN:
     ...
     break;
  ...
  }
  </pre>

  See also G3D::GModule::onEvent, G3D::GWindow::pollEvent.

  @cite Based on libsdl's SDL_Event, which is based on X11 and Win32 events
 */
typedef union {
    /** This is a GEventType, but is given uint8 type so that it does not call the constructor. */
	uint8                   type;

	SDL_ActiveEvent         active;
	SDL_KeyboardEvent       key;
	SDL_MouseMotionEvent    motion;
	SDL_MouseButtonEvent    button;
	SDL_JoyAxisEvent        jaxis;
	SDL_JoyBallEvent        jball;
	SDL_JoyHatEvent         jhat;
	SDL_JoyButtonEvent      jbutton;
	SDL_ResizeEvent         resize;
	SDL_ExposeEvent         expose;
	GQuitEvent              quit;
	SDL_UserEvent           user;
	SDL_SysWMEvent          syswm;
} GEvent;

}

#endif

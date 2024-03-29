/**
  @file GEvent.cpp

  @created 	  2007-01-27
  @edited  	  2008-07-14
    
  Copyright 2000-2009, Morgan McGuire.
  All rights reserved.
*/
#include "GLG3D/GEvent.h"
#include "G3D/stringutils.h"

namespace G3D {

std::string GEvent::toString() const {
    switch (type) {
    case GEventType::ACTIVE:
        return "ActiveEvent";

    case GEventType::KEY_UP:
        return format("KeyboardEvent key %d ('%c') + %d up", key.keysym.sym, 
                      (key.keysym.unicode >= 32) ? key.keysym.unicode &0x7F : ' ', 
                      key.keysym.mod);

    case GEventType::KEY_DOWN:
        return format("KeyboardEvent key %d ('%c') + %d down", key.keysym.sym, 
                      (key.keysym.unicode >= 32) ? key.keysym.unicode &0x7F : ' ', 
                      key.keysym.mod);

    case GEventType::MOUSE_MOTION:
        return format("MouseMotionEvent (%d, %d)", motion.x, motion.y);

    case GEventType::MOUSE_BUTTON_CLICK:
        return format("MouseButtonEvent button %d clicked %d times at (%d, %d)", 
                      button.button, button.numClicks, button.x, button.y);

    case GEventType::MOUSE_BUTTON_UP:
        return format("MouseButtonEvent button %d up at (%d, %d)", button.button, button.x, button.y);

    case GEventType::MOUSE_BUTTON_DOWN:
        return format("MouseButtonEvent button %d down at (%d, %d)", button.button, button.x, button.y);

    case GEventType::MOUSE_SCROLL_2D:
        return "MouseScroll2DEvent";

    case GEventType::JOY_AXIS_MOTION:
        return "JoyAxisEvent";

    case GEventType::JOY_BALL_MOTION:
        return "JoyBallEvent";

    case GEventType::JOY_HAT_MOTION:
        return "JoyHatEvent";

    case GEventType::JOY_BUTTON_UP:
        return "JoyButtonEvent (up)";

    case GEventType::JOY_BUTTON_DOWN:
        return "JoyButtonEvent (down)";

    case GEventType::VIDEO_RESIZE:
        return "ResizeEvent";

    case GEventType::FILE_DROP:
        return "FileDropEvent";

    case GEventType::VIDEO_EXPOSE:
        return "ExposeEvent";

    case GEventType::QUIT:
        return "QuitEvent";

    case GEventType::GUI_DOWN:
        return "GuiEvent button down";

    case GEventType::GUI_UP:
        return "GuiEvent button up";

    case GEventType::GUI_ACTION:
        return "GuiEvent action";

    case GEventType::GUI_CANCEL:
        return "GuiEvent cancel";

    case GEventType::GUI_CHANGE:
        return "GuiEvent smooth change";

    case GEventType::GUI_CLOSE:
        return "GuiEvent close";

    default:
        return "Unrecognized Event";
    }
}

}

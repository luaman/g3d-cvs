/**
 @file GLG3D/GuiContainer.h

 @created 2008-07-15
 @edited  2008-07-15

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2008, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GUICONTAINER_H
#define G3D_GUICONTAINER_H

#include "G3D/Rect2D.h"
#include "GLG3D/GuiControl.h"

namespace G3D {


/**
 Base class for controls that contain other controls.

 All coordinates of objects inside a pane are relative to the container's
 clientRect().  
 */
class GuiContainer : public GuiControl {
protected:
    enum {CONTROL_HEIGHT    =  25};
    enum {CONTROL_WIDTH     = 215};
    enum {BUTTON_WIDTH      =  80};
    enum {TOOL_BUTTON_WIDTH =  50};

    Rect2D              m_clientRect;

    GuiContainer(class GuiWindow* gui, const class GuiText& text);
    GuiContainer(class GuiContainer* parent, const class GuiText& text);

public:

    /**
     Finds the visible, enabled control underneath the mouse
     @param control (Output) pointer to the control that the mouse is over
     @param mouse Relative to the parent of this pane.
    */
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const = 0;

    /** Client rect bounds, relative to the parent (or window if
        there is no parent). */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }


    virtual void setRect(const Rect2D& rect);
};

}

#endif

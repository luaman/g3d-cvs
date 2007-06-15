/**
 @file GLG3D/GuiCheckBox.h

 @created 2006-05-01
 @edited  2007-06-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUICHECKBOX_H
#define G3D_GUICHECKBOX_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Check box or toggle button */
class GuiCheckBox : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
public:

    enum Style {BOX_STYLE, BUTTON_STYLE};

protected:

    Pointer<bool>     m_value;
    Style             m_style;
    
    GuiCheckBox(GuiWindow* gui, GuiPane* parent, const GuiCaption& text, Pointer<bool>& value, Style style = BOX_STYLE);

    virtual void render(RenderDevice* rd, const GuiSkinRef& skin) const;

    /** Delivers events when this control is clicked on and when it has the key focus. */
    virtual bool onEvent(const GEvent& event);

public:
};

} // G3D

#endif

/**
 @file GLG3D/GuiCheckBox.h

 @created 2006-05-01
 @edited  2008-02-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2008, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GUICHECKBOX_H
#define G3D_GUICHECKBOX_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"
#include "GLG3D/GuiTheme.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Check box or toggle button */
class GuiCheckBox : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
public:

    virtual void setRect(const Rect2D& rect);

protected:

    Pointer<bool>            m_value;
    GuiTheme::CheckBoxStyle  m_style;
    
    GuiCheckBox(GuiPane* parent, const GuiCaption& text, const Pointer<bool>& value, GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE);

    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    /** Delivers events when this control is clicked on and when it has the key focus. */
    virtual bool onEvent(const GEvent& event);

public:

    virtual bool toolStyle() const { 
        return m_style == GuiTheme::TOOL_CHECK_BOX_STYLE;
    }
};

} // G3D

#endif

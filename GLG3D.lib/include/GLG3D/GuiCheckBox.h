/**
 @file GLG3D/GuiCheckBox.h

 @created 2006-05-01
 @edited  2009-09-01

 G3D Library http://g3d.sf.net
 Copyright 2000-2010, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiCheckBox_h
#define G3D_GuiCheckBox_h

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

public:    
    /** Called by GuiContainer.  See GuiPane::addCheckBox */
    GuiCheckBox(GuiContainer* parent, const GuiText& text, const Pointer<bool>& value, GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE);

    /** Called by GuiContainer.*/
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    /** Called by GuiContainer. Delivers events when this control is clicked on and when it has the key focus. */
    virtual bool onEvent(const GEvent& event);

    /** True if this is a tool button */
    virtual bool toolStyle() const { 
        return m_style == GuiTheme::TOOL_CHECK_BOX_STYLE;
    }
};

} // G3D

#endif

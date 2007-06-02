/**
 @file GLG3D/GuiControl.h

 @created 2006-05-01
 @edited  2007-06-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef GUICONTROL_H
#define GUICONTROL_H

#include <string>
#include "GLG3D/GuiSkin.h"
#include "GLG3D/Widget.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Base class for all controls. */
class GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
protected:

    bool              m_enabled;

    /** The window that ultimately contains this control */
    GuiWindow*        m_gui;

    /** Parent pane */
    GuiPane*          m_parent;

    /** Relative to the enclosing pane's clientRect */
    Rect2D            m_rect;
    GuiText           m_text;
    bool              m_visible;

    GuiControl(GuiWindow* gui, GuiPane* parent);
    GuiControl(GuiWindow* gui, GuiPane* parent, const GuiText& text);

public:

    virtual ~GuiControl() {}
    bool enabled() const;
    bool mouseOver() const;
    bool visible() const;
    void setVisible(bool b);
    bool focused() const;
    void setEnabled(bool e);
    const GuiText& text() const;
    void setText(const GuiText& text);
    const Rect2D& rect() const;
    virtual void setRect(const Rect2D& rect);

protected:

    virtual void render(RenderDevice* rd, const GuiSkinRef& skin) const = 0;

    /** Events are only delivered to a control when the control that
        control has the key focus (which is transferred during a mouse
        down)
    */
    virtual bool onEvent(const GEvent& event) { return false; }
};

}

#endif

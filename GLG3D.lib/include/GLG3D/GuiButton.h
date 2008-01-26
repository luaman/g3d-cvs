/**
 @file GLG3D/GuiButton.h

 @created 2006-05-01
 @edited  2007-06-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIBUTTON_H
#define G3D_GUIBUTTON_H

#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiSkin.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Push button that can be temporarily pressed.  When the button has been pressed and released,
    a G3D::GuiEvent of type G3D::GEventType::GUI_ACTION is fired on the containing window.

    See also GuiRadioButton and GuiCheckBox for creating buttons that stay down
    when pressed.
*/
class GuiButton : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:

    /** Is the mouse currently down over this control? */
    bool m_down;

    GuiSkin::ButtonStyle m_style;

    /** Called by GuiPanew */
    GuiButton(GuiPane*, const GuiCaption& text, GuiSkin::ButtonStyle style);

    /** Called by GuiWindow */
    virtual void render(RenderDevice* rd, const GuiSkinRef& skin) const;

    virtual bool onEvent(const GEvent& event);
    
public:
};

} // G3D

#endif

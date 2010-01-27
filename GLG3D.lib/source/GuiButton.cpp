/**
 @file GuiButton.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2007-06-02
 @edited  2010-01-13
 */
#include "G3D/platform.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiButton::GuiButton(GuiContainer* parent, const GuiButton::Callback& callback, const GuiText& text, GuiTheme::ButtonStyle style) : 
    GuiControl(parent, text), 
    m_down(false), m_callback(callback), m_style(style) {}


void GuiButton::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    (void)rd;
    if (m_visible) {
        skin->renderButton(m_rect, m_enabled, focused() || mouseOver(), 
                           m_down && mouseOver(), m_caption, (GuiTheme::ButtonStyle)m_style);
    }
}


bool GuiButton::onEvent(const GEvent& event) {
    switch (event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        m_down = true;
        // invoke the pre-event handler
        m_callback.execute();
        debugAssertGLOk();
        fireEvent(GEventType::GUI_DOWN);
        debugAssertGLOk();
        return true;
    
    case GEventType::MOUSE_BUTTON_UP:
        fireEvent(GEventType::GUI_UP);

        // Only trigger an action if the mouse was still over the control
        if (m_down && m_rect.contains(Vector2(event.button.x, event.button.y))) {
            fireEvent(GEventType::GUI_ACTION);
        }

        m_down = false;
        return true;
    }

    return false;
}

}

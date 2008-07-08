/**
 @file GuiButton.cpp
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2007-06-02
 @edited  2008-06-20
 */
#include "G3D/platform.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiButton::GuiButton(GuiContainer* parent, const GuiButton::Callback& callback, const GuiCaption& text, GuiTheme::ButtonStyle style) : 
    GuiControl(parent, text), 
    m_down(false), m_callback(callback), m_style(style) {}


void GuiButton::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    if (m_visible) {
        skin->renderButton(m_rect, m_enabled, focused() || mouseOver(), 
                           m_down && mouseOver(), m_caption, (GuiTheme::ButtonStyle)m_style);
    }
}


bool GuiButton::onEvent(const GEvent& event) {
    switch (event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        m_down = true;
        {
			// invoke the pre-event handler
			m_callback.onPush();

            GEvent response;
            response.gui.type = GEventType::GUI_DOWN;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);
            return true;
        }
    
    case GEventType::MOUSE_BUTTON_UP:
        {
            GEvent response;
            response.gui.type = GEventType::GUI_UP;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);
        }

        // Only trigger an action if the mouse was still over the control
        if (m_down && m_rect.contains(Vector2(event.button.x, event.button.y))) {
            fireActionEvent();
        }

        m_down = false;
        return true;
    }

    return false;
}

}

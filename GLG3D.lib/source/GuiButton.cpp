#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiButton::GuiButton(GuiWindow* gui, GuiPane* parent, const GuiText& text) : GuiControl(gui, parent, text), m_down(false) {}

void GuiButton::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        skin->renderButton(m_rect, m_enabled, focused() || mouseOver(), m_down && mouseOver(), m_text);
    }
}

bool GuiButton::onEvent(const GEvent& event) {
    switch (event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        m_down = true;
        {
            GEvent response;
            response.gui.type = GEventType::GUI_DOWN;
            response.gui.control = this;
            m_gui->fireEvent(response);
            return true;
        }
    
    case GEventType::MOUSE_BUTTON_UP:
        {
            GEvent response;
            response.gui.type = GEventType::GUI_UP;
            response.gui.control = this;
            m_gui->fireEvent(response);
        }

        // Only trigger an action if the mouse was still over the control
        if (m_down && m_rect.contains(Vector2(event.button.x, event.button.y))) {
            // Fire the button pressed event
            GEvent response;
            response.gui.type = GEventType::GUI_ACTION;
            response.gui.control = this;
            m_gui->fireEvent(response);
        }

        m_down = false;
        return true;
    }

    return false;
}

}

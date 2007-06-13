#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiCheckBox::GuiCheckBox(GuiWindow* gui, GuiPane* parent, const GuiCaption& text, bool* value, Style style) 
    : GuiControl(gui, parent, text), m_value(value), m_style(style) {}


void GuiCheckBox::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        if (m_style == BOX_STYLE) {
            skin->renderCheckBox(m_rect, m_enabled, focused() || mouseOver(), *m_value, m_caption);
        } else {
            skin->renderButton(m_rect, m_enabled, focused() || mouseOver(), *m_value, m_caption);
        }
    }
}


bool GuiCheckBox::onEvent(const GEvent& event) {
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && m_visible) {
        *m_value = ! *m_value;
        return true;
    } else {
        return false;
    }
}


}

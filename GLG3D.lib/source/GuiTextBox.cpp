#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiTextBox::GuiTextBox(GuiWindow* gui, GuiPane* parent, const GuiText& caption, std::string* value, Update update, float captionWidth) 
    : GuiControl(gui, parent, caption), m_value(value), m_update(update), m_cursor("|"), m_captionWidth(captionWidth) {
}


void GuiTextBox::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        skin->renderLabel(Rect2D::xywh(m_rect.x0() - m_captionWidth, m_rect.y0(), m_captionWidth, m_rect.height()), m_caption, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);

        // Note that textbox does not have a mouseover state
        skin->renderTextBox(m_rect, m_enabled, focused(), *m_value, m_cursor, m_cursorPosition);
    }
}


bool GuiTextBox::onEvent(const GEvent& event) {
    return false;
    /*
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && m_visible) {
        *m_value = ! *m_value;
        return true;
    } else {
        return false;
        }*/
}


}

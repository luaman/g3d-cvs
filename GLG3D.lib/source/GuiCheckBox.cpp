#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiCheckBox::GuiCheckBox(GuiPane* parent, const GuiCaption& text, 
                         const Pointer<bool>& value, GuiSkin::CheckBoxStyle style) 
    : GuiControl(parent, text), m_value(value), m_style(style) {}


void GuiCheckBox::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        switch (m_style) {
        case GuiSkin::NORMAL_CHECK_BOX_STYLE:
            skin->renderCheckBox(m_rect, m_enabled, focused() || mouseOver(), *m_value, m_caption);
            break;

        default:
            skin->renderButton(m_rect, m_enabled, focused() || mouseOver(), *m_value, m_caption, 
                               (m_style == GuiSkin::BUTTON_CHECK_BOX_STYLE) ? 
                               GuiSkin::NORMAL_BUTTON_STYLE : GuiSkin::TOOL_BUTTON_STYLE);
        }
    }
}


void GuiCheckBox::setRect(const Rect2D& rect) {
     if (m_style == GuiSkin::NORMAL_CHECK_BOX_STYLE) {
         // TODO: use the actual font size etc. to compute bounds
         // Prevent the checkbox from stealing clicks very far away
         m_rect = rect;
         m_clickRect = Rect2D::xywh(rect.x0y0(), Vector2(min(rect.width(), 30.0f), rect.height()));
     } else {
         GuiControl::setRect(rect);
     }
}


bool GuiCheckBox::onEvent(const GEvent& event) {
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && m_visible) {
        *m_value = ! *m_value;
        fireActionEvent();
        return true;
    } else {
        return false;
    }
}


}

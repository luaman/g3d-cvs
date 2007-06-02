#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiLabel::GuiLabel(GuiWindow* gui, GuiPane* parent, const GuiText& text, GFont::XAlign x, GFont::YAlign y) 
    : GuiControl(gui, parent, text), m_xalign(x), m_yalign(y) {}


void GuiLabel::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        skin->renderLabel(m_rect, m_text, m_xalign, m_yalign);
    }
}


}

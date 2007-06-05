#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiWindow.h"

namespace G3D {

bool GuiControl::mouseOver() const {
    return m_gui->mouseOverGuiControl == this;
}
        
bool GuiControl::focused() const {
    // In focus if this control has the key focus and this window is in focus
    return (m_gui->keyFocusGuiControl == this) && m_gui->focused();
}

bool GuiControl::visible() const {
    return m_visible;
}

void GuiControl::setVisible(bool v) {
    m_visible = v;
}

bool GuiControl::enabled() const {
    return m_enabled;
}

void GuiControl::setEnabled(bool e) {
    m_enabled = e;
}

const GuiText& GuiControl::text() const {
    return m_text;
}

void GuiControl::setText(const GuiText& text) {
    m_text = text;
}

const Rect2D& GuiControl::rect() const {
    return m_rect;
}

void GuiControl::setRect(const Rect2D& rect) {
    m_rect = rect;
}

GuiControl::GuiControl(GuiWindow* gui, GuiPane* parent) : m_enabled(true), m_gui(gui), m_parent(parent), m_visible(true) {}

GuiControl::GuiControl(GuiWindow* gui, GuiPane* parent, const GuiText& text) : m_enabled(true), m_gui(gui), m_parent(parent), m_text(text), m_visible(true) {}

}
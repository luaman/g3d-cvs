/**
 @file GuiDropDownList.cpp
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2007-06-02
 @edited  2008-02-19
 */
#include "G3D/platform.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {


GuiDropDownList::GuiDropDownList
(GuiContainer*               parent, 
 const GuiCaption&           caption, 
 const Pointer<int>&         indexValue, 
 const Array<GuiCaption>&    listValue) : GuiControl(parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_listValue(listValue),
                                          m_selecting(false) {
}


GuiMenuRef GuiDropDownList::menu() { 
    if (m_menu.isNull()) {
        m_menu = GuiMenu::create(theme(), &m_listValue, m_indexValue);
    }

    return m_menu;
}


void GuiDropDownList::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    if (m_visible) {
        skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), 
                                 m_selecting, selectedValue(), m_caption, m_captionSize);
    }
}


void GuiDropDownList::showMenu() {
    // Show the menu
    Rect2D clickRect = theme()->dropDownListToClickBounds(rect(), m_captionSize);
    Vector2 clickOffset = clickRect.x0y0() - rect().x0y0();
    Vector2 menuOffset(10, clickRect.height() + 10);

    menu()->show(m_gui->manager(), window(), this, toGWindowCoords(clickOffset + menuOffset));
}


bool GuiDropDownList::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {

        if (! m_gui->manager()->contains(menu())) {
            showMenu();
        }
        return true;

    } else if (event.type == GEventType::KEY_DOWN) {
        switch (event.key.keysym.sym) {
        case GKey::DOWN:
            if (*m_indexValue < m_listValue.size() - 1) {
                *m_indexValue = *m_indexValue + 1;
                fireEvent(GEventType::GUI_ACTION);
            }
            return true;

        case GKey::UP:
            if (*m_indexValue > 0) {
                *m_indexValue = *m_indexValue - 1;
                fireEvent(GEventType::GUI_ACTION);
            }
            return true;
        default:;
        }
    }

   return false;
}


void GuiDropDownList::setRect(const Rect2D& rect) {
     m_rect = rect;
     m_clickRect = theme()->dropDownListToClickBounds(rect, m_captionSize);
}


const GuiCaption& GuiDropDownList::selectedValue() const {
    if (m_listValue.size() > 0) {
        return m_listValue[*m_indexValue];
    } else {
        const static GuiCaption empty;
        return empty;
    }
}


void GuiDropDownList::setList(const Array<GuiCaption>& c) {
    m_listValue = c;
    *m_indexValue = iClamp(*m_indexValue, 0, m_listValue.size() - 1);
}


void GuiDropDownList::setList(const Array<std::string>& c) {
    m_listValue.resize(c.size());
    for (int i = 0; i < c.size(); ++i) {
        m_listValue[i] = c[i];
    }
    *m_indexValue = iClamp(*m_indexValue, 0, m_listValue.size() - 1);
}


void GuiDropDownList::clear() {
    m_listValue.clear();
    *m_indexValue = 0;
}


void GuiDropDownList::append(const GuiCaption& c) {
    m_listValue.append(c);
}

}

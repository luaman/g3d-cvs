/**
 @file GuiDropDownList.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2007-06-02
 @edited  2009-04-17
 */
#include "G3D/platform.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {


GuiDropDownList::GuiDropDownList
(GuiContainer*               parent, 
 const GuiText&              caption, 
 const Pointer<int>&         indexValue, 
 const Array<GuiText>&       listValue) :
 GuiControl(parent, caption), 
    m_indexValue(indexValue.isNull() ? Pointer<int>(&m_myInt) : indexValue),
    m_myInt(0),
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
    (void)rd;
    if (m_visible) {
        skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), 
                                 m_selecting, selectedValue(), m_caption, m_captionSize);
    }
}


void GuiDropDownList::setSelectedValue(const std::string& s) {
    for (int i = 0; i < m_listValue.size(); ++i) {
        if (m_listValue[i].text() == s) {
            setSelectedIndex(i);
            return;
        }
    }
}


void GuiDropDownList::showMenu() {
    // Show the menu
    Rect2D clickRect = theme()->dropDownListToClickBounds(rect(), m_captionSize);
    Vector2 clickOffset = clickRect.x0y0() - rect().x0y0();
    Vector2 menuOffset(10, clickRect.height() + 10);

    menu()->show(m_gui->manager(), window(), this, toOSWindowCoords(clickOffset + menuOffset));
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
            *m_indexValue = selectedIndex();
            if (*m_indexValue < m_listValue.size() - 1) {
                *m_indexValue = *m_indexValue + 1;
                fireEvent(GEventType::GUI_ACTION);
            }
            return true;

        case GKey::UP:
            *m_indexValue = selectedIndex();
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


const GuiText& GuiDropDownList::selectedValue() const {
    if (m_listValue.size() > 0) {
        return m_listValue[selectedIndex()];
    } else {
        const static GuiText empty;
        return empty;
    }
}


void GuiDropDownList::setList(const Array<GuiText>& c) {
    m_listValue = c;
    *m_indexValue = iClamp(*m_indexValue, 0, m_listValue.size() - 1);
    m_menu = NULL;
}


void GuiDropDownList::setList(const Array<std::string>& c) {
    m_listValue.resize(c.size());
    for (int i = 0; i < c.size(); ++i) {
        m_listValue[i] = c[i];
    }
    *m_indexValue = selectedIndex();
    m_menu = NULL;
}


void GuiDropDownList::clear() {
    m_listValue.clear();
    *m_indexValue = 0;
    m_menu = NULL;
}


void GuiDropDownList::append(const GuiText& c) {
    m_listValue.append(c);
    m_menu = NULL;
}

}

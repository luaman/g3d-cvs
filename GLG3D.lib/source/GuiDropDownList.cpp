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
 Array<std::string>*         listValue) : GuiControl(parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_stringListValue(listValue),
                                          m_selecting(false),
                                          m_useStringList(true) {
}


GuiDropDownList::GuiDropDownList
(GuiContainer*               parent, 
 const GuiCaption&           caption, 
 const Pointer<int>&         indexValue, 
 Array<GuiCaption>*          listValue) : GuiControl(parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_captionListValue(listValue),
                                          m_selecting(false),
                                          m_useStringList(false) {
}


GuiMenuRef GuiDropDownList::menu() { 
    if (m_menu.isNull()) {
        if (m_useStringList) {
            m_menu = GuiMenu::create(theme(), m_stringListValue, m_indexValue);
        } else {
            m_menu = GuiMenu::create(theme(), m_captionListValue, m_indexValue);
        }

        // Make menu events appear to come from the drop down
        m_menu->m_eventSource = this;
    }

    return m_menu;
}


void GuiDropDownList::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    if (m_visible) {

        if (m_useStringList) {
            // If there are no elements in the list, display the empty string
            const std::string& str = (m_stringListValue->size() > 0) ? 
                    (*m_stringListValue)[iMax(0, iMin(m_stringListValue->size() - 1, *m_indexValue))] : 
                    "";

            skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), m_selecting, str, m_caption, m_captionSize);
        } else {
            // If there are no elements in the list, display the empty string
            const GuiCaption& str = (m_captionListValue->size() > 0) ? 
                    (*m_captionListValue)[iMax(0, iMin(m_captionListValue->size() - 1, *m_indexValue))] : 
                    "";

            skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), 
                                     m_selecting, str, m_caption, m_captionSize);
        }
    }
}


void GuiDropDownList::showMenu() {
    // Show the menu
    Rect2D clickRect = theme()->dropDownListToClickBounds(rect(), m_captionSize);
    Vector2 clickOffset = clickRect.x0y0() - rect().x0y0();
    Vector2 menuOffset(10, clickRect.height() + 10);

    menu()->show(m_gui->manager(), window(), toGWindowCoords(clickOffset + menuOffset));
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
            if (*m_indexValue < (m_useStringList ? m_stringListValue->size() : m_captionListValue->size()) - 1) {
                *m_indexValue = *m_indexValue + 1;
                fireActionEvent();
            }
            return true;

        case GKey::UP:
            if (*m_indexValue > 0) {
                *m_indexValue = *m_indexValue - 1;
                fireActionEvent();
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


}

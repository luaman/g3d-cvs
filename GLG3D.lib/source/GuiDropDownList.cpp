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


// TODO: Special menu style
GuiMenu::GuiMenu(const GuiThemeRef& skin, const Rect2D& rect) : 
    GuiWindow("", skin, rect, GuiWindow::NORMAL_FRAME_STYLE, NO_CLOSE) {
}

   
bool GuiMenu::onEvent(const GEvent& event) {
    // Hide on escape
    if (m_visible && 
        (event.type == GEventType::KEY_DOWN) && 
        (event.key.keysym.sym == GKey::ESCAPE)) {

        hide();
        return true;
    }

    // TODO: Enter == selection

    bool handled = GuiWindow::onEvent(event);

    if (! focused()) {
        hide();
    }

    return handled;
}


void GuiMenu::show(WidgetManager* manager, const Vector2& position) {
    manager->add(this);
    moveTo(position);
    setVisible(true);
    manager->setFocusedWidget(this);
}


void GuiMenu::hide() {
    setVisible(false);
    m_manager->setFocusedWidget(this);
    m_manager->remove(this);
}        

///////////////////////////////////////////////////

GuiDropDownList::GuiDropDownList
(GuiPane*                    parent, 
 const GuiCaption&           caption, 
 const Pointer<int>&         indexValue, 
 Array<std::string>*         listValue) : GuiControl(parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_stringListValue(listValue),
                                          m_selecting(false),
                                          m_useStringList(true) {
}


GuiDropDownList::GuiDropDownList
(GuiPane*                    parent, 
 const GuiCaption&           caption, 
 const Pointer<int>&         indexValue, 
 Array<GuiCaption>*          listValue) : GuiControl(parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_captionListValue(listValue),
                                          m_selecting(false),
                                          m_useStringList(false) {
}

void GuiDropDownList::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    if (m_visible) {

        if (m_useStringList) {
            // If there are no elements in the list, display the empty string
            const std::string& str = (m_stringListValue->size() > 0) ? 
                    (*m_stringListValue)[iMax(0, iMin(m_stringListValue->size() - 1, *m_indexValue))] : 
                    "";

            skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), m_selecting, str, m_caption);
        } else {
            // If there are no elements in the list, display the empty string
            const GuiCaption& str = (m_captionListValue->size() > 0) ? 
                    (*m_captionListValue)[iMax(0, iMin(m_captionListValue->size() - 1, *m_indexValue))] : 
                    "";

            skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), m_selecting, str, m_caption);
        }
    }
}


bool GuiDropDownList::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {

        // Show the menu
        // TODO:
//        m_menu->show(m_gui->manager(), rect().x0y0());
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
     m_clickRect = skin()->dropDownListToClickBounds(rect);
}


}

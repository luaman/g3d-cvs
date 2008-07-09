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


GuiMenuRef GuiMenu::create(const GuiThemeRef& skin, Array<std::string>* listPtr, const Pointer<int>& indexValue) {
    return new GuiMenu(skin, Rect2D::xywh(0, 0, 100, 200), listPtr, indexValue);
}

GuiMenuRef GuiMenu::create(const GuiThemeRef& skin, Array<GuiCaption>* listPtr, const Pointer<int>& indexValue) {
    return new GuiMenu(skin, Rect2D::xywh(0, 0, 100, 200), listPtr, indexValue);
}


GuiMenu::GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<std::string>* listPtr, const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_stringListValue(listPtr), 
    m_captionListValue(NULL), m_indexValue(indexValue), m_useStringList(true) {

    m_labelArray.resize(listPtr->size());
    for (int i = 0; i < listPtr->size(); ++i) {
        m_labelArray[i] = pane()->addLabel((*listPtr)[i]);
    }
    pane()->pack();
}


GuiMenu::GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<GuiCaption>* listPtr, const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_stringListValue(NULL), 
    m_captionListValue(listPtr), m_indexValue(indexValue), m_useStringList(false) {

    m_labelArray.resize(listPtr->size());
    for (int i = 0; i < listPtr->size(); ++i) {
        m_labelArray[i] = pane()->addLabel((*listPtr)[i]);
    }
    pane()->pack();
}


bool GuiMenu::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    // Hide on escape
    if ((event.type == GEventType::KEY_DOWN) && 
        (event.key.keysym.sym == GKey::ESCAPE)) {
        hide();
        return true;
    }

    // TODO: Enter == selection

    // TODO: Mouse
    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        // See what was clicked on
        Vector2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            click += pane()->clientRect().x0y0() - m_clientRect.x0y0();
            for (int i = 0; i < m_labelArray.size(); ++i) {
                if (m_labelArray[i]->rect().contains(click)) {
                    // Clicked on this element
                    *m_indexValue = i;
                    hide();
                    return true;
                }
            }
            return true;
        }

        // Clicked off the menu
        hide();
        return false;
    }

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


/*void GuiMenu::render(RenderDevice* rd) {
    GuiWindow::render(rd);
    
    m_skin->beginRendering(rd);
    {
        m_skin->pushClientRect(m_clientRect);
        {
            // Draw menu contents
            m_rootPane->render(rd, m_skin);
        }
        m_skin->popClientRect();
    }
    m_skin->endRendering();    
}
*/

///////////////////////////////////////////////////

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
            m_menu = GuiMenu::create(skin(), m_stringListValue, m_indexValue);
        } else {
            m_menu = GuiMenu::create(skin(), m_captionListValue, m_indexValue);
        }
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

            skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), m_selecting, str, m_caption, m_captionSize);
        }
    }
}


void GuiDropDownList::showMenu() {
    // Show the menu
    Rect2D clickRect = skin()->dropDownListToClickBounds(rect(), m_captionSize);
    Vector2 clickOffset = clickRect.x0y0() - rect().x0y0();
    Vector2 menuOffset(10, clickRect.height() + 10);

    menu()->show(m_gui->manager(), toGWindowCoords(clickOffset + menuOffset));
}


bool GuiDropDownList::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {

        if (m_gui->manager()->contains(menu())) {
            // If the menu was already open, close it
            m_menu->hide();
        } else {
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
     m_clickRect = skin()->dropDownListToClickBounds(rect, m_captionSize);
}


}

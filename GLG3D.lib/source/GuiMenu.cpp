/**
 @file GuiMenu.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2008-07-14
 @edited  2009-03-14
 */
#include "G3D/platform.h"
#include "GLG3D/GuiMenu.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiMenu::Ref GuiMenu::create(const GuiThemeRef& skin, Array<std::string>* listPtr, const Pointer<int>& indexValue) {
    return new GuiMenu(skin, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue);
}

GuiMenu::Ref GuiMenu::create(const GuiThemeRef& skin, Array<GuiText>* listPtr, const Pointer<int>& indexValue) {
    return new GuiMenu(skin, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue);
}


GuiMenu::GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<std::string>* listPtr, const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_eventSource(NULL), m_stringListValue(listPtr), 
    m_captionListValue(NULL), m_indexValue(indexValue), m_useStringList(true), m_superior(NULL) {

    pane()->setPosition(Vector2(0, 0));
    m_labelArray.resize(listPtr->size());
    for (int i = 0; i < listPtr->size(); ++i) {
        m_labelArray[i] = pane()->addLabel((*listPtr)[i]);
    }
    pane()->pack();
    m_highlightIndex = *m_indexValue;
}


GuiMenu::GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<GuiText>* listPtr, 
                 const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_stringListValue(NULL), 
    m_captionListValue(listPtr), m_indexValue(indexValue), m_useStringList(false), m_superior(NULL) {

    pane()->setHeight(0);
    m_labelArray.resize(listPtr->size());
    for (int i = 0; i < listPtr->size(); ++i) {
        m_labelArray[i] = pane()->addLabel((*listPtr)[i]);
    }
    pack();
    m_highlightIndex = *m_indexValue;
}


bool GuiMenu::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    // Hide on escape
    if ((event.type == GEventType::KEY_DOWN) && 
        (event.key.keysym.sym == GKey::ESCAPE)) {
        fireMyEvent(GEventType::GUI_CANCEL);
        hide();
        return true;
    }

    // TODO: up/down keys

    // TODO: Enter == selection

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        // See what was clicked on
        Vector2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            int i = labelIndexUnderMouse(click);
            if (i >= 0) {
                *m_indexValue = i;
                m_actionCallback.execute();
                fireMyEvent(GEventType::GUI_ACTION);
                hide();
            }
            return true;
        } 

        fireMyEvent(GEventType::GUI_CANCEL);

        // Clicked off the menu
        hide();
        return false;
    } else if (event.type == GEventType::MOUSE_MOTION) {
        // Change the highlight
        Vector2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            m_highlightIndex = labelIndexUnderMouse(click);
        }            
    }

    bool handled = GuiWindow::onEvent(event);

    if (! focused()) {
        hide();
    }

    return handled;
}


int GuiMenu::labelIndexUnderMouse(Vector2 click) const {
    click += pane()->clientRect().x0y0() - m_clientRect.x0y0();
    for (int i = 0; i < m_labelArray.size(); ++i) {
        if (m_labelArray[i]->rect().contains(click)) {
            return i;
        }
    }

    return -1;
}


void GuiMenu::fireMyEvent(GEventType type) {
    GEvent e;
    e.gui.type = type;
    e.gui.control = m_eventSource;
    Widget::fireEvent(e);
}


void GuiMenu::show(WidgetManager* manager, GuiWindow* superior, GuiControl* eventSource, const Vector2& position, bool modal, const GuiControl::Callback& actionCallback) {
    m_actionCallback = actionCallback;
    m_superior = superior;
    m_eventSource = eventSource;
    manager->add(this);

    // Clamp position to screen bounds
    OSWindow* osWindow = (superior != NULL) ? superior->window() : RenderDevice::lastRenderDeviceCreated->window();
    const Vector2 high(osWindow->width() - m_rect.width(), osWindow->height() - m_rect.height());
    Vector2 actualPos = position.min(high).max(Vector2(0,0));

    moveTo(actualPos);
    manager->setFocusedWidget(this);

    if (modal) {
        showModal(superior);
    } else {
        setVisible(true);
    }
}


void GuiMenu::hide() {
    setVisible(false);
    m_manager->remove(this);
    m_manager->setFocusedWidget(m_superior);
    m_superior = NULL;
}


void GuiMenu::render(RenderDevice* rd) const {
    if (m_morph.active) {
        GuiMenu* me = const_cast<GuiMenu*>(this);
        me->m_morph.update(me);
    }
    
    m_skin->beginRendering(rd);
    {
        m_skin->renderWindow(m_rect, focused(), false, false, false, m_text, GuiTheme::WindowStyle(m_style));
        m_skin->pushClientRect(m_clientRect);
            
            // Draw the hilight (the root pane is invisible, so it will not overwrite)
            int i = m_highlightIndex;
            if ((i >= 0) && (i < m_labelArray.size())) {
                const Rect2D& r = m_labelArray[i]->rect();
                m_skin->renderSelection(Rect2D::xywh(0, r.y0(), m_clientRect.width(), r.height()));
            }
            
            m_rootPane->render(rd, m_skin);
        m_skin->popClientRect();
    }
    m_skin->endRendering();
}

}

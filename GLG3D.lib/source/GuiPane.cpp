/**
 @file GuiPane.cpp
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2007-06-02
 @edited  2007-06-10
 */
#include "G3D/platform.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiCheckBox.h"

namespace G3D {

GuiPane::GuiPane(GuiWindow* gui, GuiPane* parent, const GuiCaption& text, const Rect2D& rect, Style style) 
    : GuiControl(gui, parent, text), m_style(style) {
    setRect(rect);
}


Vector2 GuiPane::contentsExtent() const {
    Vector2 p;
    for (int i = 0; i < controlArray.size(); ++i) {
        p = p.max(controlArray[i]->rect().x1y1());
    }

    for (int i = 0; i < paneArray.size(); ++i) {
        p = p.max(paneArray[i]->rect().x1y1());
    }

    for (int i = 0; i < labelArray.size(); ++i) {
        p = p.max(labelArray[i]->rect().x1y1());
    }

    return p;
}


Vector2 GuiPane::nextControlPos() const {
    return Vector2(0, contentsExtent().y);
}


void GuiPane::increaseBounds(const Vector2& extent) {
    if ((m_clientRect.width() < extent.x) || (m_clientRect.height() < extent.y)) {
        // Create the new client rect
        Rect2D newRect = Rect2D::xywh(Vector2(0,0), extent.max(m_clientRect.wh()));

        // Transform the client rect into an absolute rect
        if (m_style != NO_FRAME_STYLE) {
            newRect = m_gui->skin->clientToPaneBounds(newRect, GuiSkin::PaneStyle(m_style));
        }

        // The new window has the old position and the new width
        setRect(Rect2D::xywh(m_rect.x0y0(), newRect.wh()));

        if (m_parent != NULL) {
            m_parent->increaseBounds(m_rect.x1y1());
        } else {
            m_gui->increaseBounds(m_rect.x1y1());
        }
    }
}


void GuiPane::pack() {
    increaseBounds(contentsExtent());
}


void GuiPane::setRect(const Rect2D& rect) {
    m_rect = rect;
    
    if (m_style == NO_FRAME_STYLE) {
        m_clientRect = m_rect;
    } else {
        m_clientRect = m_gui->skin->paneToClientBounds(m_rect, GuiSkin::PaneStyle(m_style));
    }
}


GuiPane::~GuiPane() {
    controlArray.deleteAll();
    labelArray.deleteAll();
}


GuiDropDownList* GuiPane::addDropDownList(const GuiCaption& caption, int* indexValue, Array<std::string>* list) {
    return addControl(new GuiDropDownList(m_gui, this, caption, Pointer<int>(indexValue), list));
}


GuiCheckBox* GuiPane::addCheckBox(const GuiCaption& text, bool* value, GuiCheckBox::Style style) {
    return addControl(new GuiCheckBox(m_gui, this, text, Pointer<bool>(value), style));
}


GuiTextBox* GuiPane::addTextBox(const GuiCaption& caption, std::string* value, GuiTextBox::Update update) {
    
    return addControl(new GuiTextBox(m_gui, this, caption, Pointer<std::string>(value), update));
}


GuiRadioButton* GuiPane::addRadioButton(const GuiCaption& text, int myID, void* selection, GuiRadioButton::Style style) {

    return addControl(new GuiRadioButton(m_gui, this, text, myID, Pointer<int>(reinterpret_cast<int*>(selection)), style));
}


GuiButton* GuiPane::addButton(const GuiCaption& text, GuiButton::Style style) {
    GuiButton* b = new GuiButton(m_gui, this, text, style);
    b->setRect(Rect2D::xywh(nextControlPos(), Vector2(80, CONTROL_HEIGHT)));
    
    controlArray.append(b);

    return b;
}


GuiLabel* GuiPane::addLabel(const GuiCaption& text, GFont::XAlign x, GFont::YAlign y) {
    GuiLabel* b = new GuiLabel(m_gui, this, text, x, y);
    b->setRect(Rect2D::xywh(nextControlPos(), Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), CONTROL_HEIGHT)));
    
    labelArray.append(b);

    return b;
}


GuiPane* GuiPane::addPane(const GuiCaption& text, float h, GuiPane::Style style) {
    h = max(h, 0.0f);
    Rect2D minRect = m_gui->skin->clientToPaneBounds(Rect2D::xywh(0,0,0,0), GuiSkin::PaneStyle(style));

    Vector2 pos = nextControlPos();

    // Back up by the border size
    pos -= minRect.x0y0();

    Rect2D newRect = Rect2D::xywh(pos, Vector2(m_clientRect.width() - pos.x * 2, h + minRect.height()));

    GuiPane* p = new GuiPane(m_gui, this, text, newRect, style);

    paneArray.append(p);
    increaseBounds(p->rect().x1y1());

    return p;
}


void GuiPane::findControlUnderMouse(Vector2 mouse, GuiControl*& control) const {
    if (! m_clientRect.contains(mouse) || ! m_visible) {
        return;
    }

    mouse -= m_clientRect.x0y0();
    for (int i = 0; i < paneArray.size(); ++i) {
        paneArray[i]->findControlUnderMouse(mouse, control);
        if (control != NULL) {
            return;
        }
    }

    for (int i = 0; i < controlArray.size(); ++i) {
        if (controlArray[i]->m_clickRect.contains(mouse) && controlArray[i]->visible() && controlArray[i]->enabled()) {
            control = controlArray[i];
            break;
        }
    }
}


void GuiPane::render(RenderDevice* rd, const GuiSkinRef& skin) const {

    if (m_style != NO_FRAME_STYLE) {
        skin->renderPane(m_rect, GuiSkin::PaneStyle(m_style));
    }

    skin->pushClientRect(m_clientRect);

    for (int p = 0; p < paneArray.size(); ++p) {
        paneArray[p]->render(rd, skin);
    }

    for (int c = 0; c < controlArray.size(); ++c) {
        controlArray[c]->render(rd, skin);
    }

    for (int L = 0; L < labelArray.size(); ++L) {
        labelArray[L]->render(rd, skin);
    }

    skin->popClientRect();
}


void GuiPane::remove(GuiControl* control) {
    
    int i = labelArray.findIndex(reinterpret_cast<GuiLabel* const>(control));
    int j = controlArray.findIndex(control);
    int k = paneArray.findIndex(reinterpret_cast<GuiPane* const>(control));

    if (i != -1) {

        labelArray.fastRemove(i);
        delete control;

    } else if (j != -1) {

        controlArray.fastRemove(j);

        if (m_gui->keyFocusGuiControl == control) {
            m_gui->keyFocusGuiControl = NULL;
        }

        if (m_gui->mouseOverGuiControl == control) {
            m_gui->mouseOverGuiControl = NULL;
        }

        delete control;

    } else if (k != -1) {

        paneArray.fastRemove(k);
        delete control;

    }
}

}

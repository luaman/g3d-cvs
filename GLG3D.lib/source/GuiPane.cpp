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
    : GuiControl(gui, parent, text), m_style(style), nextGuiControlPos(0,0) {
    setRect(rect);
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


GuiCheckBox* GuiPane::addCheckBox(const GuiCaption& text, bool* value, GuiCheckBox::Style style) {
    GuiCheckBox* c = new GuiCheckBox(m_gui, this, text, value, style);
    c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += c->rect().height();
    
    controlArray.append(c);

    return c;
}


GuiTextBox* GuiPane::addTextBox(const GuiCaption& caption, std::string* value, GuiTextBox::Update update) {
    GuiTextBox* c = new GuiTextBox(m_gui, this, caption, value, update, TEXT_CAPTION_WIDTH);
    c->setRect(Rect2D::xywh(nextGuiControlPos + Vector2(TEXT_CAPTION_WIDTH, 0), Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += c->rect().height();
    
    controlArray.append(c);

    return c;
}


GuiRadioButton* GuiPane::addRadioButton(const GuiCaption& text, int myID, void* selection, GuiRadioButton::Style style) {
    GuiRadioButton* c = new GuiRadioButton(m_gui, this, text, myID, reinterpret_cast<int*>(selection), style);
    c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += c->rect().height();
    
    controlArray.append(c);
    
    return c;
}


GuiButton* GuiPane::addButton(const GuiCaption& text) {
    GuiButton* b = new GuiButton(m_gui, this, text);
    b->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(80, 30)));
    nextGuiControlPos.y += b->rect().height();
    
    controlArray.append(b);

    return b;
}


GuiLabel* GuiPane::addLabel(const GuiCaption& text, GFont::XAlign x, GFont::YAlign y) {
    GuiLabel* b = new GuiLabel(m_gui, this, text, x, y);
    b->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += b->rect().height();
    
    labelArray.append(b);

    return b;
}


GuiPane* GuiPane::addPane(const GuiCaption& text, float h, GuiPane::Style style) {
    GuiPane* p = new GuiPane(m_gui, this, text, Rect2D::xywh(nextGuiControlPos, Vector2(m_clientRect.width() - 4 - nextGuiControlPos.x * 2, h)), style);
    nextGuiControlPos.y += p->rect().height();

    paneArray.append(p);

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
        if (controlArray[i]->m_rect.contains(mouse) && controlArray[i]->visible() && controlArray[i]->enabled()) {
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

#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiCheckBox.h"

namespace G3D {

GuiPane::GuiPane(GuiWindow* gui, GuiPane* parent, const GuiText& text, const Rect2D& rect, Style style) 
    : GuiControl(gui, parent, text), m_style(style), nextGuiControlPos(0,0) {
    setRect(rect);
}


void GuiPane::setRect(const Rect2D& rect) {
    m_rect = rect;
    
    switch (m_style) {
    case NO_FRAME_STYLE:
        m_clientRect = m_rect;
        break;
        
    case SIMPLE_FRAME_STYLE:
        m_clientRect = m_gui->skin->simplePaneToClientBounds(m_rect);
        break;
        
    case ORNATE_FRAME_STYLE:
        m_clientRect = m_gui->skin->ornatePaneToClientBounds(m_rect);
        break;
    }
}


GuiPane::~GuiPane() {
    controlArray.deleteAll();
    labelArray.deleteAll();
}


GuiCheckBox* GuiPane::addCheckBox(const GuiText& text, bool* value, GuiCheckBox::Style style) {
    GuiCheckBox* c = new GuiCheckBox(m_gui, this, text, value, style);
    c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += c->rect().height();
    
    controlArray.append(c);

    return c;
}


GuiRadioButton* GuiPane::addRadioButton(const GuiText& text, int myID, void* selection, GuiRadioButton::Style style) {
    GuiRadioButton* c = new GuiRadioButton(m_gui, this, text, myID, reinterpret_cast<int*>(selection), style);
    c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += c->rect().height();
    
    controlArray.append(c);
    
    return c;
}


GuiButton* GuiPane::addButton(const GuiText& text) {
    GuiButton* b = new GuiButton(m_gui, this, text);
    b->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(80, 30)));
    nextGuiControlPos.y += b->rect().height();
    
    controlArray.append(b);

    return b;
}


GuiLabel* GuiPane::addLabel(const GuiText& text, GFont::XAlign x, GFont::YAlign y) {
    GuiLabel* b = new GuiLabel(m_gui, this, text, x, y);
    b->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 30)));
    nextGuiControlPos.y += b->rect().height();
    
    labelArray.append(b);

    return b;
}


GuiPane* GuiPane::addPane(const GuiText& text, float h, GuiPane::Style style) {
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

    switch (m_style) {
    case SIMPLE_FRAME_STYLE:
        skin->renderSimplePane(m_rect);
        break;
    
    case ORNATE_FRAME_STYLE:
        skin->renderOrnatePane(m_rect);
        break;

    default:;
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
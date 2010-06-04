/**
 @file GuiPane.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2007-06-02
 @edited  2010-03-11
 */
#include "G3D/platform.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiTabPane.h"

namespace G3D {

/** Pixels of padding between controls */
static const float CONTROL_PADDING = 4.0f;

void GuiPane::init(const Rect2D& rect) {
    setRect(rect);
}

GuiPane::GuiPane(GuiWindow* gui, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style) 
    : GuiContainer(gui, text), m_style(style) {
    init(rect);
}


GuiPane::GuiPane(GuiContainer* parent, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style) 
    : GuiContainer(parent, text), m_style(style) {
    init(rect);
}


void GuiPane::morphTo(const Rect2D& r) {
    m_morph.morphTo(rect(), r);
}


void GuiPane::setCaption(const GuiText& caption) {
    GuiControl::setCaption(caption);
    setRect(rect());
}


Vector2 GuiPane::contentsExtent() const {
    Vector2 p(0.0f, 0.0f);
    for (int i = 0; i < controlArray.size(); ++i) {
        p = p.max(controlArray[i]->rect().x1y1());
    }

    for (int i = 0; i < containerArray.size(); ++i) {
        p = p.max(containerArray[i]->rect().x1y1());
    }

    for (int i = 0; i < labelArray.size(); ++i) {
        p = p.max(labelArray[i]->rect().x1y1());
    }
    return p;
}


Vector2 GuiPane::nextControlPos(bool isTool) const {
    if (isTool && (controlArray.size() > 0) && controlArray.last()->toolStyle() ) {
        // Place next to previous tool
        return controlArray.last()->rect().x1y0();
    } else {
        float y = contentsExtent().y;
        return Vector2(CONTROL_PADDING, max(y, CONTROL_PADDING));
    }
}


void GuiPane::pack() {
    // Resize to minimum bounds
    setSize(m_rect.wh() - m_clientRect.wh());
    for (int i = 0; i < containerArray.size(); ++i) {
        GuiPane* p = dynamic_cast<GuiPane*>(containerArray[i]);
        if (p != NULL) {
            p->pack();
        }
    }
    increaseBounds(contentsExtent());
}


void GuiPane::setRect(const Rect2D& rect) {
    m_rect = rect;       
    m_clientRect = theme()->paneToClientBounds(m_rect, m_caption, GuiTheme::PaneStyle(m_style));
}


GuiPane::~GuiPane() {
    controlArray.deleteAll();
    labelArray.deleteAll();
}


GuiTextureBox* GuiPane::addTextureBox
(const GuiText& caption,
 const Texture::Ref& t,
 const GuiTextureBox::Settings& s,
 bool embedded) {
    return addControl(new GuiTextureBox(this, caption, t, s, embedded), 240);
}


GuiTextureBox* GuiPane::addTextureBox
(const Texture::Ref& t,
 const GuiTextureBox::Settings& s,
 bool embedded) {
     return addTextureBox(t->name(), t, s, embedded);
}


GuiDropDownList* GuiPane::addDropDownList
(const GuiText& caption, 
 const Array<std::string>& list,
 const Pointer<int>& pointer,
 const GuiControl::Callback& actionCallback) {

    Array<GuiText> c;
    c.resize(list.size());
    for (int i = 0; i < c.size(); ++i) {
        c[i] = list[i];
    }
    return addDropDownList(caption, c, pointer, actionCallback);
}


GuiDropDownList* GuiPane::addDropDownList
(const GuiText& caption, 
 const Array<GuiText>& list,
 const Pointer<int>& pointer,
 const GuiControl::Callback& actionCallback) {
    return addControl(new GuiDropDownList(this, caption, pointer, list, actionCallback));
}


GuiRadioButton* GuiPane::addRadioButton(const GuiText& text, int myID, void* selection, 
                                        GuiTheme::RadioButtonStyle style) {

    GuiRadioButton* c = addControl
        (new GuiRadioButton(this, text, myID, Pointer<int>(reinterpret_cast<int*>(selection)), style));

    Vector2 size(0, (float)CONTROL_HEIGHT);

    if (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) {
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::TOOL_BUTTON_STYLE);
        size.x = max(float(TOOL_BUTTON_WIDTH), bounds.x);
    } else if (style == 1) { // Doesn't compile on gcc for some reason: GuiTheme::BUTTON_RADIO_BUTTON_STYLE) {
        size.x = BUTTON_WIDTH;
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::NORMAL_BUTTON_STYLE);
        size = size.max(bounds);
    } else {
        size.x = 30.0f;
    }

    c->setSize(size);

    return c;
}


GuiCheckBox* GuiPane::addCheckBox
(const GuiText& text,
 const Pointer<bool>& pointer,
 GuiTheme::CheckBoxStyle style) {
    GuiCheckBox* c = addControl(new GuiCheckBox(this, text, pointer, style));
    
    Vector2 size(0, CONTROL_HEIGHT);

    if (style == GuiTheme::TOOL_CHECK_BOX_STYLE) {
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::TOOL_BUTTON_STYLE);
        size.x = max(float(TOOL_BUTTON_WIDTH), bounds.x);
    } else {
        size.x = BUTTON_WIDTH;
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::NORMAL_BUTTON_STYLE);
        size = size.max(bounds);
    }

    c->setSize(size);

    return c;
}


GuiControl* GuiPane::addCustom(GuiControl* c) {
    c->setPosition(nextControlPos(c->toolStyle()));

    GuiContainer* container = dynamic_cast<GuiContainer*>(c);
    if (container) {
        containerArray.append(container);
    } else {
        controlArray.append(c);
    }

    increaseBounds(c->rect().x1y1());
    return c;
}


GuiRadioButton* GuiPane::addRadioButton(const GuiText& text, int myID,  
    const Pointer<int>& ptr, 
    GuiTheme::RadioButtonStyle style) {
    
    GuiRadioButton* c = addControl(new GuiRadioButton(this, text, myID, ptr, style));
    
    Vector2 size((float)BUTTON_WIDTH, (float)CONTROL_HEIGHT);

    // Ensure that the button is wide enough for the caption
    const Vector2& bounds = theme()->minButtonSize(text, 
        (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) ? GuiTheme::TOOL_BUTTON_STYLE : GuiTheme::NORMAL_BUTTON_STYLE);

    if (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) {
        c->setSize(Vector2(max((float)TOOL_BUTTON_WIDTH, bounds.x), CONTROL_HEIGHT));
    } else if (style == GuiTheme::BUTTON_RADIO_BUTTON_STYLE) {
        c->setSize(Vector2(max((float)BUTTON_WIDTH, bounds.x), CONTROL_HEIGHT));
    }

    return c;
}


GuiButton* GuiPane::addButton(const GuiText& text, GuiTheme::ButtonStyle style) {
    return addButton(text, GuiButton::Callback(), style);
}


GuiButton* GuiPane::addButton(const GuiText& text, const GuiControl::Callback& callback, GuiTheme::ButtonStyle style) {
    GuiButton* b = new GuiButton(this, callback, text, style);

    addControl(b);

    Vector2 size((float)BUTTON_WIDTH, (float)CONTROL_HEIGHT);

    // Ensure that the button is wide enough for the caption
    const Vector2& bounds = theme()->minButtonSize(text, style);
    if (style == GuiTheme::NORMAL_BUTTON_STYLE) {
        size = size.max(bounds);
    } else {
        size.x = max((float) TOOL_BUTTON_WIDTH, bounds.x);
    }

    b->setSize(size);
    
    return b;
}


GuiLabel* GuiPane::addLabel(const GuiText& text, GFont::XAlign xalign, GFont::YAlign yalign) {
    GuiLabel* b = new GuiLabel(this, text, xalign, yalign);

    const Vector2& bounds = theme()->bounds(text);
    b->setRect(Rect2D::xywh(nextControlPos(), 
                            bounds.max(Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 
                                               CONTROL_HEIGHT))));
    
    labelArray.append(b);

    return b;
}


GuiFunctionBox* GuiPane::addFunctionBox(const GuiText& text, Spline<float>* spline) {

    GuiFunctionBox* control = new GuiFunctionBox(this, text, spline);

    Vector2 p = nextControlPos(control->toolStyle());
    control->setRect
        (Rect2D::xywh(p, Vector2((float)CONTROL_WIDTH, control->rect().height())));
    
    increaseBounds(control->rect().x1y1());
    
    GuiContainer* container = dynamic_cast<GuiContainer*>(control);
    if (container == NULL) {
        controlArray.append(control);
    } else {
        containerArray.append(container);
    }

    return control;
}


GuiTabPane* GuiPane::addTabPane(const Pointer<int>& index) {
    GuiTabPane* p = new GuiTabPane(this, index);
    
    Vector2 pos = nextControlPos();
    p->moveBy(pos);

    containerArray.append(p);
    increaseBounds(p->rect().x1y1());
    return p;
}


GuiPane* GuiPane::addPane(const GuiText& text, GuiTheme::PaneStyle style) {
    Rect2D minRect = theme()->clientToPaneBounds(Rect2D::xywh(0,0,0,0), text, style);

    Vector2 pos = nextControlPos();

    // Back up by the border size
    pos -= minRect.x0y0();

    // Ensure the width isn't negative due to a very small m_clientRect
    // which would push the position off the parent panel
    float newRectWidth = max(m_clientRect.width() - pos.x * 2, 0.0f);

    Rect2D newRect = Rect2D::xywh(pos, Vector2(newRectWidth, minRect.height()));

    GuiPane* p = new GuiPane(this, text, newRect, style);

    containerArray.append(p);
    increaseBounds(p->rect().x1y1());

    return p;
}


void GuiPane::findControlUnderMouse(Vector2 mouse, GuiControl*& control) const {
    if (! m_clientRect.contains(mouse) || ! m_visible) {
        return;
    }

    mouse -= m_clientRect.x0y0();

    // Test in the opposite order of rendering so that the top-most control receives the event
    for (int i = controlArray.size() - 1; i >= 0; --i) {
        if (controlArray[i]->m_clickRect.contains(mouse) && controlArray[i]->visible() && controlArray[i]->enabled()) {
            control = controlArray[i];
            break;
        }
    }

    for (int i = containerArray.size() - 1; i >= 0; --i) {
        containerArray[i]->findControlUnderMouse(mouse, control);
        if (control != NULL) {
            return;
        }
    }
}


void GuiPane::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    if (m_morph.active) {
        GuiPane* me = const_cast<GuiPane*>(this);
        me->m_morph.update(me);
    }

    if (! m_visible) {
        return;
    }

    skin->renderPane(m_rect, m_caption, m_style);

    renderChildren(rd, skin);
}


void GuiPane::renderChildren(RenderDevice* rd, const GuiThemeRef& skin) const {
    skin->pushClientRect(m_clientRect);

    for (int L = 0; L < labelArray.size(); ++L) {
        labelArray[L]->render(rd, skin);
    }

    for (int c = 0; c < controlArray.size(); ++c) {
        controlArray[c]->render(rd, skin);
    }

    for (int p = 0; p < containerArray.size(); ++p) {
        containerArray[p]->render(rd, skin);
    }

    skin->popClientRect();
}


void GuiPane::remove(GuiControl* control) {
    
    int i = labelArray.findIndex(reinterpret_cast<GuiLabel* const>(control));
    int j = controlArray.findIndex(control);
    int k = containerArray.findIndex(reinterpret_cast<GuiPane* const>(control));

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

        containerArray.fastRemove(k);
        delete control;

    }
}

}

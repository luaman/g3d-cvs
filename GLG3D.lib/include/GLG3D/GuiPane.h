/**
 @file GLG3D/GuiPane.h

 @created 2006-05-01
 @edited  2007-06-11

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIPANE_H
#define G3D_GUIPANE_H

#include <string>
#include "G3D/Pointer.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiSkin.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiTextBox.h"

namespace G3D {

class GuiButton;

/**
 Sub-rectangle of a window.  Created by GuiWindow::addPane().
 If a pane is invisible, everything inside of it is also invisible.

 All coordinates of objects inside a pane are relative to the pane's
 clientRect().  See GuiWindow for an example of creating a user interface.
 */
class GuiPane : public GuiControl {
    friend class GuiWindow;
    friend class GuiControl;
    friend class GuiButton;
    friend class GuiRadioButton;
    friend class _GuiSliderBase;

private:
    enum {CONTROL_HEIGHT = 25};
    enum {CONTROL_WIDTH = 180};
    enum {TEXT_CAPTION_WIDTH = 80};
public:

    /** Controls the appearance of the pane's borders and background.
     */
    // These constants must match the GuiSkin::PaneStyle constants
    enum Style {SIMPLE_FRAME_STYLE, ORNATE_FRAME_STYLE, NO_FRAME_STYLE};
    
protected:

    /** If this is a mouse event, make it relative to the client rectangle */
    static void makeRelative(GEvent& e, const Rect2D& clientRect);

    Style               m_style;

    /** Position to place the next control at. */
    Vector2             nextGuiControlPos;

    Array<GuiControl*>  controlArray;
    /** Sub panes */
    Array<GuiPane*>     paneArray;
    Array<GuiLabel*>    labelArray;

    Rect2D              m_clientRect;

    GuiPane(GuiWindow* gui, GuiPane* parent, const GuiCaption& text, const Rect2D& rect, Style style);

    virtual void render(RenderDevice* rd, const GuiSkinRef& skin) const;
    
    /**
    Finds the visible, enabled control underneath the mouse
    @param control (Output) pointer to the control that the mouse is over
    @param mouse Relative to the parent of this pane.
    */
    void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;

    virtual bool onEvent(const GEvent& event) { return false; }

public:

    /** Client rect bounds, relative to the parent pane (or window if
        there is no parent). */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }

    /** Set relative to the parent pane (or window) */
    virtual void setRect(const Rect2D& rect);

    ~GuiPane();

    GuiPane* addPane(const GuiCaption& text, float height, GuiPane::Style style = GuiPane::NO_FRAME_STYLE);

    /**
       <pre>
       Foo* foo = new Foo();
       gui->addCheckBox("Enabled", foo, &Foo::enabled, &Foo::setEnabled);

       BarRef foo = Bar::create();
       gui->addCheckBox("Enabled", bar.pointer(), &Bar::enabled, &Bar::setEnabled);
       </pre>
    */
    template<class T>
    GuiCheckBox* addCheckBox
    (const GuiCaption& text,
     T* object,
     bool (T::*get)() const,
     void (T::*set)(bool),
     GuiCheckBox::Style style = GuiCheckBox::BOX_STYLE
     ) {
        
        GuiCheckBox* c = new GuiCheckBox(m_gui, this, text, object, get, set, style);
        c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), CONTROL_HEIGHT)));
        nextGuiControlPos.y += c->rect().height();

        controlArray.append(c);

        return c;
    }

    template<class T>
    GuiTextBox* addTextBox
    (const GuiCaption& caption,
     T* object,
     std::string (T::*get)() const,
     void (T::*set)(std::string),
     GuiTextBox::Update update = GuiTextBox::DELAYED_UPDATE
     ) {
        
        GuiTextBox* c = new GuiTextBox(m_gui, this, caption, object, get, set, update, TEXT_CAPTION_WIDTH);
        c->setRect(Rect2D::xywh(nextGuiControlPos + Vector2(TEXT_CAPTION_WIDTH, 0), Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), CONTROL_HEIGHT)));
        nextGuiControlPos.y += c->rect().height();
        controlArray.append(c);

        return c;
    }

    /**
       Example:
       <pre>
       gui->addCheckBox("Enabled", &enabled);
       </pre>
     */
    GuiCheckBox* addCheckBox(const GuiCaption& text, bool* value, GuiCheckBox::Style = GuiCheckBox::BOX_STYLE);

    template<typename Value, class T>
    GuiSlider<Value>* addSlider
    (const GuiCaption& text,
     T* object,
     Value (T::*get)() const,
     void (T::*set)(Value),
     Value min,
     Value max,
     bool horizontal = true) {
        
        GuiSlider<Value>* c = new GuiSlider<Value>(m_gui, this, text, object, get, set, min, max, horizontal);
        c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(min(m_clientRect.width(), CONTROL_WIDTH), CONTROL_HEIGHT)));
        nextGuiControlPos.y += c->rect().height();

        controlArray.append(c);

        return c;
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiCaption& text, Value* value, Value min, Value max, bool horizontal = true) {
        
        GuiSlider<Value>* c = new GuiSlider<Value>(m_gui, this, text, value, min,  max, horizontal);
        c->setRect(Rect2D::xywh(nextGuiControlPos, Vector2(G3D::min(m_clientRect.width(), (float)CONTROL_WIDTH), CONTROL_HEIGHT)));
        nextGuiControlPos.y += c->rect().height();

        controlArray.append(c);

        return c;
    }

    /**
       Example:
       <pre>
       enum Day {SUN, MON, TUE, WED, THU, FRI, SAT};

       Day day;
       
       gui->addRadioButton("Sun", SUN, &day);
       gui->addRadioButton("Mon", MON, &day);
       gui->addRadioButton("Tue", TUE, &day);
       ...
       </pre>
       @param selection Must be a pointer to an int or enum.  The current selection value for a group of radio buttons.
     */
    GuiRadioButton* addRadioButton(const GuiCaption& text, int myID, void* selection,
                                GuiRadioButton::Style style = GuiRadioButton::RADIO_STYLE);

    GuiButton* addButton(const GuiCaption& text);

    GuiTextBox* addTextBox(const GuiCaption& caption, std::string* value, GuiTextBox::Update update = GuiTextBox::DELAYED_UPDATE);

    GuiLabel* addLabel(const GuiCaption& text, GFont::XAlign xalign = GFont::XALIGN_LEFT, 
                    GFont::YAlign = GFont::YALIGN_CENTER);

    /**
     Removes this control from the GuiPane.
     */
    void remove(GuiControl* gui);
};

}

#endif

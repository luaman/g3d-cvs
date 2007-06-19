/**
 @file GLG3D/GuiPane.h

 @created 2006-05-01
 @edited  2007-06-15

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
#include "GLG3D/GuiDropDownList.h"

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

    /** Updates this pane to ensure that its client rect is least as wide and
        high as the specified extent, then recursively calls
        increaseBounds on its parent.
     */
    void increaseBounds(const Vector2& extent);

    template<class T>
    T* addControl(T* c) {
        c->setRect(Rect2D::xywh(nextGuiControlPos, 
                                Vector2(max(m_clientRect.width(), (float)CONTROL_WIDTH), CONTROL_HEIGHT)));
        nextGuiControlPos.y += c->rect().height();

        increaseBounds(c->rect().x1y1());

        controlArray.append(c);
        return c;
    }

public:

    /** Client rect bounds, relative to the parent pane (or window if
        there is no parent). */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }

    /** Set relative to the parent pane (or window) */
    virtual void setRect(const Rect2D& rect);

    ~GuiPane();

    /** 
        @param height Non-client size of the pane.  This will automatically grow as controls are added if left at zero.
     */
    GuiPane* addPane(const GuiCaption& text, float height = 0, GuiPane::Style style = GuiPane::NO_FRAME_STYLE);

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
        return addControl(new GuiCheckBox(m_gui, this, text, object, get, set, style));
    }

    template<class T>
    GuiTextBox* addTextBox
    (const GuiCaption& caption,
     T* object,
     std::string (T::*get)() const,
     void (T::*set)(std::string),
     GuiTextBox::Update update = GuiTextBox::DELAYED_UPDATE
     ) {
        
        return addControl(new GuiTextBox(m_gui, this, caption, Pointer<std::string>(object, get, set), update));
    }

    template<class IndexObj>
    GuiDropDownList* addDropDownList
    (const GuiCaption& caption,
     IndexObj* indexObject,
     int (IndexObj::*indexGet)() const,
     void (IndexObj::*indexSet)(int),
     Array<std::string>* list) {
        
        return addControl(new GuiDropDownList
            (m_gui, 
            this, 
            caption, 
            Pointer<int>(indexObject, indexGet, indexSet),
             list));
    }

    GuiDropDownList* addDropDownList(const GuiCaption& caption, int* indexValue, Array<std::string>* list);
    
    template<typename EnumOrInt, class T>
    GuiRadioButton* addRadioButton(const GuiCaption& text, int myID,  
        T* object,
        EnumOrInt (T::*get)() const,
        void (T::*set)(EnumOrInt), 
        GuiRadioButton::Style style) {
        
        return addControl(new GuiRadioButton
                          (m_gui, this, text, myID, 
                           Pointer<int>(object, 
                                        reinterpret_cast<int (T::*)() const>(get), 
                                        reinterpret_cast<void (T::*)(EnumOrInt)>(set)), style));
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
        
        return addControl(new GuiSlider<Value>(m_gui, this, text, Pointer<Value>(object, get, set), 
                                               min, max, horizontal));
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiCaption& text, Value* value, Value min, Value max, bool horizontal = true) {
        
        return addControl(new GuiSlider<Value>(m_gui, this, text, value, min,  max, horizontal));
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

       @param selection Must be a pointer to an int or enum.  The
       current selection value for a group of radio buttons.
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

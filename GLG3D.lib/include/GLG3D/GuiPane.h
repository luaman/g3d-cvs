/**
 @file GLG3D/GuiPane.h

 @created 2006-05-01
 @edited  2007-10-15

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIPANE_H
#define G3D_GUIPANE_H

#include <string>
#include <limits.h>
#include "G3D/Pointer.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiNumberBox.h"
#include "GLG3D/GuiDropDownList.h"

namespace G3D {

class GuiButton;

/**
 Sub-rectangle of a window.  Created by GuiWindow::addPane().
 If a pane is invisible, everything inside of it is also invisible.

 All coordinates of objects inside a pane are relative to the pane's
 clientRect().  See GuiWindow for an example of creating a user interface.
 */
class GuiPane : public GuiContainer {
    friend class GuiWindow;
    friend class GuiControl;
    friend class GuiButton;
    friend class GuiRadioButton;
    friend class _GuiSliderBase;
protected:

    /** Caption label */
    GuiLabel*           m_label;

    _internal::Morph    m_morph;

    GuiTheme::PaneStyle m_style;

    Array<GuiControl*>  controlArray;

    /** Sub panes */
    Array<GuiContainer*> containerArray;

    Array<GuiLabel*>    labelArray;

    GuiPane(GuiWindow* gui, const GuiCaption& text, const Rect2D& rect, GuiTheme::PaneStyle style);
    GuiPane(GuiContainer* parent, const GuiCaption& text, const Rect2D& rect, GuiTheme::PaneStyle style);

    /**
       Called from constructors.
     */
    void init(const Rect2D& rect);

    virtual bool onEvent(const GEvent& event) { return false; }

    /** Updates this pane to ensure that its client rect is least as wide and
        high as the specified extent, then recursively calls
        increaseBounds on its parent.
     */
    void increaseBounds(const Vector2& extent);

    /** Finds the next vertical position for a control relative to the client rect. */
    Vector2 nextControlPos(bool isTool = false) const;

    template<class T>
    T* addControl(T* control) {
        Vector2 p = nextControlPos(control->toolStyle());
        control->setRect(Rect2D::xywh(p, Vector2(max(m_clientRect.width() - p.x, 
                                               (float)CONTROL_WIDTH), CONTROL_HEIGHT)));

        increaseBounds(control->rect().x1y1());

        GuiContainer* container = dynamic_cast<GuiContainer*>(control);
        if (container == NULL) {
            controlArray.append(control);
        } else {
            containerArray.append(container);
        }

        return control;
    }

    Vector2 contentsExtent() const;

    /** Called from render() */
    void renderChildren(RenderDevice* rd, const GuiThemeRef& skin) const;

public:

    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;

    /** If the original caption was non-empty (even if it was " "), 
        the new caption will be shown.*/
    virtual void setCaption(const GuiCaption& caption);    

    /** Set relative to the parent pane (or window) */
    virtual void setRect(const Rect2D& rect);

    /**
       Causes the window to change shape and/or position to meet the
       specified location.  The window will not respond to drag events
       while it is morphing.
     */
    virtual void morphTo(const Rect2D& r);

    /** Returns true while a morph is in progress. */
    bool morphing() const {
        return m_morph.active;
    }

    ~GuiPane();

    /**
       Add a custom ("user-created") subclass of GuiControl.  @a control
       should not be a subclass of GuiPane.  Do not add a standard
       (e.g., G3D::GuiButton, G3D::GuiPane) control using this method.
     */
    void addCustom(GuiControl* control);

    /** 
        If the text is "", no space is reserved for a caption.  If non-empty (even " "), then
        space is reserved and the caption may later be changed.
     */
    GuiPane* addPane(const GuiCaption& text = "", GuiTheme::PaneStyle style = GuiTheme::SIMPLE_PANE_STYLE);

    /**
       <pre>
       bool enabled;
       gui->addCheckBox("Enabled", &enabled);

       Foo* foo = new Foo();
       gui->addCheckBox("Enabled", Pointer<bool>(foo, &Foo::enabled, &Foo::setEnabled));

       BarRef foo = Bar::create();
       gui->addCheckBox("Enabled", Pointer<bool>(bar, &Bar::enabled, &Bar::setEnabled));
       </pre>
    */
    GuiCheckBox* addCheckBox
    (const GuiCaption& text,
     const Pointer<bool>& pointer,
     GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE);

    GuiCheckBox* addCheckBox
    (const GuiCaption& text,
     bool* pointer,
     GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE
     ) {
        return addCheckBox(text, Pointer<bool>(pointer), style);
    }

    GuiTextBox* addTextBox
    (const GuiCaption& caption,
     const Pointer<std::string>& stringPointer,
     GuiTextBox::Update update = GuiTextBox::DELAYED_UPDATE
     ) {        
        return addControl(new GuiTextBox(this, caption, stringPointer, update));
    }

    GuiDropDownList* addDropDownList(const GuiCaption& caption, const Pointer<int>& indexPointer, Array<std::string>* list);
    GuiDropDownList* addDropDownList(const GuiCaption& caption, const Pointer<int>& indexPointer, Array<GuiCaption>* list);
    
    template<typename EnumOrInt, class T>
    GuiRadioButton* addRadioButton(const GuiCaption& text, int myID,  
        T* object,
        EnumOrInt (T::*get)() const,
        void (T::*set)(EnumOrInt), 
        GuiRadioButton::Style style) {
        
        // Turn enums into ints to allow this to always act as a pointer to an int
        GuiRadioButton* c = addControl(new GuiRadioButton
                          (this, text, myID, 
                           Pointer<int>(object, 
                                        reinterpret_cast<int (T::*)() const>(get), 
                                        reinterpret_cast<void (T::*)(int)>(set)), 
                           style));
        if (style == GuiRadioButton::TOOL_STYLE) {
            c->setSize(Vector2(TOOL_BUTTON_WIDTH, CONTROL_HEIGHT));
        } else if (style == GuiRadioButton::BUTTON_STYLE) {
            c->setSize(Vector2(BUTTON_WIDTH, CONTROL_HEIGHT));
        }
        return c;
    }

    /** Provide the default clamp bounds for addNumberBox.*/
    static int minVal(int x) { return INT_MAX; }
    /** Provide the default clamp bounds for addNumberBox.*/
    static int maxVal(int x) { return INT_MIN; }
    /** Provide the default clamp bounds for addNumberBox.*/
    static double minVal(double x) { return -inf();}
    /** Provide the default clamp bounds for addNumberBox.*/
    static double maxVal(double x) { return inf();}

    /** Create a text box for numbers.
      @param suffix A label to the right of the number, e.g., units
      @param roundIncrement Round typed values to the nearest increment of this, 0 for no rounding.
    */
    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiCaption&   text, 
        const Pointer<Value>& value, 
        const GuiCaption&   suffix = "", 
        bool                showSlider = false, 
        Value               min = (Value)minVal(Value()), 
        Value               max = (Value)maxVal(Value()), 
        Value               roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, value, suffix, showSlider, min, max, roundIncrement));
    }

    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiCaption&   text, 
        Value*              value, 
        const GuiCaption&   suffix = "", 
        bool                showSlider = false, 
        Value               min = (Value)minVal(Value()), 
        Value               max = (Value)maxVal(Value()), 
        Value               roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, Pointer<Value>(value), suffix, showSlider, min, max, roundIncrement));
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiCaption& text, const Pointer<Value>& value, Value min, Value max, bool horizontal = true) {
        return addControl(new GuiSlider<Value>(this, text, value, min,  max, horizontal));
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiCaption& text, Value* value, Value min, Value max, bool horizontal = true) {
        return addSlider(text, Pointer<Value>(value), min,  max, horizontal);
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
    GuiRadioButton* addRadioButton(const GuiCaption& text, int myID, void* selection, GuiRadioButton::Style style = GuiRadioButton::RADIO_STYLE);

	GuiButton* addButton(const GuiCaption& text, const GuiControl::Callback& callback, GuiTheme::ButtonStyle style);

	template<class Class>
	inline GuiButton* addButton(const GuiCaption& text, Class* const callbackObject, void (Class::*callbackMethod)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
		return addButton(text, GuiControl::Callback(callbackObject, callbackMethod), style);
	}

	template<class Class>
	inline GuiButton* addButton(const GuiCaption& text, const ReferenceCountedPointer<Class>& callbackObject, void (Class::*callbackMethod)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
		return addButton(text, GuiControl::Callback(callbackObject, callbackMethod), style);
	}

	inline GuiButton* addButton(const GuiCaption& text, void (*callbackFunction)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
		return addButton(text, GuiControl::Callback(callbackFunction), style);
	}

	GuiButton* addButton(const GuiCaption& text, GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE);

    GuiLabel* addLabel(const GuiCaption& text, GFont::XAlign xalign = GFont::XALIGN_LEFT, GFont::YAlign = GFont::YALIGN_CENTER);

    /**
     Removes this control from the GuiPane.
     */
    void remove(GuiControl* gui);

    /** 
        Resize this pane so that all of its controls are visible and so that there is
        no wasted space.

        @sa G3D::GuiWindow::pack
     */
    void pack();
};

}

#endif

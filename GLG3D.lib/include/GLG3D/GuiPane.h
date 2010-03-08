/**
 @file GLG3D/GuiPane.h

 @created 2006-05-01
 @edited  2010-01-13

 G3D Library http://g3d.sf.net
 Copyright 2000-2010, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiPane_h
#define G3D_GuiPane_h

#include <string>
#include <limits.h>
#include "G3D/Pointer.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
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
#include "GLG3D/GuiFunctionBox.h"
#include "GLG3D/GuiTextureBox.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class GuiButton;
class GuiTabPane;

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

    _internal::Morph        m_morph;

    GuiTheme::PaneStyle     m_style;

    Array<GuiControl*>      controlArray;

    /** Sub panes */
    Array<GuiContainer*>    containerArray;

    Array<GuiLabel*>        labelArray;

    GuiPane(GuiWindow* gui, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style);

public:

    /** For use by GuiContainers.  \sa GuiPane::addPane, GuiWindow::pane */
    GuiPane(GuiContainer* parent, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style);

private:
    /**
       Called from constructors.
     */
    void init(const Rect2D& rect);

    /** Finds the next vertical position for a control relative to the client rect. */
    Vector2 nextControlPos(bool isTool = false) const;

    template<class T>
    T* addControl(T* control, float height = CONTROL_HEIGHT) {
        Vector2 p = nextControlPos(control->toolStyle());
        control->setRect
            (Rect2D::xywh(p, Vector2((float)CONTROL_WIDTH, height)));

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

    virtual void render(RenderDevice* rd, const GuiThemeRef& theme) const;

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;

    /** If the original caption was non-empty (even if it was " "), 
        the new caption will be shown.*/
    virtual void setCaption(const GuiText& caption);    

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
    GuiControl* addCustom(GuiControl* control);

    /** 
        If the text is "", no space is reserved for a caption.  If non-empty (even " "), then
        space is reserved and the caption may later be changed.
     */
    GuiPane* addPane(const GuiText& text = "", GuiTheme::PaneStyle style = GuiTheme::SIMPLE_PANE_STYLE);

    GuiTextureBox* addTextureBox(const GuiText& caption = "",
                                 const Texture::Ref& t = NULL,
                                 const GuiTextureBox::Settings&  s = GuiTextureBox::Settings(),
                                 bool embedded = false);

    GuiTextureBox* addTextureBox(const Texture::Ref& t,
                                 const GuiTextureBox::Settings&  s = GuiTextureBox::Settings(),
                                 bool embedded = false);

    GuiTabPane* addTabPane(const Pointer<int>& index = NULL);

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
    (const GuiText& text,
     const Pointer<bool>& pointer,
     GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE);

    GuiCheckBox* addCheckBox
    (const GuiText& text,
     bool* pointer,
     GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE
     ) {
        return addCheckBox(text, Pointer<bool>(pointer), style);
    }

    GuiFunctionBox* addFunctionBox(const GuiText& text, Spline<float>* spline);

    GuiTextBox* addTextBox
    (const GuiText& caption,
     const Pointer<std::string>& stringPointer,
     GuiTextBox::Update update = GuiTextBox::DELAYED_UPDATE
     ) {        
        return addControl(new GuiTextBox(this, caption, stringPointer, update));
    }

    /**
     \brief Add a drop-down list.

      You can make the \a indexPointer reference an enum type by casting:
      
      <pre>
      Pointer<int>(objectPointer, 
                   reinterpret_cast<int (Class::*)() const>(getMethod), 
                   reinterpret_cast<void (Class::*)(int)>(setMethod))) 
     */
    GuiDropDownList* addDropDownList(const GuiText& caption, const Array<std::string>& list, const Pointer<int>& indexPointer = NULL, const GuiControl::Callback& actionCallback = GuiControl::Callback());
    GuiDropDownList* addDropDownList(const GuiText& caption, const Array<GuiText>& list = Array<GuiText>(), const Pointer<int>& indexPointer = NULL, const GuiControl::Callback& actionCallback = GuiControl::Callback());

    GuiRadioButton* addRadioButton(const GuiText& text, int myID,  
        const Pointer<int>& ptr, 
        GuiTheme::RadioButtonStyle style);

    template<typename EnumOrInt, class T>
    GuiRadioButton* addRadioButton(const GuiText& text, int myID,  
        T* object,
        EnumOrInt (T::*get)() const,
        void (T::*set)(EnumOrInt), 
        GuiTheme::RadioButtonStyle style) {
        
            return addRadioButton(text, myID, Pointer<int>(object, 
                                        reinterpret_cast<int (T::*)() const>(get), 
                                        reinterpret_cast<void (T::*)(int)>(set)), style);
    }

    /** Provide the default clamp bounds for addNumberBox.*/
    static int minVal(int x) { (void)x;return INT_MAX; }
    /** Provide the default clamp bounds for addNumberBox.*/
    static int maxVal(int x) { (void)x;return INT_MIN; }
    /** Provide the default clamp bounds for addNumberBox.*/
    static double minVal(double x) { (void)x;return -inf();}
    /** Provide the default clamp bounds for addNumberBox.*/
    static double maxVal(double x) { (void)x;return inf();}

    /** Create a text box for numbers.
      @param suffix A label to the right of the number, e.g., units
      @param roundIncrement Round typed values to the nearest increment of this, 0 for no rounding.
    */
    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiText&   text, 
        const Pointer<Value>& value, 
        const GuiText&   suffix = "", 
        GuiTheme::SliderScale sliderScale = GuiTheme::NO_SLIDER, 
        Value               min = (Value)minVal(Value()), 
        Value               max = (Value)maxVal(Value()), 
        Value               roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, value, suffix, sliderScale, 
                                                  min, max, roundIncrement));
    }

    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiText&   text, 
        Value*              value, 
        const GuiText&   suffix = "", 
        GuiTheme::SliderScale sliderScale = GuiTheme::NO_SLIDER, 
        Value               min = (Value)minVal(Value()), 
        Value               max = (Value)maxVal(Value()), 
        Value               roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, Pointer<Value>(value), 
                                                  suffix, sliderScale, min, max, roundIncrement));
    }

    /** @deprecated Use the new version with a GuiTheme::SliderScale argument.*/
    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiText&   text, 
        Value*           value, 
        const GuiText&   suffix,
        bool             showSlider,
        Value            min = (Value)minVal(Value()), 
        Value            max = (Value)maxVal(Value()), 
        Value            roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, Pointer<Value>(value), 
            suffix, showSlider ? GuiTheme::LINEAR_SLIDER : GuiTheme::NO_SLIDER, min, max, roundIncrement));
    }

    /** @deprecated Use the new version with a GuiTheme::SliderScale argument.*/
    template<typename Value>
    GuiNumberBox<Value>* addNumberBox(
        const GuiText&   text, 
        const Pointer<Value>& value, 
        const GuiText&   suffix, 
        bool                showSlider, 
        Value               min = (Value)minVal(Value()), 
        Value               max = (Value)maxVal(Value()), 
        Value               roundIncrement = 0) {

        return addControl(new GuiNumberBox<Value>(this, text, value, suffix, showSlider ? GuiTheme::LINEAR_SLIDER : GuiTheme::NO_SLIDER, 
                                                  min, max, roundIncrement));
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiText& text, const Pointer<Value>& value, 
        Value min, Value max, bool horizontal = true, GuiTheme::SliderScale scale = GuiTheme::LINEAR_SLIDER) {
        return addControl(new GuiSlider<Value>(this, text, value, min,  max, horizontal, scale));
    }

    template<typename Value>
    GuiSlider<Value>* addSlider(const GuiText& text, Value* value, 
                                Value min, Value max, bool horizontal = true, GuiTheme::SliderScale scale = GuiTheme::LINEAR_SLIDER) {
        return addSlider(text, Pointer<Value>(value), min, max, horizontal, scale);
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
    GuiRadioButton* addRadioButton(const GuiText& text, int myID, void* selection, 
                                   GuiTheme::RadioButtonStyle style = GuiTheme::NORMAL_RADIO_BUTTON_STYLE);

    GuiButton* addButton(const GuiText& text, const GuiControl::Callback& actionCallback, GuiTheme::ButtonStyle style);

    template<class Class>
    inline GuiButton* addButton(const GuiText& text, Class* const callbackObject, void (Class::*callbackMethod)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
        return addButton(text, GuiControl::Callback(callbackObject, callbackMethod), style);
    }
    
    template<class Class>
    inline GuiButton* addButton(const GuiText& text, const ReferenceCountedPointer<Class>& callbackObject, void (Class::*callbackMethod)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
        return addButton(text, GuiControl::Callback(callbackObject, callbackMethod), style);
    }
    
    inline GuiButton* addButton(const GuiText& text, void (*callbackFunction)(), GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE) {
        return addButton(text, GuiControl::Callback(callbackFunction), style);
    }
    
    GuiButton* addButton(const GuiText& text, GuiTheme::ButtonStyle style = GuiTheme::NORMAL_BUTTON_STYLE);
    
    /** @param xalign Horizontal alignment of text within the rect of the label 
        @param yalign Vertical alignment of text within the rect of the label 
    */
    GuiLabel* addLabel(const GuiText& text, GFont::XAlign xalign = GFont::XALIGN_LEFT,
                       GFont::YAlign yalign = GFont::YALIGN_CENTER);

    /**
     Removes this control from the GuiPane.
     */
    void remove(GuiControl* gui);

    /** 
        Resize this pane so that all of its controls are visible and
        so that there is no wasted space.

        @sa G3D::GuiWindow::pack
     */
    void pack();
};

}

#endif

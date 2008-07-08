/**
 @file GLG3D/GuiSlider.h

 @created 2006-05-01
 @edited  2008-01-18

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2008, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUISLIDER_H
#define G3D_GUISLIDER_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;
class GuiPane;

class _GuiSliderBase : public GuiControl {
    friend class GuiPane;

protected:

    bool              m_horizontal;
    bool              m_inDrag;
    float             m_dragStartValue;

    _GuiSliderBase(GuiContainer* parent, const GuiCaption& text, bool horizontal);

    /** Position from which the mouse drag started, relative to
        m_gui.m_clientRect.  When dragging the thumb, the cursor may not be
        centered on the thumb the way it is when the mouse clicks
        on the track.
    */
    Vector2           m_dragStart;
    
    /** Get value on the range 0 - 1 */
    virtual float floatValue() const = 0;

    /** Set value on the range 0 - 1 */
    virtual void setFloatValue(float f) = 0;

    virtual bool onEvent(const GEvent& event);

public:

    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

};


/** Slider.  See GuiWindow for an example of creating a slider. 

    <ul>
     <li> GEventType::GUI_ACTION when the thumb is released.
     <li> GEventType::GUI_CHANGE during scrolling.
     <li> GEventType::GUI_DOWN when the mouse is pressed down.
     <li> GEventType::GUI_UP when the mouse is released.
    </ul>

    The min/mix values are enforced on the GUI, but not on the value 
    if it is changed programmatically.
*/
template<class Value>
class GuiSlider : public _GuiSliderBase {
    friend class GuiPane;
    friend class GuiWindow;
protected:

    Pointer<Value>    m_value;
    Value             m_minValue;
    Value             m_maxValue;

    float floatValue() const {
        return (float)(*m_value - m_minValue) / (float)(m_maxValue - m_minValue);
    }

    void setFloatValue(float f) {
        *m_value = (Value)(f * (m_maxValue - m_minValue) + m_minValue);
    }

public:

    /** Public for GuiNumberBox.  Do not call 
    @param eventSource if null, set to this.
     */
    GuiSlider(GuiContainer* parent, const GuiCaption& text, 
              const Pointer<Value>& value, Value minValue, Value maxValue, bool horizontal, GuiControl* eventSource = NULL) :
        _GuiSliderBase(parent, text, horizontal), m_value(value), 
        m_minValue(minValue), m_maxValue(maxValue) {
        if (eventSource != NULL) {
            m_eventSource = eventSource;
        }
    }

    Value minValue() const {
        return m_minValue;
    }

    Value maxValue() const {
        return m_maxValue;
    }

    void setRange(Value lo, Value hi) {
        m_minValue = min(lo, hi);
        m_maxValue = max(lo, hi);
    }

};

} // G3D

#endif

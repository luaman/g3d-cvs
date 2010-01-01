/**
 @file GLG3D/GuiSlider.h

 @created 2006-05-01
 @edited  2008-01-18

 G3D Library http://g3d.sf.net
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

    _GuiSliderBase(GuiContainer* parent, const GuiText& text, bool horizontal);

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

/** Used by GuiSlider */
template<class T>
class LogScaleAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<T>;

    Pointer<T>      m_source;
    double          m_low;
    double          m_high;

    /** m_high - m_low */
    double          m_range;

    double          m_base;
    double          m_logBase;

    typedef ReferenceCountedPointer<LogScaleAdapter> Ptr;

    LogScaleAdapter(Pointer<T> ptr, const T& low, const T& high) : 
        m_source(ptr), m_low((double)low), m_high((double)high) {
        m_range = m_high - m_low;

        m_base = max(10.0, m_range / 100.0);
        m_logBase = ::log(m_base);
    }

    /** For use by Pointer<T> */
    T get() const {
        if (m_range == 0) {
            // No scaling necessary
            return m_source.getValue();
        }

        double v = double(m_source.getValue());

        // Normalize the value
        double y = (double(v) - m_low) / m_range;

        // Scale logarithmically
        double x = ::log(y * (m_base - 1.0) + 1.0) / m_logBase;

        // Expand range
        return T(x * m_range + m_low);
    }

    /** For use by Pointer<T> */
    void set(const T& v) {
        if (m_range == 0) {
            // No scaling necessary
            m_source.setValue(v);
            return;
        }

        // Normalize the value to the range (0, 1)
        double x = (v - m_low) / m_range;

        // Keep [0, 1] range but scale exponentially
        double y = (::pow(m_base, x) - 1.0) / (m_base - 1.0);

        m_source.setValue(T(y * m_range + m_low));
    }

public:


    /** @brief Converts a pointer to a linear scale value on the range 
        [@a low, @a high] to a logarithic scale value on the same 
        range. 
        
        Note that the scale is spaced logarithmically between
        low and high. However, the transformed value is not the logarithm 
        of the value, so low = 0 is supported, but negative low values 
        will not yield a negative logarithmic scale. */
    static Pointer<T> wrap(Pointer<T> ptr, const T& low, const T& high) {
        debugAssert(high >= low);
        Ptr p = new LogScaleAdapter(ptr, low, high);
        return Pointer<T>(p, &LogScaleAdapter<T>::get, &LogScaleAdapter<T>::set);
    }
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
    GuiSlider(GuiContainer* parent, const GuiText& text, 
              const Pointer<Value>& value, Value minValue, Value maxValue, bool horizontal, 
              GuiTheme::SliderScale scale,
              GuiControl* eventSource = NULL) :
        _GuiSliderBase(parent, text, horizontal), 
        m_minValue(minValue), m_maxValue(maxValue) {

        debugAssertM(scale != GuiTheme::NO_SLIDER, "Cannot construct a slider with GuiTheme::NO_SLIDER");
        if (scale == GuiTheme::LOG_SLIDER) {
            m_value = LogScaleAdapter<Value>::wrap(value, minValue, maxValue);
        } else {
            m_value = value;
        }

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

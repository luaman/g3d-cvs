/**
 @file GLG3D/GuiNumberBox.h

 @created 2008-07-09
 @edited  2008-10-15

 G3D Library http://g3d.sf.net
 Copyright 2001-2008, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GUINUMBERBOX_H
#define G3D_GUINUMBERBOX_H

#include "G3D/Pointer.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiSlider.h"

namespace G3D {

class GuiWindow;
class GuiPane;


/** Number editing textbox with associated slider.  
    See GuiWindow for an example of creating a number box. 

    <b>Events:</b>
    <ul>
      <li> GEventType::GUI_ACTION when the slider thumb is released or enter is pressed in the text box.
      <li> GEventType::GUI_CHANGE during slider scrolling.
      <li> GEventType::GUI_DOWN when the mouse is pressed down on the slider.
      <li> GEventType::GUI_UP when the mouse is released on the slider.
      <li> GEventType::GUI_CANCEL when ESC is pressed in the text box
    </ul>

    The min/mix/rounding values are enforced on the GUI, but not on the underlying value 
    if it is changed programmatically. 

    "nan", "inf", and "-inf" are all parsed to the appropriate floating point values.

    @sa G3D::GuiPane::addNumberBox
*/
template<class Value>
class GuiNumberBox : public GuiContainer {
    friend class GuiPane;
    friend class GuiWindow;

protected:

    // Constants
    enum {textBoxWidth = 60};

    /** Current value */
    Pointer<Value>    m_value;

    /** Value represented by m_textValue */
    Value             m_oldValue;

    /** Text version of value */
    std::string       m_textValue;

    std::string       m_formatString;

    /** Round to the nearest multiple of this value. */
    Value             m_roundIncrement;

    Value             m_minValue;
    Value             m_maxValue;

    /** NULL if there is no slider */
    GuiSlider<Value>* m_slider;

    GuiTextBox*       m_textBox;

    GuiText           m_units;
    float             m_unitsSize;

    // Methods declared in the middle of member variables
    // to provide forward declarations.  Do not change 
    // declaration order.

    void roundAndClamp(Value& v) const {
        if (m_roundIncrement != 0) {
            v = (Value)floor(v / (double)m_roundIncrement + 0.5) * m_roundIncrement;
        }
        if (v < m_minValue) {
            v = m_minValue;
        }
        if (v > m_maxValue) {
            v = m_maxValue;
        }
    }

    void updateText() {
        // The text display is out of date
        m_oldValue = *m_value;
        roundAndClamp(m_oldValue);
        *m_value = m_oldValue;
        if (m_oldValue == inf()) {
            m_textValue = "inf";
        } else if (m_oldValue == -inf()) {
            m_textValue = "-inf";
        } else if (isNaN(m_oldValue)) {
            m_textValue = "nan";
        } else {
            m_textValue = format(m_formatString.c_str(), m_oldValue);
        }
    }

    /** Called when the user commits the text box. */
    void commit() {
        double f = (double)m_minValue;
        std::string s = m_textValue;

        // Parse the m_textValue to a float
        int numRead = sscanf(s.c_str(), "%lf", &f);
        
        if (numRead == 1) {
            // Parsed successfully
            *m_value = (Value)f;
        } else {
            s = trimWhitespace(toLower(s));

            if (m_textValue == "inf") {
                f = inf();
            } else if (m_textValue == "-inf") {
                f = -inf();
            } else if (m_textValue == "nan") {
                f = nan();
            }
        }
        updateText();
    }

    /** Notifies the containing pane when the text is commited. */
    class MyTextBox : public GuiTextBox {
    protected:
        GuiNumberBox<Value>*      m_numberBox;

        virtual void commit() {
            GuiTextBox::commit();
            m_numberBox->commit();
        }
    public:

        MyTextBox(GuiNumberBox<Value>* parent, const GuiText& caption, 
               const Pointer<std::string>& value, Update update) :
               GuiTextBox(parent, caption, value, update),
               m_numberBox(parent) {
           // Make events appear to come from parent
           m_eventSource = parent;
       }
    };
  
    ////////////////////////////////////////////////////////
    // The following overloads allow GuiNumberBox to select the appropriate format() string
    // based on Value.

    // Smaller int types will automatically convert
    static std::string formatString(int v, int roundIncrement) {
        (void)roundIncrement;
        (void)v;
        return "%d";
    }

    static std::string formatString(int64 v, int64 roundIncrement) {
#       ifdef MSVC
            // http://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx
            return "%I64d";
#       else
            // http://www.gnu.org/software/libc/manual/html_node/Integer-Conversions.html
            return "%lld";
#       endif
    }

    /** Computes the format string for the precision specification needed to see the most
        significant digit of roundIncrement. */
    static std::string precision(double roundIncrement) {
        if (roundIncrement == 0) {
            return "";
        } else if (roundIncrement > 1) {
            // Show only the integer part
            return ".0";
        } else {
            // Compute the number of decimal places needed for the roundIncrement
            int n = iCeil(-log10(roundIncrement));
            return format(".%d", n);
        }
    }

    static std::string formatString(float v, float roundIncrement) {
        (void)v;
        return "%" + precision(roundIncrement) + "f";
    }

    std::string formatString(double v, double roundIncrement) {
        (void)v;
        return "%" + precision(roundIncrement) + "lf";
    }

    /////////////////////////////////////////////////////////////////

    GuiNumberBox(
        GuiContainer*           parent, 
        const GuiText&          caption, 
        const Pointer<Value>&   value, 
        const GuiText&          units,
        GuiTheme::SliderScale   scale,
        Value                   minValue, 
        Value                   maxValue,
        Value                   roundIncrement) :
        GuiContainer(parent, caption),
        m_value(value),
        m_roundIncrement(roundIncrement),
        m_minValue(minValue),
        m_maxValue(maxValue),
        m_slider(NULL),
        m_textBox(NULL),
        m_units(units),
        m_unitsSize(22) {

        debugAssert(m_roundIncrement >= 0);

        if (scale != GuiTheme::NO_SLIDER) {
            debugAssertM(m_minValue > -inf() && m_maxValue < inf(), 
                "Cannot have a NumberBox with infinite bounds and a slider");

            m_slider = new GuiSlider<Value>(this, "", value, m_minValue, m_maxValue, true, scale, this);
        }

        m_textBox = new MyTextBox(this, "", &m_textValue, GuiTextBox::DELAYED_UPDATE);

        m_formatString = formatString(*value, roundIncrement);

        m_oldValue = *m_value;
        roundAndClamp(m_oldValue);
        *m_value = m_oldValue;
        updateText();
    }

public:

    ~GuiNumberBox() {
        delete m_textBox;
        delete m_slider;
    }

    // The return value is not a const reference, since value is usually int or float
    Value minValue() const {
        return m_minValue;
    }

    Value maxValue() const {
        return m_maxValue;
    }

    virtual void setCaption(const std::string& c) {
        GuiContainer::setCaption(c);

        // Resize other parts in response to caption size changing
        setRect(m_rect);
    }

    void setRange(Value lo, Value hi) {
        if (m_slider != NULL) {
            m_slider->setRange(lo, hi);
        }

        m_minValue = min(lo, hi);
        m_maxValue = max(lo, hi);
    }

    virtual void setRect(const Rect2D& rect) {
        GuiContainer::setRect(rect);

        float controlSpace = m_rect.width() - m_captionSize;

        m_textBox->setRect(Rect2D::xywh(m_captionSize, 0, textBoxWidth, CONTROL_HEIGHT));

        if (m_slider != NULL) {
            float x = m_textBox->rect().x1() + m_unitsSize;
            m_slider->setRect(Rect2D::xywh(x, 0.0f, 
                max(controlSpace - (x - m_captionSize) - 2, 5.0f), (float)CONTROL_HEIGHT));
        }
    }

    void setUnitsSize(float s) {
        m_unitsSize = s;
        setRect(rect());
    }

    /** The number of pixels between the text box and the slider.*/
    float unitsSize() const {
        return m_unitsSize;
    }

    virtual void setEnabled(bool e) {
        m_textBox->setEnabled(e);
        if (m_slider != NULL) {
            m_slider->setEnabled(e);
        }
    }

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const {
        if (! m_clientRect.contains(mouse) || ! m_visible) {
            return;
        }

        mouse -= m_clientRect.x0y0();
        if (m_textBox->clickRect().contains(mouse) && m_textBox->visible() && m_textBox->enabled()) {
            control = m_textBox;
        } else if ((m_slider != NULL) && m_slider->clickRect().contains(mouse) && m_slider->visible() && m_slider->enabled()) {
            control = m_slider;
        }
    }

    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const {
        if (! m_visible) {
            return;
        }
        if (m_oldValue != *m_value) {
            const_cast<GuiNumberBox<Value>*>(this)->updateText();
        }

        skin->pushClientRect(m_clientRect);
            m_textBox->render(rd, skin);

            // Don't render the slider if there isn't enough space for it 
            if ((m_slider != NULL) && (m_slider->rect().width() > 10)) {
                m_slider->render(rd, skin);
            }

            // Render caption and units
            skin->renderLabel(m_rect - m_clientRect.x0y0(), m_caption, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER, m_enabled);

            const Rect2D& textBounds = m_textBox->rect();
            skin->renderLabel(Rect2D::xywh(textBounds.x1y0(), Vector2(m_unitsSize, textBounds.height())), 
                m_units, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER, m_enabled);
        skin->popClientRect();
    }

};

} // G3D

#endif

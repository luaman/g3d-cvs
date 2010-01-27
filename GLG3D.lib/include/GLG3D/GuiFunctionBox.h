/**
  @file GuiFunctionBox.h
 
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  @created 2007-10-02
  @edited  2008-07-10
 */

#ifndef G3D_GuiFunctionBox_h
#define G3D_GuiFunctionBox_h

#include "G3D/Spline.h"
#include "G3D/Vector2.h"
#include "G3D/Rect2D.h"
#include "G3D/Color4.h"
#include "GLG3D/GuiControl.h"

namespace G3D {

class RenderDevice;
class GuiWindow;
class GuiPane;

/**
   Displays a Spline<float> with editable control points.
 */
class GuiFunctionBox : public GuiControl {
protected:
    friend class GuiPane;

    enum {NONE = -1};

    /** Index of the currently selected control point, NONE if no selection */
    int             m_selected;

    Color4          m_splineColor;
    Color4          m_controlColor;
    Color4          m_gridColor;

    float           m_minTime;
    float           m_maxTime;
    float           m_minValue;
    float           m_maxValue;

    /** Derivative of (time, control) per pixel, computed during rendering. */
    Vector2         m_scale;

    /** True if dragging m_selected */
    bool            m_drag;

    /** Mouse position when drag began */
    Vector2         m_mouseStart;

    /** m_spline->time[m_selected] when drag began */
    float           m_timeStart;

    /** m_spline->control[m_selected] when drag began */
    float           m_valueStart;
    
    Spline<float>*  m_spline;

    /** Rendering client bounds updated by every render. */
    Rect2D          m_bounds;

    /** Bounds for mouse clicks and scissor region, updated by every render. */
    Rect2D          m_clipBounds;

    /** Returns the pixel position at which this control point will render. */
    Vector2 controlPointLocation(int i) const;

    void drawBackground(RenderDevice* rd, const GuiThemeRef& skin) const;
    void drawSpline(RenderDevice* rd, const GuiThemeRef& skin) const;
    void drawControlPoints(RenderDevice* rd, const GuiThemeRef& skin) const;

    /** Called by GuiWindow */
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    virtual bool onEvent(const GEvent& event);

    /** Ensure that spline values fit the legal range. */
    void clampValues();
    
    /** Ensures that all spline time samples are monotonically
        increasing and within bounds.  Works outwards from element @a
        i. */
    void clampTimes(int i = NONE);

    /** Returns the control point nearest this location, creating it
        if there is no nearby control point.
     */
    int getNearestControlPoint(const Vector2& pixelLoc);

    GuiFunctionBox(GuiContainer* parent, const GuiText& text, Spline<float>* spline);

public:


    /** Default to [0, 1] */
    void setTimeBounds(float minTime, float maxTime) {
        m_minTime = minTime;
        m_maxTime = maxTime;
        clampTimes(m_selected);
    }

    /** Setting these automatically clamps all spline points to the
        legal range.*/
    void setValueBounds(float minVal, float maxVal) {
        m_minValue = minVal;
        m_maxValue = maxVal;
        clampValues();
        clampTimes();
    }
    
    virtual float defaultCaptionSize() const;

    void setCurveColor(const Color4& c) {
        m_splineColor = c;
    }

    void setControlPointColor(const Color4& c) {
        m_controlColor = c;
    }

    void setGridColor(const Color4& c) {
        m_gridColor = c;
    }
};

} // G3D
#endif

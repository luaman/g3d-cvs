/**
 @file GuiFunctionBox.cpp

 @author Morgan McGuire, morgan@cs.williams.edu
 */
#include "GLG3D/GuiFunctionBox.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiTheme.h"

namespace G3D {

GuiFunctionBox::GuiFunctionBox(GuiContainer* parent, const GuiCaption& text, 
                               Spline<float>* spline) : GuiControl(parent, text), m_spline(spline) {

    setCaptionSize(TOP_CAPTION_SIZE);
    setSize(Vector2(190, 120));

    m_minTime  = 0.0f;
    m_maxTime  = 1.0f;
    m_minValue = 0.0f;
    m_maxValue = 1.0f;

    m_splineColor  = Color3::red();
    m_gridColor    = Color3::white() * 0.9f;
    m_controlColor = Color3::black();

    m_selected = NONE;

    clampValues();
    clampTimes();
}


void GuiFunctionBox::clampValues() {
    for (int i = 0; i < m_spline->control.size(); ++i) {
        m_spline->control[i] = clamp(m_spline->control[i], m_minValue, m_maxValue);
    }
}


void GuiFunctionBox::clampTimes(int start) {
    // Ensure that times are at least this far apart
    const float minDistance = 0.01f;

    m_spline->cyclic = false;

    // Ensure minimum distance between elements
    if (start >= 0) {
        // Work backwards towards 0
        for (int i = start - 1; i >= 0; --i) {
            m_spline->time[i] = min(m_spline->time[i], m_spline->time[i + 1] - minDistance);
        }
        
        // Work forwards towards the end
        for (int i = start + 1; i < m_spline->time.size(); ++i) {
            m_spline->time[i] = max(m_spline->time[i], m_spline->time[i - 1] + minDistance);
        }
    }

    // Ensure all values are in bounds
    for (int i = 0; i < m_spline->time.size(); ++i) {
        m_spline->time[i] = clamp(m_spline->time[i], m_minTime, m_maxTime);
    }

    // Ensure minimum distance from boundaries
    // Work backwards towards 0
    for (int i = m_spline->time.size() - 2; i >= 0; --i) {
        m_spline->time[i] = min(m_spline->time[i], m_spline->time[i + 1] - minDistance);
    }

    // Work forwards towards the end
    for (int i = 1; i < m_spline->time.size(); ++i) {
        m_spline->time[i] = max(m_spline->time[i], m_spline->time[i - 1] + minDistance);
    }
    
}


float GuiFunctionBox::defaultCaptionSize() const {
    return TOP_CAPTION_SIZE;
}


void GuiFunctionBox::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    GuiFunctionBox* me = const_cast<GuiFunctionBox*>(this);

    me->m_clipBounds = skin->canvasToClientBounds(m_rect, m_captionSize);
    int shrink = 4;
    // Shrink bounds slightly so that we can see axes and points rendered against the edge
    me->m_bounds = Rect2D::xywh(m_clipBounds.x0y0() + Vector2(shrink, shrink),
                                m_clipBounds.wh() - Vector2(shrink, shrink) * 2);

    // Use textbox borders
    skin->renderCanvas(m_rect, m_enabled, focused(), m_caption, m_captionSize);

    me->m_scale.x = (m_maxTime  - m_minTime)  / m_bounds.width(); 
    me->m_scale.y = (m_maxValue - m_minValue) / m_bounds.height(); 

    static int count = 0;
    count = (count + 1) % 10;
    if (count == 0) {
        // Make sure that the spline hasn't been corrupted by the
        // program since we last checked it.  Avoid doing this every
        // frame, however.
        me->clampTimes(m_selected);
        me->clampValues();
    }

    skin->pauseRendering();
    {
        // Scissor region ignores transformation matrix
        const CoordinateFrame& matrix = rd->getObjectToWorldMatrix();
        rd->enableClip2D(m_clipBounds + matrix.translation.xy());
        drawBackground(rd, skin);
        drawSpline(rd, skin);
        drawControlPoints(rd, skin);
    }
    skin->resumeRendering();
}


void GuiFunctionBox::drawBackground(RenderDevice* rd, const GuiThemeRef& skin) const {
    // Draw grid
    rd->setLineWidth(1);
    rd->beginPrimitive(RenderDevice::LINES);
        rd->setColor(m_gridColor);
        int N = 10;
        for (int i = 0; i < N; ++i) {
            float x = m_bounds.x0() + m_bounds.width() * i / (N - 1.0f);
            float y = m_bounds.y0() + m_bounds.height() * i / (N - 1.0f);
            // Horizontal line
            rd->sendVertex(Vector2(m_bounds.x0(), y));
            rd->sendVertex(Vector2(m_bounds.x1(), y));

            // Vertical line
            rd->sendVertex(Vector2(x, m_bounds.y0()));
            rd->sendVertex(Vector2(x, m_bounds.y1()));
        }
    rd->endPrimitive();
}


void GuiFunctionBox::drawSpline(RenderDevice* rd, const GuiThemeRef& skin) const {
    int N = iMax(m_bounds.width() / 4, 30);

#   if 0  // Debugging code for looking at the slope of the curve

    // Show desired tangents in green
    rd->beginPrimitive(RenderDevice::LINES);
    rd->setColor(Color3::green() * 0.5);
    for (int j = 0; j < m_spline->control.size(); ++j) {
        float t, v, x, y;
        int i;

        i = j - 1;
        m_spline->getControl(i, t, v);
        x = m_bounds.x0() + m_bounds.width() * (t - m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 va(x, y);

        i = j;
        m_spline->getControl(i, t, v);
        x = m_bounds.x0() + m_bounds.width() * (t - m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 vb(x, y);

        i = j + 1;
        m_spline->getControl(i, t, v);
        x = m_bounds.x0() + m_bounds.width() * (t - m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 vc(x, y);

        Vector2 tangent = (vc - va).direction() * m_bounds.width() / (2 * m_spline->control.size());
        rd->sendVertex(vb - tangent);
        rd->sendVertex(vb + tangent);
    }
    rd->endPrimitive();

    // Show true tangents in blue
    rd->beginPrimitive(RenderDevice::LINES);
    rd->setColor(Color3::blue());
    for (int i = 1; i < m_spline->control.size() - 1; ++i) {
        float t, v, x, y;

        float dt = (m_spline->time[i + 1] - m_spline->time[i - 1]) / 10;

        t = m_spline->time[i] - dt;
        v = m_spline->evaluate(t);
        x = m_bounds.x0() + m_bounds.width() * (t - m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 va(x, y);

        t = m_spline->time[i];
        v = m_spline->control[i];
        x = m_bounds.x0() + m_bounds.width() * (t - m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 vb(x, y);

        t = m_spline->time[i] + dt;
        v = m_spline->evaluate(t);
        x = m_bounds.x0() + m_bounds.width() * t - (m_minTime) / (m_maxTime - m_minTime);
        y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 vc(x, y);

        Vector2 tangent = (vc - va).direction() * m_bounds.width() / (2 * m_spline->control.size());
        rd->sendVertex(vb - tangent);
        rd->sendVertex(vb + tangent);
    }
    rd->endPrimitive();
#   endif

    rd->beginPrimitive(RenderDevice::LINE_STRIP);
        rd->setColor(m_splineColor);
        for (int i = -2; i < N + 2; ++i) {
            float t = (m_maxTime - m_minTime) * i / (N - 1.0f) + m_minTime;
            float v = m_spline->evaluate(t);
            
            float x = m_bounds.x0() + m_bounds.width() * i / (N - 1.0f);
            float y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

            rd->sendVertex(Vector2(x, y));
        }
    rd->endPrimitive();

}


Vector2 GuiFunctionBox::controlPointLocation(int i) const {
    float t = m_spline->time[i];
    float v = m_spline->control[i];
    return Vector2(m_bounds.x0() + (t - m_minTime) / m_scale.x,
                   m_bounds.y1() - (v - m_minValue) / m_scale.y);
}


int GuiFunctionBox::getNearestControlPoint(const Vector2& pos) {
    
    int closestIndex = NONE;
    float closestDistance2 = inf();

    // Find the closest control point
    for (int i = 0; i < m_spline->size(); ++i) {
        Vector2 loc = controlPointLocation(i);
        float distance2 = (loc - pos).squaredLength();

        if (distance2 < closestDistance2) {
            closestIndex = i;
            closestDistance2 = distance2;
        }
    }
    const float THRESHOLD = 10; // pixels

    // See if the discovered control point is within a reasonable distance.
    if (closestIndex != NONE) {
        float distance = sqrt(closestDistance2);

        if (distance <= THRESHOLD) {
            return closestIndex;
        }
    }

    // See if the curve itself is near the click point
    int N = 100;
    closestDistance2 = inf();
    float closestT;
    float closestV;

    for (int i = 0; i < N; ++i) {
        float t = (m_maxTime - m_minTime) * i / (N - 1.0f) + m_minTime;
        float v = m_spline->evaluate(t);
        
        float x = m_bounds.x0() + m_bounds.width() * i / (N - 1.0f);
        float y = m_bounds.y1() - (v - m_minValue) / m_scale.y;

        Vector2 loc(x, y);

        float distance2 = (loc - pos).squaredLength();

        if (distance2 < closestDistance2) {
            closestT = t;
            closestV = v;
            closestDistance2 = distance2;
        }
    }

    if (sqrt(closestDistance2) < THRESHOLD) {
        // Add a control point
        m_selected = false;
        m_drag = false;

        if (m_spline->time[0] > closestT) {
            m_spline->time.insert(0, closestT);
            m_spline->control.insert(0, closestV);
            clampTimes(0);
            clampValues();
            return 0;
        }

        if (m_spline->time.last() < closestT) {
            m_spline->append(closestT, closestV);
            clampTimes(m_spline->size() - 1);
            clampValues();
            return m_spline->size() - 1;
        }

        for (int i = 1; i < m_spline->size(); ++i) {
            if (m_spline->time[i] > closestT) {
                m_spline->time.insert(i, closestT);
                m_spline->control.insert(i, closestV);
                clampTimes(i);
                clampValues();
                return i;
            }
        }
    }
        
    return NONE;
}


void GuiFunctionBox::drawControlPoints(RenderDevice* rd, const GuiThemeRef& skin) const {
    int size = 6;

    rd->setColor(m_controlColor);
    if (m_selected != NONE) {
        rd->setPointSize(size + 2);
        rd->beginPrimitive(RenderDevice::POINTS);
            Vector2 loc = controlPointLocation(m_selected);
            rd->sendVertex(loc);
        rd->endPrimitive();
    }

    rd->setPointSize(size);
    rd->beginPrimitive(RenderDevice::POINTS);
    {
        int N = m_spline->size();
        for (int i = 0; i < N; ++i) {
            Vector2 loc = controlPointLocation(i);
            rd->sendVertex(loc);
        }

        if (m_selected != NONE) {
            rd->setColor(m_splineColor);
            Vector2 loc = controlPointLocation(m_selected);
            rd->sendVertex(loc);
        }
    }
    rd->endPrimitive();
}

    
bool GuiFunctionBox::onEvent(const GEvent& event) {
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && 
        m_clipBounds.contains(Vector2(event.button.x, event.button.y))) {

        Vector2 pos = Vector2(event.button.x, event.button.y);
        // Go from pixel scale to time/value
        
        // Find (or create) nearest control point
        m_selected = getNearestControlPoint(pos);

        if (m_selected != NONE) {
            m_timeStart    = m_spline->time[m_selected];
            m_valueStart   = m_spline->control[m_selected];
            m_mouseStart   = pos;

            // Start drag
            m_drag = true;
        }
        return true;

    } else if (event.type == GEventType::MOUSE_BUTTON_UP) {

        // Stop drag
        m_drag = false;
        return true;

    } else if (m_drag && (event.type == GEventType::MOUSE_MOTION)) {

        Vector2 mouse(event.motion.x, event.motion.y);

        // Move point, clamping adjacents        
        Vector2 delta = mouse - m_mouseStart;
        // Hide weird mouse event delivery
        if (delta.squaredLength() < 100000) {            
            m_spline->time[m_selected]    = m_timeStart  + delta.x * m_scale.x;
            m_spline->control[m_selected] = m_valueStart - delta.y * m_scale.y;

            // Clamp adjacent points
            clampTimes(m_selected);
            clampValues();
            return true;
        }
    } else if ((event.type == GEventType::KEY_DOWN) && (m_selected != NONE) &&
               (m_spline->size() > 1) &&
               ((event.key.keysym.sym == GKey::DELETE) || (event.key.keysym.sym == GKey::BACKSPACE))) {
        
        // Remove this control point
        if (m_selected < m_spline->size()) {
            m_spline->control.remove(m_selected);
            m_spline->time.remove(m_selected);
        }

        m_selected = NONE;
        return true;
    }

    return false;
}

} // G3D

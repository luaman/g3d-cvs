/**
  @file FirstPersonManipulator.cpp

  @maintainer Morgan McGuire, morgan@cs.brown.edu

  @created 2002-07-28
  @edited  2008-07-15
*/

#include "G3D/platform.h"

#include "G3D/Rect2D.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/UserInput.h"

namespace G3D {

FirstPersonManipulatorRef FirstPersonManipulator::create() {
    return new FirstPersonManipulator();
}


FirstPersonManipulator::FirstPersonManipulator() : 
    m_maxMoveRate(10),
    m_maxTurnRate(20),
    m_yaw(0),
    m_pitch(0),
    m_active(false),
    m_mouseMode(MOUSE_DIRECT) {
}


void FirstPersonManipulator::getFrame(CoordinateFrame& c) const {
	c.translation = m_translation;
    c.rotation = Matrix3::fromEulerAnglesZYX(0, -m_yaw, -m_pitch);

    debugAssert(isFinite(c.rotation[0][0]));

    debugAssertM(c.rotation[1][1] >= 0, 
        "y-axis tipped under the equator due to an internal "
        "inconsistency in FirstPersonManipulator");

    debugAssertM(fuzzyEq(c.rotation[1][0], 0.0f),
        "x-axis is not in the plane of the equator due to an internal "
        "inconsistency in FirstPersonManipulator");
}


CoordinateFrame FirstPersonManipulator::frame() const {
	CoordinateFrame c;
	getFrame(c);
	return c;
}


FirstPersonManipulator::~FirstPersonManipulator() {
}


FirstPersonManipulator::MouseMode FirstPersonManipulator::mouseMode() const {
    return m_mouseMode;
}


void FirstPersonManipulator::setMouseMode(FirstPersonManipulator::MouseMode m) {
    if (m_mouseMode != m) {
        bool wasActive = active();

        if (wasActive) {
            // Toggle activity to let the cursor and 
            // state variables reset.
            setActive(false);
        }

        m_mouseMode = m;

        if (wasActive) {
            setActive(true);
        }
    }
}


bool FirstPersonManipulator::active() const {
    return m_active;
}


void FirstPersonManipulator::reset() {
    m_active      = false;
    m_yaw        = -halfPi();
    m_pitch      = 0;
    m_translation  = Vector3::zero();
    setMoveRate(10);

#   ifdef G3D_OSX
        // OS X has a really slow mouse by default
        setTurnRate(pi() * 12);
#   else
        setTurnRate(pi() * 5);
#   endif
}

bool FirstPersonManipulator::rightDown(UserInput* ui) const {
#   ifdef G3D_OSX
       return 
           ui->keyDown(GKey::RIGHT_MOUSE) || 
           (ui->keyDown(GKey::LEFT_MOUSE) && 
            (
             ui->keyDown(GKey::LSHIFT) ||
             ui->keyDown(GKey::RSHIFT) ||
             ui->keyDown(GKey::LCTRL) ||
             ui->keyDown(GKey::RCTRL)));
#   else
       return ui->keyDown(GKey::RIGHT_MOUSE);
#   endif
}

void FirstPersonManipulator::setActive(bool a) {
    if (m_active == a) {
        return;
    }
    m_active = a;

    switch (m_mouseMode) {
    case MOUSE_DIRECT:
        m_userInput->setPureDeltaMouse(m_active);
        break;

    case MOUSE_DIRECT_RIGHT_BUTTON:
        // Only turn on when activeand the right mouse button is down
        m_userInput->setPureDeltaMouse(m_active && rightDown(m_userInput));
        break;

    case MOUSE_SCROLL_AT_EDGE:
    case MOUSE_PUSH_AT_EDGE:        
        m_userInput->setPureDeltaMouse(false);
        if (m_active) {
            m_userInput->window()->incInputCaptureCount();
        } else {
            m_userInput->window()->decInputCaptureCount();
        }
        break;

    default:
        debugAssert(false);
    }
}


void FirstPersonManipulator::setMoveRate(double metersPerSecond) {
    m_maxMoveRate = metersPerSecond;
}


void FirstPersonManipulator::setTurnRate(double radiansPerSecond) {
    m_maxTurnRate = radiansPerSecond;
}


void FirstPersonManipulator::lookAt(
    const Vector3&      position) {

    const Vector3 look = (position - m_translation);

    m_yaw   = aTan2(look.x, -look.z);
    m_pitch = -aTan2(look.y, distance(look.x, look.z));
}


void FirstPersonManipulator::setFrame(const CoordinateFrame& c) {
    Vector3 look = c.lookVector();

    setPosition(c.translation);

    // this is work towards a patch for bug #1022341
    /*
    if (fuzzyEq(abs(look.dot(Vector3::unitY())), 1.0)) {
        // Looking straight up or down; lookAt won't work
        float dummy;
        float y, p;
        c.rotation.toEulerAnglesZYX(dummy, y, p);
        yaw = -y;
        pitch = -p;

    } else {
    */
        lookAt(c.translation + look);
//    }
}


void FirstPersonManipulator::onPose(Array<PosedModel::Ref>& p3d, Array<PosedModel2DRef>& p2d) {
}


void FirstPersonManipulator::onNetwork() {
}


void FirstPersonManipulator::onLogic() {
}


void FirstPersonManipulator::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    if (! m_active) {
        return;
    }

    if (m_userInput == NULL) {
        return;
    }

    RealTime elapsedTime = rdt;

    {
        // Translation direction
        Vector2 direction(m_userInput->getX(), m_userInput->getY());
        direction.unitize();

        // Translate forward
        m_translation += (lookVector() * direction.y + frame().rightVector() * direction.x) *
            elapsedTime * m_maxMoveRate;
    }
    
    // Desired change in yaw and pitch
    Vector2 delta;
    float maxTurn = m_maxTurnRate * elapsedTime;

    switch (m_mouseMode) {
    case MOUSE_DIRECT_RIGHT_BUTTON:
        {
            bool mouseDown = rightDown(m_userInput);
            m_userInput->setPureDeltaMouse(mouseDown);
            if (! mouseDown) {
                // Skip bottom case
                break;
            }
        }
        // Intentionally fall through to MOUSE_DIRECT

    case MOUSE_DIRECT:
        delta = m_userInput->mouseDXY() / 100.0f;
        break;


    case MOUSE_SCROLL_AT_EDGE:
        {
            Rect2D viewport = Rect2D::xywh(0, 0, m_userInput->window()->width(), m_userInput->window()->height());
            Vector2 mouse = m_userInput->mouseXY();

            Vector2 hotExtent(max(50.0f, viewport.width() / 8), 
                              max(50.0f, viewport.height() / 6));

            // The hot region is outside this rect
            Rect2D hotRegion = Rect2D::xyxy(
                viewport.x0() + hotExtent.x, viewport.y0() + hotExtent.y,
                viewport.x1() - hotExtent.y, viewport.y1() - hotExtent.y);

            // See if the mouse is near an edge
            if (mouse.x <= hotRegion.x0()) {
                delta.x = -square(1.0 - (mouse.x - viewport.x0()) / hotExtent.x);
                // - Yaw
            } else if (mouse.x >= hotRegion.x1()) {
                delta.x = square(1.0 - (viewport.x1() - mouse.x) / hotExtent.x);
                // + Yaw
            }

            if (mouse.y <= hotRegion.y0()) {
                delta.y = -square(1.0 - (mouse.y - viewport.y0()) / hotExtent.y) * 0.6;
                // - pitch
            } else if (mouse.y >= hotRegion.y1()) {
                delta.y = square(1.0 - (viewport.y1() - mouse.y) / hotExtent.y) * 0.6;
                // + pitch
            }

            delta *= maxTurn / 5;
        }
        break;

//    case MOUSE_PUSH_AT_EDGE: 
    default:
        debugAssert(false);
    }


    // Turn rate limiter
    if (G3D::abs(delta.x) > maxTurn) {
        delta.x = maxTurn * G3D::sign(delta.x);
    }

    if (G3D::abs(delta.y) > maxTurn) {
        delta.y = maxTurn * G3D::sign(delta.y);
    }

    m_yaw   += delta.x;
    m_pitch += delta.y;

    // As a patch for a setCoordinateFrame bug, we prevent 
    // the camera from looking exactly along the y-axis.
    m_pitch = clamp(m_pitch, (float)-halfPi() + 0.001f, (float)halfPi() - 0.001f);

    debugAssert(isFinite(m_yaw));
    debugAssert(isFinite(m_pitch));
}


void FirstPersonManipulator::onUserInput(UserInput* ui) {
    m_userInput = ui;
}


bool FirstPersonManipulator::onEvent(const GEvent& event) {
    return false;
}


}


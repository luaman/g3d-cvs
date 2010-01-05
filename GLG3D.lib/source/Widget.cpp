/**
 @file Widget.cpp

 @maintainer Morgan McGuire, morgan3d@users.sourceforge.net

 @created 2006-04-22
 @edited  2009-12-28
*/

#include "GLG3D/Widget.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GEvent.h"

namespace G3D {

void Widget::fireEvent(const GEvent& event) {
    m_manager->fireEvent(event);
}


OSWindow* Widget::window() const {
    if (m_manager == NULL) {
        return NULL;
    } else {
        return m_manager->window();
    }
}


OSWindow* WidgetManager::window() const {
    return m_window;
}


WidgetManager::Ref WidgetManager::create(OSWindow* window) {
    WidgetManager* m = new WidgetManager();
    m->m_window = window;
    return m;
}


void WidgetManager::fireEvent(const GEvent& event) {
    m_window->fireEvent(event);
}


WidgetManager::WidgetManager() : 
    m_locked(false) {
}


int WidgetManager::size() const {
    return m_moduleArray.size();
}


const Widget::Ref& WidgetManager::operator[](int i) const {
    return m_moduleArray[i];
}


void WidgetManager::beginLock() {
    debugAssert(! m_locked);
    m_locked = true;
}


void WidgetManager::endLock() {
    debugAssert(m_locked);
    m_locked = false;

    for (int e = 0; e < m_delayedEvent.size(); ++e) {
        DelayedEvent& event = m_delayedEvent[e];
        switch (event.type) {
        case DelayedEvent::REMOVE_ALL:
            clear();
            break;

        case DelayedEvent::REMOVE:
            remove(event.module);
            break;

        case DelayedEvent::ADD:
            add(event.module);
            break;
            
        case DelayedEvent::SET_FOCUS_AND_MOVE_TO_FRONT:
            setFocusedWidget(event.module, true);
            break;

        case DelayedEvent::SET_FOCUS:
            setFocusedWidget(event.module, false);
            break;

        case DelayedEvent::SET_DEFOCUS:
            defocusWidget(event.module);
            break;

        case DelayedEvent::MOVE_TO_BACK:
            moveWidgetToBack(event.module);
            break;
        }
    }

    m_delayedEvent.fastClear();
}


void WidgetManager::remove(const Widget::Ref& m) {
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::REMOVE, m));
    } else {
        if (m_focusedModule == m) {
            m_focusedModule = NULL;
        }
        int j = m_moduleArray.findIndex(m);
        if (j != -1) {
            m->setManager(NULL);
            m_moduleArray.remove(j);
            return;
        }
        debugAssertM(false, "Removed a Widget that was not in the manager.");
    }
}


bool WidgetManager::contains(const Widget::Ref& m) const {
    return m_moduleArray.contains(m);
}


void WidgetManager::add(const Widget::Ref& m) {
    debugAssert(m.notNull());
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::ADD, m));
    } else {
        // Do not add elements that already are in the manager
        if (! m_moduleArray.contains(m)) {
            if (m_focusedModule.notNull() && (m_focusedModule == m_moduleArray.last())) {
                // Cannot replace the focused module at the top of the priority list
                m_moduleArray[m_moduleArray.size() - 1] = m;
                m_moduleArray.append(m_focusedModule);
            } else {
                m_moduleArray.append(m);
            }
            m->setManager(this);
        }
    }
}


Widget::Ref WidgetManager::focusedWidget() const {
    return m_focusedModule;
}


void WidgetManager::moveWidgetToBack(const Widget::Ref& widget) {
   if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::MOVE_TO_BACK, widget));
   } else {
       int i = m_moduleArray.findIndex(widget);
       if (i > 0) {
           // Found and not already at the bottom
           m_moduleArray.remove(i);
           m_moduleArray.insert(0, widget);
       } 
   }
}


void WidgetManager::defocusWidget(const Widget::Ref& m) {
   if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_DEFOCUS, m));
   } else if (focusedWidget().pointer() == m.pointer()) {
       setFocusedWidget(NULL);
   }    
}


void WidgetManager::setFocusedWidget(const Widget::Ref& m, bool moveToFront) {
    if (m_locked) {
        if (moveToFront) {
            m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_FOCUS_AND_MOVE_TO_FRONT, m));
        } else {
            m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_FOCUS, m));
        }
    } else {

        debugAssert(m.isNull() || m_moduleArray.contains(m));

        if (m.notNull() && moveToFront) {
            // Move to the first event position
            int i = m_moduleArray.findIndex(m);
            m_moduleArray.remove(i);
            m_moduleArray.append(m);
        }

        m_focusedModule = m;
    }
}


void WidgetManager::clear() {
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::REMOVE_ALL));
    } else {
        m_moduleArray.clear();
        m_focusedModule = NULL;
    }
}

// Iterate through all modules in priority order.
// This same iteration code is used to implement
// all Widget methods concisely.
#define ITERATOR(body)\
    beginLock();\
        for (int i = m_moduleArray.size() - 1; i >=0; --i) {\
            body;\
        }\
    endLock();


void WidgetManager::onPose(
    Array<Surface::Ref>& posedArray, 
    Array<Surface2DRef>& posed2DArray) {

    beginLock();
    for (int i = 0; i < m_moduleArray.size(); ++i) {
        m_moduleArray[i]->onPose(posedArray, posed2DArray);
    }
    endLock();

}


void WidgetManager::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    ITERATOR(m_moduleArray[i]->onSimulation(rdt, sdt, idt));
}


bool WidgetManager::onEvent(const GEvent& event) {
    const bool motionEvent = 
        (event.type == GEventType::MOUSE_MOTION) ||
        (event.type == GEventType::JOY_AXIS_MOTION) ||
        (event.type == GEventType::JOY_HAT_MOTION) ||
        (event.type == GEventType::JOY_BALL_MOTION);

    beginLock();
    {

        // Except for motion events, ensure that the focused module gets each event first
        if (m_focusedModule.notNull() && ! motionEvent) {
            if (m_focusedModule->onEvent(event)) { 
                endLock(); 
                return true;
            }
        }

        for (int i = m_moduleArray.size() - 1; i >=0; --i) {
            // Don't double-deliver to the focused module
            if (motionEvent || (m_moduleArray[i] != m_focusedModule)) {
                if (m_moduleArray[i]->onEvent(event) && ! motionEvent) {
                    endLock(); 
                    return true;
                }
            }
        }
    }
    endLock();

    return false;
}


void WidgetManager::onUserInput(UserInput* ui) {
    ITERATOR(m_moduleArray[i]->onUserInput(ui));
}


void WidgetManager::onNetwork() {
    ITERATOR(m_moduleArray[i]->onNetwork());
}

void WidgetManager::onAI() {
    ITERATOR(m_moduleArray[i]->onAI());
}

#undef ITERATOR

bool WidgetManager::onEvent(const GEvent& event, 
                             WidgetManager::Ref& a) {
    static WidgetManager::Ref x(NULL);
    return onEvent(event, a, x);
}


bool WidgetManager::onEvent(const GEvent& event, WidgetManager::Ref& a, WidgetManager::Ref& b) {
    a->beginLock();
    if (b.notNull()) {
        b->beginLock();
    }

    int numManagers = (b.isNull()) ? 1 : 2;

    // Process each
    for (int k = 0; k < numManagers; ++k) {
        Array<Widget::Ref>& array = 
            (k == 0) ?
            a->m_moduleArray :
            b->m_moduleArray;
                
        for (int i = array.size() - 1; i >= 0; --i) {

            debugAssertGLOk();
            if (array[i]->onEvent(event)) {
                debugAssertGLOk();
                if (b.notNull()) {
                    b->endLock();
                }
                a->endLock();
                debugAssertGLOk();
                return true;
            }
        }
        debugAssertGLOk();
    }
    
    if (b.notNull()) {
        b->endLock();
    }
    a->endLock();

    debugAssertGLOk();

    return false;
}


} // G3D

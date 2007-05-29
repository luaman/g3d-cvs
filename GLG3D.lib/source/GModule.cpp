/**
 @file GModule.cpp

 @maintainer Morgan McGuire, morgan3d@users.sourceforge.net

 @created 2006-04-22
 @edited  2007-05-22
*/

#include "GLG3D/GModule.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {


GModuleManagerRef GModuleManager::create() {
    return new GModuleManager();
}


GModuleManager::GModuleManager() : 
    m_locked(false) {
}


int GModuleManager::size() const {
    return m_moduleArray.size();
}


const GModuleRef& GModuleManager::operator[](int i) const {
    return m_moduleArray[i];
}


void GModuleManager::beginLock() {
    debugAssert(! m_locked);
    m_locked = true;
}


void GModuleManager::endLock() {
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
            
        case DelayedEvent::SET_FOCUS:
            setFocusedModule(event.module);
            break;

        case DelayedEvent::SET_DEFOCUS:
            setDefocusedModule(event.module);
            break;
        }
    }

    m_delayedEvent.fastClear();
}


void GModuleManager::remove(const GModuleRef& m) {
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
        debugAssertM(false, "Removed a GModule that was not in the manager.");
    }
}


void GModuleManager::add(const GModuleRef& m) {
    debugAssert(m.notNull());
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::ADD, m));
    } else {
        if (m_focusedModule.notNull()) {
            // Cannot replace the focused module at the top of the priority list
            m_moduleArray[m_moduleArray.size() - 1] = m;
            m_moduleArray.append(m_focusedModule);
        } else {
            m_moduleArray.append(m);
        }
        m->setManager(this);
    }
}


GModuleRef GModuleManager::focusedModule() const {
    return m_focusedModule;
}


void GModuleManager::setDefocusedModule(const GModuleRef& m) {
   if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_DEFOCUS, m));
   } else if (focusedModule().pointer() == m.pointer()) {
       setFocusedModule(NULL);
   }    
}


void GModuleManager::setFocusedModule(const GModuleRef& m) {
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_FOCUS, m));
    } else {

        debugAssert(m.isNull() || m_moduleArray.contains(m));

        if (m.notNull()) {
            // Move to the first event position
            int i = m_moduleArray.findIndex(m);
            m_moduleArray.remove(i);
            m_moduleArray.append(m);
        }

        m_focusedModule = m;
    }
}


void GModuleManager::clear() {
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::REMOVE_ALL));
    } else {
        m_moduleArray.clear();
        m_focusedModule = NULL;
    }
}

// Iterate through all modules in priority order.
// This same iteration code is used to implement
// all GModule methods concisely.
#define ITERATOR(body)\
    beginLock();\
        for (int i = m_moduleArray.size() - 1; i >=0; --i) {\
            body;\
        }\
    endLock();


void GModuleManager::getPosedModel(
    Array<PosedModel::Ref>& posedArray, 
    Array<PosedModel2DRef>& posed2DArray) {

    beginLock();
    for (int i = 0; i < m_moduleArray.size(); ++i) {
        m_moduleArray[i]->getPosedModel(posedArray, posed2DArray);
    }
    endLock();

}


void GModuleManager::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    ITERATOR(m_moduleArray[i]->onSimulation(rdt, sdt, idt));
}

bool GModuleManager::onEvent(const GEvent& event) {
    bool motionEvent = 
        (event.type == GEventType::MOUSEMOTION) ||
        (event.type == JOYAXISMOTION) ||
        (event.type == JOYHATMOTION);

    // if the event is ever consumed, abort iteration
    ITERATOR(if (m_moduleArray[i]->onEvent(event) && ! motionEvent) { endLock(); return true; });
    return false;
}

void GModuleManager::onUserInput(UserInput* ui) {
    ITERATOR(m_moduleArray[i]->onUserInput(ui));
}

void GModuleManager::onNetwork() {
    ITERATOR(m_moduleArray[i]->onNetwork());
}

void GModuleManager::onLogic() {
    ITERATOR(m_moduleArray[i]->onLogic());
}

#undef ITERATOR

bool GModuleManager::onEvent(const GEvent& event, 
                             GModuleManagerRef& a) {
    static GModuleManagerRef x(NULL);
    return onEvent(event, a, x);
}


bool GModuleManager::onEvent(const GEvent& event, GModuleManagerRef& a, GModuleManagerRef& b) {
    a->beginLock();
    if (b.notNull()) {
        b->beginLock();
    }

    int numManagers = (b.isNull()) ? 1 : 2;

    // Process each
    for (int k = 0; k < numManagers; ++k) {
        Array<GModuleRef>& array = 
            (k == 0) ?
            a->m_moduleArray :
            b->m_moduleArray;
                
        for (int i = array.size() - 1; i >= 0; --i) {
            if (array[i]->onEvent(event)) {
                if (b.notNull()) {
                    b->endLock();
                }
                
                a->endLock();
                return true;
            }
        }
    }
    
    if (b.notNull()) {
        b->endLock();
    }
    a->endLock();

    return false;
}


} // G3D

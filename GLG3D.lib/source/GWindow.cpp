/**
 @file GWindow.cpp
  
 @maintainer Morgan McGuire, matrix@graphics3d.com
 
 @created 2004-11-16
 @edited  2007-03-16
 */

#include "GLG3D/GWindow.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GLCaps.h"
#ifdef G3D_WIN32
#    include "GLG3D/Win32Window.h"
#elif defined(G3D_OSX)
#    include "GLG3D/CarbonWindow.h"
#else
#    include "GLG3D/SDLWindow.h"
#endif

namespace G3D {

GWindow* GWindow::create(const GWindow::Settings& s) {
#   ifdef G3D_WIN32
        return Win32Window::create(s);
#   elif defined(G3D_OSX)
        return CarbonWindow::create(s);
#   else
        return SDLWindow::create(s);
#   endif
}

const GWindow* GWindow::m_current = NULL;

void GWindow::fireEvent(const GEvent& event) {
    m_eventQueue.pushBack(event);
}


bool GWindow::pollOSEvent(GEvent& e) {
    (void)e;
    return false;
}


bool GWindow::pollEvent(GEvent& e) {
    // Extract all pending events and put them on the queue.

    while (pollOSEvent(e) != 0) {
        m_eventQueue.pushBack(e);
    }

    // Return the first pending event
    if (m_eventQueue.size() > 0) {
        e = m_eventQueue.popFront();
        return true;
    } else {
        return false;
    }
}

void GWindow::executeLoopBody() {
    if (notDone()) {
        if (loopBodyStack.last().isGApp) {
            loopBodyStack.last().app->oneFrame();
        } else {                
            loopBodyStack.last().func(loopBodyStack.last().arg);
        }
    }
}


void GWindow::pushLoopBody(GApp* app) {
    loopBodyStack.push(LoopBody(app));
    app->beginRun();
}


void GWindow::popLoopBody() {
    if (loopBodyStack.size() > 0) {
        if (loopBodyStack.last().isGApp) {
            loopBodyStack.last().app->endRun();
        }
        loopBodyStack.pop();
    }
}

}

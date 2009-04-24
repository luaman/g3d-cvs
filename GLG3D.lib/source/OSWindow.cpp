/**
 @file OSWindow.cpp
  
 @maintainer Morgan McGuire, morgan@cs.williams.edu
 
 @created 2004-11-16
 @edited  2007-03-16
 */

#include "GLG3D/OSWindow.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"
#ifdef G3D_WIN32
#    include "GLG3D/Win32Window.h"
#elif defined(G3D_OSX)
#    include "GLG3D/CarbonWindow.h"
#else
#    include "GLG3D/SDLWindow.h"
#endif

namespace G3D {

OSWindow* OSWindow::create(const OSWindow::Settings& s) {
#   ifdef G3D_WIN32
        return Win32Window::create(s);
#   elif defined(G3D_OSX)
        return CarbonWindow::create(s);
#   else
        return SDLWindow::create(s);
#   endif
}

const OSWindow* OSWindow::m_current = NULL;

void OSWindow::handleResize(int width, int height) {
    // update settings
    m_settings.width = width;
    m_settings.height = height;

    // update viewport
    Rect2D newViewport = Rect2D::xywh(0, 0, width, height);
    m_renderDevice->setViewport(newViewport);

    // force swap buffers
    m_renderDevice->swapBuffers();
}

void OSWindow::fireEvent(const GEvent& event) {
    m_eventQueue.pushBack(event);
}


void OSWindow::getOSEvents(Queue<GEvent>& events) {
    // no events added
    (void)events;
}


bool OSWindow::pollEvent(GEvent& e) {

    // Extract all pending events and put them on the queue.
    getOSEvents(m_eventQueue);

    // Return the first pending event
    if (m_eventQueue.size() > 0) {
        e = m_eventQueue.popFront();
        return true;
    } else {
        return false;
    }
}

void OSWindow::executeLoopBody() {
    if (notDone()) {
        if (m_loopBodyStack.last().isGApp) {
            m_loopBodyStack.last().app->oneFrame();
        } else {                
            m_loopBodyStack.last().func(m_loopBodyStack.last().arg);
        }
    }
}


void OSWindow::pushLoopBody(GApp* app) {
    m_loopBodyStack.push(LoopBody(app));
    app->beginRun();
}


void OSWindow::popLoopBody() {
    if (m_loopBodyStack.size() > 0) {
        if (m_loopBodyStack.last().isGApp) {
            m_loopBodyStack.last().app->endRun();
        }
        m_loopBodyStack.pop();
    }
}

}

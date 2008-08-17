/**
   @file FontViewer.cpp
 
   Viewer for supported video files.
 
*/
#include "VideoViewer.h"


VideoViewer::VideoViewer() {
}


void VideoViewer::onInit(const std::string& filename) {

    m_video = VideoInput::fromFile(filename, VideoInput::Settings());
}

void VideoViewer::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    if (m_video.notNull() && !m_video->finished()) {
        m_video->readNext(rdt, m_videoTexture);
    }
}


void VideoViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {

    // set clear color
    app->colorClear = Color3::white();

    rd->push2D();

    Rect2D windowBounds = rd->viewport();

    rd->setTexture(0, m_videoTexture);
    Draw::fastRect2D(windowBounds, rd);

    if (m_video.notNull()) {
        screenPrintf("Video: %d x %d", m_video->width(), m_video->height());
    } else {
        screenPrintf("Video: not supported");
    }

    screenPrintf("Window: %d x %d", rd->width(), rd->height());

    rd->pop2D();
}

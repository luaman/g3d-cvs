/**
 @file FontViewer.h
 
 Viewer for supported video files.
 
*/
#ifndef VIDEOVIEWER_H
#define VIDEOVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class VideoViewer : public Viewer {
private:

    VideoInput::Ref m_video;
    Texture::Ref    m_videoTexture;

public:
	VideoViewer();
	virtual void onInit(const std::string& filename);
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif 

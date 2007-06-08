/**
 @file SkyViewer.h
 
 Viewer for image files that correspond to a "sky" cube map
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef SKYVIEWER_H
#define SKYVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class SkyViewer : public Viewer {
private:
    SkyParameters       skyParameters;
    SkyRef              sky;

public:
	SkyViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app);

};

#endif 

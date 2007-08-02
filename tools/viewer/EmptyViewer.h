/**
 @file EmptyViewer.h
 
 If the person tried to load a file that wasn't valid, this default viewer is displayed
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef EMPTYVIEWER_H
#define EMPTYVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class EmptyViewer : public Viewer {

public:
	EmptyViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif 

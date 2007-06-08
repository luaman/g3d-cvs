/**
 @file Viewer.h
 
 The base class for the more specialized Viewers
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef VIEWER_H
#define VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

class Viewer {
public:
	virtual ~Viewer() {}
	virtual void onInit(const std::string& filename) = 0;
    virtual void onGraphics(RenderDevice* rd, App* app) = 0;

};

#endif 

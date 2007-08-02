/**
 @file BSPViewer.h
 
 Viewer for Quake3 .bsp maps
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef BSPVIEWER_H
#define BSPVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class BSPViewer : public Viewer {
private:
    BSPMapRef				map;
	bool					reset;
	int						numVertices;
	int						numFaces;
	int						numTextures; 
	int						numLightMaps;

public:
	BSPViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif 

/**
 @file MD2Viewer.h
 
 Viewer for Quake2 .md2 models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef MD2VIEWER_H
#define MD2VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class MD2Viewer : public Viewer {
private:
    Array<MD2ModelRef>			models;
	Array<PosedModelRef>		posed;
	Array<GMaterial>			materials;
	MD2Model::Pose				currentPose;

	bool						validWeapon;
	std::string					textureName;
	int							numEdges;
	int							numFaces;
	int							numVertices;

	void pose(RealTime deltaTime);

public:
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);
};

#endif 

/**
 @file MD2Viewer.h
 
 Viewer for Quake2 .md2 models
 
 \maintainer Morgan McGuire
 @author Eric Muller, Dan Fast, Katie Creel
 
 @created 2007-05-31
 @edited  2010-04-19
 */
#ifndef MD2VIEWER_H
#define MD2VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class MD2Viewer : public Viewer {
private:
    MD2Model::Ref               model;
    Array<Surface::Ref>         posed;
    MD2Model::Pose              currentPose;

    int							numEdges;
    int							numFaces;
    int							numVertices;
    
    void pose(RealTime deltaTime);

public:
    virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting);
};

#endif 

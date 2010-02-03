/**
 @file MD2Viewer.h
 
 Viewer for Quake2 .md2 models
 
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2010-02-08
 */
#ifndef MD2VIEWER_H
#define MD2VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class MD2Viewer : public Viewer {
private:
    Array<MD2Model::Ref>        models;
    Array<Surface::Ref>         posed;
    Array<Material::Ref>        materials;
    MD2Model::Pose              currentPose;

    bool						validWeapon;
    std::string					textureName;
    int							numEdges;
    int							numFaces;
    int							numVertices;
    
    void pose(RealTime deltaTime);

public:
    virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting);
};

#endif 

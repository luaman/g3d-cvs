/**
 @file MD2Viewer.cpp
 
 Viewer for Quake2 .md2 models
 
 @maintainer  Morgan McGuire
 @author Eric Muller, Dan Fast, Katie Creel
 
 @created 2007-05-31
 @edited  2010-04-19
 */
#include "MD2Viewer.h"


void MD2Viewer::pose(RealTime dt){

    currentPose.onSimulation(dt, MD2Model::Pose::Action());

    const CoordinateFrame& cframe = CFrame::fromXYZYPRDegrees(0,0,3,180);

	posed.fastClear();
	model->pose(posed, cframe, currentPose);
}


void MD2Viewer::onInit(const std::string& filename) {
	
    model = MD2Model::create(filename);

	currentPose = MD2Model::Pose::Pose(MD2Model::STAND, 0);
}


void MD2Viewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    app->colorClear = Color3::white();
	screenPrintf("Triangles: %d", model->numTriangles());
	screenPrintf("Current animation: %d", currentPose.animation);

	pose(app->desiredFrameDuration());

    rd->enableLighting();
    rd->setAmbientLightColor(Color3::white());

	for(int p = 0; p < posed.size(); ++p){
		posed[p]->render(rd);
	}
	
}

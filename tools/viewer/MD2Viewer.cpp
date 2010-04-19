/**
 @file MD2Viewer.cpp
 
 Viewer for Quake2 .md2 models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "MD2Viewer.h"


void MD2Viewer::pose(RealTime dt){

    currentPose.onSimulation(dt, MD2Model::Pose::Action());

	CoordinateFrame cframe(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180)));

	posed.clear();
	for(int m = 0; m < models.size(); ++m){
		models[m]->pose(posed, cframe, currentPose);
	}
}


void MD2Viewer::onInit(const std::string& filename) {
	
	std::string resourceDir = filenamePath(filename);

	//Character texture handling
	Array<std::string> files;
	std::string desiredTexture1, desiredTexture2;

	// By convention, base .md2 models are named tris.md2.
	// Weapons are either named weapon.md2 or w_(name).md2.
	// First, we test to see whether a weapon has been loaded
	// and find its corresponding texture.
	if ("w" == toLower(filenameBaseExt(filename)).substr(0,1)) {
		std::string textureTest = toLower(filenameBaseExt(filename));
		int remove = (int) filenameExt(filename).length();
		textureTest = textureTest.substr(0, textureTest.length() - remove);
		textureTest = textureTest + "pcx";
		if (Texture::isSupportedImage(resourceDir + textureTest)) {
			desiredTexture1 = resourceDir + textureTest;
		}
		validWeapon = false;
	} else {
		FileSystem::getFiles(resourceDir + "*.pcx", files, true);
		FileSystem::getFiles(resourceDir + "*.jpg", files, true);
		FileSystem::getFiles(resourceDir + "*.tga", files, true);
		FileSystem::getFiles(resourceDir + "*.png", files, true);
		for(int f = 0; f < files.length(); ++f){
            if (FileSystem::size(desiredTexture1) < FileSystem::size(files[f])){
				if (beginsWith(filenameBaseExt(files[f]), "w_") || beginsWith(filenameBaseExt(files[f]), "weapon")){
					//desired texture is unchanged
				} else {
					desiredTexture1 = files[f];
				}
			}
		}
        if (FileSystem::exists(resourceDir + "weapon.md2") && (filename != (resourceDir + "weapon.md2"))) {
			validWeapon = true;
		} else {
			validWeapon = false;
		}
	}
	

	if (FileSystem::exists(resourceDir + "weapon.pcx") && validWeapon){
        desiredTexture2 = resourceDir + "weapon.pcx";
    }

    //the equivalent of tris.md2 is stored in filename
	models.push(MD2Model::fromFile(filename, desiredTexture1));
    if (validWeapon) {
	    models.push(MD2Model::fromFile(resourceDir + "weapon.pcx", desiredTexture2));
    }

	textureName = filenameBaseExt(desiredTexture1);
	numEdges = models[0]->edges().length();
	numFaces = models[0]->faces().length();
	numVertices = models[0]->vertices().length();

	currentPose = MD2Model::Pose::Pose(MD2Model::STAND, 0);
}


void MD2Viewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    app->colorClear = Color3::white();
	screenPrintf("Edges: %d", numEdges);
	screenPrintf("Faces: %d", numFaces);
	screenPrintf("Vertices: %d", numVertices);
	screenPrintf("Current texture: %s", textureName.c_str());
	screenPrintf("Current animation: %d", currentPose.animation);

	if (validWeapon){
		screenPrintf("Weapon found.");
	}

	pose(app->desiredFrameDuration());

	for(int p = 0; p < posed.size(); ++p){
		posed[p]->render(rd);
	}
	
}

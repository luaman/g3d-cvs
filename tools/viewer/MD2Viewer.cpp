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

	currentPose.doSimulation(dt, false, false, false, false, false,
								false, false, false, false, false, false, false,
								false, false, false, false);

	CoordinateFrame cframe(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180)));

	posed.clear();
	for(int m = 0; m < models.size(); ++m){
		posed.push(models[m]->pose(cframe, currentPose, materials[m]));
	}
}


void MD2Viewer::onInit(const std::string& filename) {
	
	//the equivalent of tris.md2 is stored in filename
	models.push(MD2Model::fromFile(filename));
	materials.push(GMaterial());

	std::string resourceDir = filenamePath(filename);

	Texture::PreProcess p;
	p.brighten = 2.0;

	//Character texture handling
	Array<std::string> files;
	std::string desiredTexture;
	if ((toLower(filenameBaseExt(filename)) == "weapon.md2") &&
		fileExists(resourceDir + "weapon.pcx")) {

		// Loading just the weapon
		desiredTexture = resourceDir + "weapon.pcx";
	} else {
		getFiles(resourceDir + "*.pcx", files, true);
		getFiles(resourceDir + "*.jpg", files, true);
		getFiles(resourceDir + "*.tga", files, true);
		getFiles(resourceDir + "*.png", files, true);
		for(int f = 0; f < files.length(); ++f){
			if(fileLength(desiredTexture) < fileLength(files[f])){
				if(beginsWith(filenameBaseExt(files[f]), "w_")||beginsWith(filenameBaseExt(files[f]), "weapon")){
					//desired texture is unchanged
				} else {
					desiredTexture = files[f];
				}
			}
		}
	}

	if (desiredTexture != "") {
        p.brighten = endsWith(toLower(desiredTexture), ".pcx") ? 2.0 : 1.0;
		materials[0].texture.push(Texture::fromFile(desiredTexture,
													TextureFormat::AUTO(),
													Texture::DIM_2D,
													Texture::Settings::defaults(),
													p));
	}
	
	// If there is a weapon.md2 file and that wasn't the file that was explicitly 
	// requested, load it as well.
	validWeapon = fileExists(resourceDir + "weapon.md2") && 
					   (filename != (resourceDir + "weapon.md2"));
	if (validWeapon){
		models.push(MD2Model::fromFile(resourceDir + "weapon.md2"));
		materials.push(GMaterial());
	}

	if(fileExists(resourceDir + "weapon.pcx") && validWeapon){
        p.brighten = 2.0;
		materials[1].texture.push(Texture::fromFile(resourceDir + "weapon.pcx",
													TextureFormat::AUTO(),
													Texture::DIM_2D,
													Texture::Settings::defaults(),
													p));
    }

	textureName = filenameBaseExt(desiredTexture);
	numEdges = models[0]->edges().length();
	numFaces = models[0]->faces().length();
	numVertices = models[0]->vertices().length();

	currentPose = MD2Model::Pose::Pose(MD2Model::STAND, 0);
}


void MD2Viewer::onGraphics(RenderDevice* rd, App* app) {
    app->colorClear = Color3::white();
	app->debugPrintf("Edges: %d", numEdges);
	app->debugPrintf("Faces: %d", numFaces);
	app->debugPrintf("Vertices: %d", numVertices);
	app->debugPrintf("Current texture: %s", textureName.c_str());
	app->debugPrintf("Current animation: %d", currentPose.animation);
	if(validWeapon){
		app->debugPrintf("Weapon found.");
	}

	pose(app->desiredFrameDuration());

	for(int p = 0; p < posed.size(); ++p){
		posed[p]->render(rd);
	}
	
}

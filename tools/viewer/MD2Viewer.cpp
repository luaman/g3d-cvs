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
		posed.push(models[m]->pose(cframe, currentPose, materials[m]));
	}
}


void MD2Viewer::onInit(const std::string& filename) {
	
	//the equivalent of tris.md2 is stored in filename
	models.push(MD2Model::fromFile(filename));
    materials.push(Material::createDiffuse(Color3::white()));

	std::string resourceDir = filenamePath(filename);

	Texture::Preprocess p;
    p.modulate = Color4::one() * 2.0f;

	//Character texture handling
	Array<std::string> files;
	std::string desiredTexture;

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
			desiredTexture = resourceDir + textureTest;
		}
		validWeapon = false;
	} else {
		FileSystem::getFiles(resourceDir + "*.pcx", files, true);
		FileSystem::getFiles(resourceDir + "*.jpg", files, true);
		FileSystem::getFiles(resourceDir + "*.tga", files, true);
		FileSystem::getFiles(resourceDir + "*.png", files, true);
		for(int f = 0; f < files.length(); ++f){
            if (FileSystem::size(desiredTexture) < FileSystem::size(files[f])){
				if (beginsWith(filenameBaseExt(files[f]), "w_") || beginsWith(filenameBaseExt(files[f]), "weapon")){
					//desired texture is unchanged
				} else {
					desiredTexture = files[f];
				}
			}
		}
        if (FileSystem::exists(resourceDir + "weapon.md2") && (filename != (resourceDir + "weapon.md2"))) {
			validWeapon = true;
		} else {
			validWeapon = false;
		}
	}


    if (desiredTexture != "") {
        Any lamb(Any::TABLE, "Texture::Specification");
        lamb["filename"] = desiredTexture;
        if (endsWith(toLower(desiredTexture), ".pcx")) {
            lamb["preprocess"] = Any(Any::ARRAY, "Texture::PreProcess::quake");
        }
        Any anySpec(Any::TABLE, "Material::Specification");
        anySpec["lambertian"] = lamb;
		materials[0] = Material::create(anySpec);
	}
	
	// If there is a weapon.md2 file and that wasn't the file that was explicitly 
	// requested, load it as well.  validWeapon is set when the primary file is
	// loaded, above
	if (validWeapon){
		models.push(MD2Model::fromFile(resourceDir + "weapon.md2"));
        materials.push(Material::createDiffuse(Color3::white()));
	}

	if (FileSystem::exists(resourceDir + "weapon.pcx") && validWeapon){
        Any lamb(Any::TABLE, "Texture::Specification");
        lamb["filename"] = resourceDir + "weapon.pcx";
        lamb["preprocess"] = Any(Any::ARRAY, "Texture::PreProcess::quake");
        Any anySpec(Any::TABLE, "Material::Specification");
        anySpec["lambertian"] = lamb;
        materials[1] = Material::create(anySpec);
    }

	textureName = filenameBaseExt(desiredTexture);
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

/**
 @file BSPViewer.cpp
 
 Viewer for Quake3 .bsp maps
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "BSPViewer.h"


BSPViewer::BSPViewer():reset(false){}


void BSPViewer::onInit(const std::string& filename) {
	// filename has the form resourceDir/maps/file.bsp
	std::string path = filenamePath(filename);
	std::string mapName = filenameBaseExt(filename);

	std::string resourceDir = path + "../";

	map = BSPMap::fromFile(resourceDir, mapName);

	Array<Vector3> vertexArray, normalArray;
	Array<Vector2> texCoordArray, lightCoordArray;
	Array<TextureRef> texMapArray, lightMapArray;
	Array<int> indArray, texMapIndexArray, lightMapIndexArray, texCoordIndexArray;
	map->getTriangles(vertexArray, normalArray, indArray, texCoordArray,
						texMapIndexArray, lightCoordArray, lightMapIndexArray, 
						texCoordIndexArray, texMapArray, lightMapArray);

	numFaces = indArray.length()/3;
	numVertices = vertexArray.length();
	numTextures = texMapArray.length();
	numLightMaps = lightMapArray.length();

	//So that we can reset the camera to the map's
	//default starting position the next time
	//we render
	reset = true;
}


void BSPViewer::onGraphics(RenderDevice* rd, App* app) {
	if(reset == true){
		reset = false;
		app->defaultCamera.setCoordinateFrame(map->startingPosition);
		app->defaultController->setFrame(map->startingPosition);
	}

	app->debugPrintf("Faces: %d", numFaces);
	app->debugPrintf("Vertices: %d", numVertices);
	app->debugPrintf("Light Maps: %d", numLightMaps);
	app->debugPrintf("Textures: %d", numTextures);
	Vector3 camPos = app->defaultCamera.getCoordinateFrame().translation;
	app->debugPrintf("Current camera position: (%f, %f, %f)", camPos.x, camPos.y, camPos.z);

	map->render(rd, app->defaultCamera);
	
}

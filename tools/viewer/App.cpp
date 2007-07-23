/**
 @file App.cpp
 
 App that allows viewing of 3D assets
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-07-08
 */
#include "App.h"
#include "IFSViewer.h"
#include "ArticulatedViewer.h"
#include "TextureViewer.h"
#include "FontViewer.h"
#include "SkyViewer.h"
#include "BSPViewer.h"
#include "MD2Viewer.h"
#include "GUIViewer.h"
#include "EmptyViewer.h"


App::App(const GApp::Settings& settings, const std::string& file) :
    GApp(settings),
	filename(file),
	viewer(NULL) {

	setDesiredFrameRate(60);
}


void App::onInit() {
	showRenderingStats = false;
	window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");

    sky = Sky::fromFile(dataDir + "sky/");
    skyParameters = SkyParameters( G3D::toSeconds(11, 00, 00, AM) );
    lighting = Lighting::fromSky( sky, skyParameters, Color3::white() );
    lighting->lightArray.append( lighting->shadowedLightArray );
    lighting->shadowedLightArray.clear();

    toneMap->setEnabled(false);
	
	colorClear = Color3::white();
	//modelController = ThirdPersonManipulator::create();

	setViewer(filename);
}


void App::onCleanup() {
	delete viewer;
	viewer = NULL;
}


bool App::onEvent(const GEvent& e) {
	switch (e.type) {
    case GEventType::FILE_DROP:
		{
		   Array<std::string> fileArray;
		   window()->getDroppedFilenames(fileArray);
		   setViewer(fileArray[0]);
		   return true;
		}

	default:;
    }

	return false;
}


void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {

    LightingRef localLighting = toneMap->prepareLighting(lighting);
	toneMap->setEnabled(false);
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(colorClear);
    rd->clear(true, true, true);

    int width = rd->width();
    
    rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

		// Render the file that is currently being viewed
		if (viewer != NULL) {
			viewer->onGraphics(rd, this);
		}
	
        PosedModel2D::sortAndRender(rd, posed2D);
	rd->disableLighting();
}

/** Must all be lower case */
static const std::string cubeMapExtension[] = {"up", "dn", "bk", "rt", "lf", "ft"};

static bool allCubeMapFacesExist(const std::string& base, const std::string& ext) {
	for (int f = 0; f < 6; ++f) {
		if (! fileExists(base + cubeMapExtension[f] + "." + ext) &&
			! fileExists(base + toUpper(cubeMapExtension[f]) + "." + ext)) {
 			return false;
		}
	}
	return true;
}

/**
 @param ext Does not contain the period, must be an image extension
 */
static bool isCubeMapFace(const std::string& _base, const std::string& _ext) {
	std::string base = toLower(_base);
	std::string ext = toLower(_ext);
	for (int f = 0; f < 6; ++f) {
		if (endsWith(base, cubeMapExtension[f] + "." + ext)) {
			return true;
		}
	}
	return false;
}


void App::setViewer(const std::string& newFilename) {
	filename = newFilename;
	defaultCamera.setCoordinateFrame(CoordinateFrame(Vector3(0,0,5)));
	defaultController->setFrame(CoordinateFrame(Vector3(0,0,5)));
	//modelController->setFrame(CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180))));
	delete viewer;
	viewer = NULL;
	
	std::string ext = toLower(filenameExt(filename));

	if (ext == "ifs") {

		viewer = new IFSViewer();

	} else if (ext == "3ds") {

		viewer = new ArticulatedViewer();

	} else if (Texture::isSupportedImage(filename)) {

		// Images can be either a Texture or a Sky: figure out which we have
	
		// If the image represents a skybox, there will be 5 others just like it,
		// corresponding to the rest of the cube
		if (isCubeMapFace(filename, ext)) {

			std::string skyRoot = filename.substr(0, filename.length() - 6);

			if (allCubeMapFacesExist(skyRoot, ext)) {
				viewer = new SkyViewer();
			} else {
				viewer = new TextureViewer();
			}
		} else {
			viewer = new TextureViewer();
		}
	} else if (ext == "fnt") {

		viewer = new FontViewer(debugFont);

	} else if (ext == "bsp") {

		viewer = new BSPViewer();

	} else if (ext == "md2") {

		viewer = new MD2Viewer();

	} else if (ext == "skn") {

		viewer = new GUIViewer(this);

	} else if (ext == "pk3") {
		// Something in Quake format - figure out what we should load
		Array <std::string> files;
		bool set = false;

		// First, try for a .bsp map
		std::string search = filename + "/maps/*";
		getFiles(search, files, true);
		for (int t = 0; t < files.length(); ++t) {

			if (filenameExt(files[t]) == "bsp") {

				filename = files[t];
				viewer = new BSPViewer();
				set = true;
			}
		}
		if (!set) {
			viewer = new EmptyViewer();
		}
	} else {

		viewer = new EmptyViewer();
	
	}

	if (viewer != NULL) {
		viewer->onInit(filename);
	}

	window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");
}

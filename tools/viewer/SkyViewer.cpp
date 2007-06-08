/**
 @file SkyViewer.cpp
 
 Viewer for image files that correspond to a "sky" cube map
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "SkyViewer.h"


SkyViewer::SkyViewer()
{}


void SkyViewer::onInit(const std::string& filename) {

	//sky fileloading uses a wildcard that loads a general string of it.
	//we need to manipulate the string in order to find its format and if it begins with a
	//word like up. 
	int test = (int)filename.find("sky/");

	std::string mainDir = filename.substr(0, test + 4);

	std::string subDir = filename.substr(test + 4);
	subDir = subDir.substr(0, subDir.find("/") + 1);
	
	std::string appendation = filename.substr(subDir.length() + mainDir.length());	
	appendation = appendation.substr(0, (appendation.length() - 6));

	appendation += ("*." + filenameExt(filename));

	sky = Sky::fromFile(mainDir + subDir, appendation, false);
	
    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
}


void SkyViewer::onGraphics(RenderDevice* rd, App* app) {
	rd->disableLighting();
	sky->render(rd, skyParameters);
}

/**
 @file EmptyViewer.cpp
 
 If the person tried to load a file that wasn't valid, this default viewer is displayed
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "EmptyViewer.h"


EmptyViewer::EmptyViewer(){}


void EmptyViewer::onInit(const std::string& filename) {}


void EmptyViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
	screenPrintf("No valid item to view currently\n");
	screenPrintf("Drag and drop an item to view it in 3D\n");
	screenPrintf("\n");
	screenPrintf("Supported files: ICO, JPG, BMP, PCX, DDS, TGA, PPM, PNG, GIF, PK3, MD2, BSP, FNT, 3DS, IFS, SKN");
}

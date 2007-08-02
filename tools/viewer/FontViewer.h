/**
 @file FontViewer.h
 
 Viewer for .fnt files, with a default font displayed for comparison
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef FONTVIEWER_H
#define FONTVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class FontViewer : public Viewer {
private:

	std::string		m_fontfilename;
	GFontRef		m_font;
	GFontRef		m_basefont;

public:
	FontViewer(GFontRef base);
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif 

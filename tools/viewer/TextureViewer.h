/**
 @file TextureViewer.h
 
 Viewer for image files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef TEXTUREVIEWER_H
#define TEXTUREVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class TextureViewer : public Viewer {
private:

	TextureRef	    m_texture;
	int				m_width;
	int				m_height;

	bool			m_isSky;
    SkyParameters   m_skyParameters;
    SkyRef          m_sky;


public:
	TextureViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif 

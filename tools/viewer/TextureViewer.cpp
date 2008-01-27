/**
 @file TextureViewer.cpp
 
 Viewer for image files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "TextureViewer.h"


TextureViewer::TextureViewer() :
    m_texture(NULL),
    m_width(0),
    m_height(0), 
    m_filename(""),
    m_ext("") {

}


void TextureViewer::onInit(const std::string& filename) {

	m_texture = Texture::fromFile( filename, TextureFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::video() );
	m_height = m_texture->height();
	m_width = m_texture->width();
	m_filename = filename;

	m_ext = toLower(filenameExt(m_filename));	

}


void TextureViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
	screenPrintf("Width: %d",  m_width);
	screenPrintf("Height: %d",  m_height);
	screenPrintf("Format: .%s", m_ext.c_str());

	rd->push2D();
		
		// Creates a rectangle the size of the window.
		float windowHeight = rd->viewport().height();
		float windowWidth = rd->viewport().width();
		
		Rect2D rect;
		if((windowWidth > m_width) && (windowHeight > m_height)){
			// creates a rectangle the size of the texture
			// centered in the window
			rect = Rect2D::xywh(windowWidth/2 - m_width/2,
								windowHeight/2 - m_height/2,
								m_width,
								m_height);
		} else {
			// Creates a rectangle the size of the texture
			// in the top left corner of the window, if the window
			// is smaller than the image currently
			rect = m_texture->rect2DBounds();
		}

		rd->setTexture(0, m_texture);
		Draw::rect2D(rect, rd);
	rd->pop2D();
}

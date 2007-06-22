/**
   @file FontViewer.cpp
 
   Viewer for .fnt files, with a default font displayed for comparison
 
   @maintainer Eric Muller 09edm@williams.edu
   @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
   @created 2007-05-31
   @edited  2007-06-08
*/
#include "FontViewer.h"


FontViewer::FontViewer(GFontRef base) :
    m_font(NULL),
    m_basefont(base),
    m_fontfilename("")	
{}


void FontViewer::onInit(const std::string& filename) {

    m_fontfilename = filename;
    m_font = GFont::fromFile( m_fontfilename );
}


void FontViewer::onGraphics(RenderDevice* rd, App* app) {
    app->colorClear = Color3::white();

    rd->push2D();

    Rect2D windowBounds = rd->viewport();

    rd->setLineWidth(0.5f);
    rd->setColor(Color3::black());
    rd->setObjectToWorldMatrix(Vector3(0.375, 0.375, 0));
    rd->beginPrimitive(RenderDevice::LINES);
	
    for (int i = 0; i <= 16; ++i) {
        // Horizontal line
        float y = i * windowBounds.height() / 16;
        rd->sendVertex(Vector2(0, y));
        rd->sendVertex(Vector2(windowBounds.width(), y));
            
        // Vertical line
        float x = i * windowBounds.width() / 16;
        rd->sendVertex(Vector2(x, 0));
        rd->sendVertex(Vector2(x, windowBounds.height()));
    }
    rd->endPrimitive();


    const Vector2 pos2D		= Vector2( 50.0, 50.0 );
    double  size			= windowBounds.height()/16.0 /2.0 ;
    const Color4  color		= Color3::black();
    const Color4 outline	= Color4::clear();
    for (int y = 0; y < 16; ++y ) {
        for (int x = 0; x < 16; ++x ) {
            int i = x + y*16;
            std::string s = "";
            s += (char)i;

            // Character in the current font
            m_font->draw2D( rd, s, Vector2( x* windowBounds.width()/16.0 + windowBounds.width()/32.0, 
                                            y*windowBounds.height()/16.0 + windowBounds.height()/32.0), 
                            size, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER ); 
			
            // Character in the normal font
            m_basefont->draw2D( rd, s, Vector2( x* windowBounds.width()/16.0 + windowBounds.width()/64.0, 
                                                y* windowBounds.height()/16.0 + windowBounds.height()/20.0), 
                                size/2.0, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER ); 

            // Character number
            m_basefont->draw2D( rd, format("%d", i), Vector2( x* windowBounds.width()/16.0 + windowBounds.width()/20.0, 
                                                              y* windowBounds.height()/16.0 + windowBounds.height()/20.0), 
                                size/2.0, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER );

        }
    }


    rd->pop2D();
}

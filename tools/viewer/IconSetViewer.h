/**
 @file IconSet.h
 
 Viewer for .icn files

 \author Morgan McGuire
 
 \created 2010-01-04
 \edited  2010-01-04
 */
#ifndef IconSetViewer_h
#define IconSetViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class IconSetViewer : public Viewer {
private:

    GFont::Ref   m_font;
    IconSet::Ref m_iconSet;

public:
    IconSetViewer(const GFont::Ref& captionFont);
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting);

};

#endif 

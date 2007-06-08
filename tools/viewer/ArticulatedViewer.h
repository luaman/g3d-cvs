/**
 @file ArticulatedViewer.h
 
 Viewer for .3ds models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef ARTICULATEDVIEWER_H
#define ARTICULATEDVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class ArticulatedViewer : public Viewer {
private:

	ArticulatedModelRef		m_model;
	int						m_numEdges;
	int						m_numFaces;
	bool					m_texturebool;
	std::string				m_texture;
	int						m_numVertices;


public:
	ArticulatedViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app);

};

#endif 

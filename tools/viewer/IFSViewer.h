/**
 @file IFSViewer.h
 
 Viewer for .ifs models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef IFSVIEWER_H
#define IFSVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class IFSViewer : public Viewer {
private:

	IFSModelRef		m_model;
	int				m_numEdges;
	int				m_numFaces;
	bool			m_texturebool;
	std::string		m_texture;
	int				m_numVertex;

public:
	IFSViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app);

};

#endif 
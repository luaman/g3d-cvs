/**
 @file IFSViewer.cpp
 
 Viewer for .ifs models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "IFSViewer.h"


IFSViewer::IFSViewer() :
	m_model(NULL),
	m_numEdges(0),
	m_numFaces(0),
	m_texturebool(false),
	m_texture(""),
	m_numVertex(0) 
{}


void IFSViewer::onInit(const std::string& filename) {
	const double scale = 2;
	const bool weld = true;

	m_model = IFSModel::fromFile( filename, scale,  CoordinateFrame(), weld, true );
	SurfaceRef posedifs = m_model->pose(CoordinateFrame(), true);

	m_numEdges = posedifs->edges().size();
	m_numFaces = posedifs->faces().size();
	m_numVertex = posedifs->vertices().size();
	m_texturebool = posedifs->hasTexCoords();
}


void IFSViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
	SurfaceRef posedifs = m_model->pose(CoordinateFrame(), true);

	screenPrintf ( "Edges: %d", m_numEdges );
	screenPrintf ( "Faces: %d", m_numFaces );
	screenPrintf ( "Vertices: %d", m_numVertex );
	if (m_texturebool) {
		screenPrintf ( "Contains texture coordinates" );
	} else {
		screenPrintf ( "No texture coordinates" );
	}
	posedifs->render( rd ); 
}

/**
 \file ArticulatedViewer.h
 
 Viewer for files that can be loaded by ArticulatedModel

 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2010-02-26
 */
#ifndef ArticulatedViewer_h
#define ArticulatedViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class ArticulatedViewer : public Viewer {
private:

	ArticulatedModel::Ref	    m_model;
	int						    m_numEdges;
	int						    m_numFaces;
	int						    m_numVertices;

    int                         m_selectedPartIndex;
    int                         m_selectedTriListIndex;
    /** In the trilist index array */
    int                         m_selectedTriangleIndex;
    SuperSurface::GPUGeom::Ref  m_selectedGeom;

    float                       m_scale;
    Vector3                     m_offset;


public:
	ArticulatedViewer();
	virtual void onInit(const std::string& filename);
    virtual bool onEvent(const GEvent& e, App* app);
    virtual void onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting);

};

#endif 

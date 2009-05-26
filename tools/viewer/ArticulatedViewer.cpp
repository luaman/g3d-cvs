/**
 @file ArticulatedViewer.cpp
 
 Viewer for .3ds models
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "ArticulatedViewer.h"

ArticulatedViewer::ArticulatedViewer() :
	m_model(NULL),
	m_numEdges(0),
	m_numFaces(0),
	m_texturebool(false),
	m_texture(""),
	m_numVertices(0)
	{}


void ArticulatedViewer::onInit(const std::string& filename) {

	m_model = ArticulatedModel::fromFile(filename);
	Array<Surface::Ref> arrayModel;
	m_model->pose(arrayModel);

	const Array<ArticulatedModel::Part>& partArray = m_model->partArray;

	// Find the number of vertices and faces in the model, for display later
	for (int y = 0; y < partArray.size(); ++y) {
		m_numVertices += partArray[y].geometry.vertexArray.size();
		m_numFaces += ((partArray[y].indexArray.size())/3);
	}

    return;

    // TODO: Scale the model in place, rather than re-loading!

    bool overwrite = true;

	// Gind the size of the bounding box of the entire model
	AABox bounds;
	if (arrayModel.size() > 0) {
		m_texturebool = false;
	
		//checks to see if the model has texture coordinates, for display later
		for (int x = 0; x < arrayModel.size(); ++x) {		
			m_texturebool = arrayModel[x]->hasTexCoords() || m_texturebool;

			//merges the bounding boxes of all the seperate parts into the bounding box of the entire object
			AABox temp;
			Box partBounds = arrayModel[x]->worldSpaceBoundingBox();

			// Some models have screwed up bounding boxes
			if (partBounds.extent().isFinite()) {
                if (overwrite) {
                    partBounds.getBounds(bounds);
                    overwrite = false;
                } else {
				    partBounds.getBounds(temp);
				    bounds.merge(temp);
                }
			}
		}

        if (overwrite) {
            // We never found a part with a finite bounding box
            bounds = AABox(Vector3::zero());
        }

		Vector3 extent = bounds.extent();
		Vector3 center = bounds.center();

		float scale = 2.0f / max(extent.x, max(extent.y, extent.z));

        if (scale <= 0) {
            scale = 1;
        }

        if (! isFinite(scale)) {
            scale = 1;
        }

        if (! center.isFinite()) {
            center = Vector3::zero();
        }

		CoordinateFrame transform;
		transform.rotation = Matrix3::identity() * scale;
		transform.translation = -center * scale;

		m_model = ArticulatedModel::fromFile( filename, transform );
	}
}


void ArticulatedViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {

    // Separate and sort the models
	static Array<G3D::Surface::Ref> posed3D;
    
    m_model->pose(posed3D);
    Surface::sortAndRender(rd, app->defaultCamera, posed3D, lighting, app->shadowMap);
    posed3D.fastClear();

	screenPrintf("Faces: %d", m_numFaces);
	screenPrintf("Vertices: %d", m_numVertices);

	if (m_texturebool) {
		screenPrintf("Contains texture Coordinates");
	} else {
		screenPrintf("No texture coordinates");
	}
}

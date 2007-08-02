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
	Array<PosedModel::Ref> arrayModel;
	m_model->pose(arrayModel);

	const Array<ArticulatedModel::Part>& partArray = m_model->partArray;

	// Find the number of vertices and faces in the model, for display later
	for (int y = 0; y < partArray.size(); ++y) {
		m_numVertices += partArray[y].geometry.vertexArray.size();
		m_numFaces += ((partArray[y].indexArray.size())/3);
	}

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
	static Array<G3D::PosedModel::Ref> otherOpaque;
	static Array<G3D::PosedModel::Ref> opaqueAModel;
	static Array<G3D::PosedModel::Ref> transparent;

	const CoordinateFrame cameraFrame = rd->getCameraToWorldMatrix();

	otherOpaque.fastClear();
	opaqueAModel.fastClear();
	transparent.fastClear();

//    static RealTime t0 = System::time();
//    float angle = (System::time() - t0);
    m_model->pose(otherOpaque);//, Matrix3::fromAxisAngle(Vector3::unitY(), angle));

    bool shadows = (lighting->shadowedLightArray.size() > 0) && app->shadowMap.enabled();

    if (shadows) {  
        // Generate shadow map        
        const float lightProjX = 8, lightProjY = 8, lightProjNear = 1, lightProjFar = 20;
        app->shadowMap.updateDepth(rd, lighting->shadowedLightArray[0],
            lightProjX, lightProjY, lightProjNear, lightProjFar, otherOpaque);
    }

	Vector3 cameraLook = cameraFrame.lookVector();
	PosedModel::sort(otherOpaque, cameraLook , opaqueAModel, transparent);

    // Opaque unshadowed
    ArticulatedModel::renderNonShadowed(opaqueAModel, rd, lighting);
    for (int m = 0; m < otherOpaque.size(); ++m) {
        otherOpaque[m]->renderNonShadowed(rd, lighting);
    }

    // Opaque shadowed    
    if (shadows) {
        ArticulatedModel::renderShadowMappedLightPass(opaqueAModel, rd, lighting->shadowedLightArray[0], app->shadowMap.lightMVP(), app->shadowMap.depthTexture());
        for (int m = 0; m < otherOpaque.size(); ++m) {
            otherOpaque[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], app->shadowMap.lightMVP(), app->shadowMap.depthTexture());
        }
    }

    // Transparent
    for (int m = 0; m < transparent.size(); ++m) {
        transparent[m]->renderNonShadowed(rd, lighting);
        if (shadows) {
            transparent[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], app->shadowMap.lightMVP(), app->shadowMap.depthTexture());
        }
    }

	app->debugPrintf("Faces: %d", m_numFaces);
	app->debugPrintf("Vertices: %d", m_numVertices);

	if (m_texturebool) {
		app->debugPrintf("Contains texture Coordinates");
	} else {
		app->debugPrintf("No texture coordinates");
	}
}

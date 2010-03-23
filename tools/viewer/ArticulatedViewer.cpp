/**
 @file ArticulatedViewer.cpp
 
 Viewer for .3ds models
 
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "ArticulatedViewer.h"

ArticulatedViewer::ArticulatedViewer() :
	m_model(NULL),
	m_numEdges(0),
	m_numFaces(0),
	m_numVertices(0)
	{}


void ArticulatedViewer::onInit(const std::string& filename) {

    m_selectedPartIndex = -1;
    m_selectedTriListIndex = -1;

    ArticulatedModel::Preprocess preprocess;
    m_model = ArticulatedModel::fromFile(filename, preprocess);
    Array<Surface::Ref> arrayModel;
    m_model->pose(arrayModel);
    
    const Array<ArticulatedModel::Part>& partArray = m_model->partArray;
    
    // Find the number of vertices and faces in the model, for display later
    for (int y = 0; y < partArray.size(); ++y) {
        m_numVertices += partArray[y].geometry.vertexArray.size();
        m_numFaces += ((partArray[y].indexArray.size())/3);
    }
    
    m_scale = 1;
    m_offset = Vector3::zero();
    bool overwrite = true;
    
    // Find the size of the bounding box of the entire model
    AABox bounds;
    if (arrayModel.size() > 0) {
        
        for (int x = 0; x < arrayModel.size(); ++x) {		
            
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
        
        // Scale to 5 units
        float scale = 5.0f / max(extent.x, max(extent.y, extent.z));
        
        if (scale <= 0) {
            scale = 1;
        }

        if (! isFinite(scale)) {
            scale = 1;
        }

        m_scale = scale;
        m_offset = -scale * center;

        if (! center.isFinite()) {
            center = Vector3();
        }

        // Transform parts in-place
        for (int partIndex = 0; partIndex < m_model->partArray.length(); ++partIndex) {
            ArticulatedModel::Part& part = m_model->partArray[partIndex];

            MeshAlg::Geometry& geom = part.geometry;

            // Transform cframes
            if (part.parent == -1) {
                // Translate the root(s)
                part.cframe.translation -= center;
            }

            part.cframe.translation *= scale;
            
            for (int vertIndex = 0; vertIndex < geom.vertexArray.length(); ++vertIndex) {
                geom.vertexArray[vertIndex] = geom.vertexArray[vertIndex] * scale;
            }
            
            m_model->partArray[partIndex].computeBounds();
            m_model->partArray[partIndex].updateVAR();
        }
    }
}


static void printHierarchy
(const ArticulatedModel::Ref& model,
 int                          index,
 const std::string&           indent) {
    
    const ArticulatedModel::Part& part = model->partArray[index];
    screenPrintf("%s`%s'\n", indent.c_str(), part.name.c_str());

    for (int i = 0; i < part.subPartArray.size(); ++i) {
        printHierarchy(model, part.subPartArray[i], indent + "  ");
    }
}


void ArticulatedViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    
    // Separate and sort the models
    static Array<G3D::Surface::Ref> posed3D;
    
    m_model->pose(posed3D);
    Surface::sortAndRender(rd, app->defaultCamera, posed3D, lighting, app->shadowMap);

    for (int p = 0; p < posed3D.size(); ++p) {
        SuperSurface::Ref s = posed3D[p].downcast<SuperSurface>();
        if (m_selectedGeom == s->gpuGeom()) {
            rd->pushState();
                rd->setObjectToWorldMatrix(s->coordinateFrame());
                rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
                rd->setPolygonOffset(-1.0f);
                rd->setColor(Color3::green() * 0.8f);
                rd->setTexture(0, NULL);
                s->sendGeometry(rd);
            rd->popState();
            break;
        }
    }
    posed3D.fastClear();

    screenPrintf("[Shown scaled by %f and offset by (%f, %f, %f)]\n", m_scale, m_offset.x, m_offset.y, m_offset.z);
    
    screenPrintf("Model Faces: %d,  Vertices: %d\n", m_numFaces, m_numVertices);
    if (m_selectedGeom.notNull()) {
        screenPrintf(" Selected `%s' partArray[%d].triList[%d].indexArray[%d...%d]\n", 
            m_model->partArray[m_selectedPartIndex].name.c_str(),
            m_selectedPartIndex, m_selectedTriListIndex,
            m_selectedTriangleIndex, m_selectedTriangleIndex + 2);
    }

    screenPrintf("Hierarchy:");
    // Hierarchy: find roots
    for (int i = 0; i < m_model->partArray.size(); ++i) {
        if (m_model->partArray[i].parent == -1) {
            printHierarchy(m_model, i, "");
        }
    }
}


bool ArticulatedViewer::onEvent(const GEvent& e, App* app) {
    if ((e.type == GEventType::MOUSE_BUTTON_CLICK) && (e.button.button == 0)) {
        // Intersect all tri lists with the ray from the camera
        const Ray& ray = app->defaultCamera.worldRay(e.button.x, e.button.y, 
            app->renderDevice->viewport());

        float distance = inf();
        m_selectedGeom = NULL;
        m_selectedPartIndex = -1;
        m_selectedTriListIndex = -1;
        m_selectedTriangleIndex = -1;

        for (int i = 0; i < m_model->partArray.size(); ++i) {
            const ArticulatedModel::Part& part = m_model->partArray[i];
            alwaysAssertM(part.parent == -1, "Not implemented for parts with hierarchy");

            for (int t = 0; t < part.triList.size(); ++t) {
                ArticulatedModel::Part::TriList::Ref triList = part.triList[t];

                const Box& wsBox = part.cframe.toWorldSpace(triList->boxBounds);
                float test = ray.intersectionTime(wsBox);
                if (test < distance) {
                    // See if we clicked on an actual triangle
                    const Ray&              osRay   = part.cframe.toObjectSpace(ray);
                    const Array<Vector3>&   vertex  = part.geometry.vertexArray;
                    const Array<int>&       index   = triList->indexArray;

                    debugAssert(index.size() % 3 == 0);
                    for (int j = 0; j < index.size(); j += 3) {
                        test = osRay.intersectionTime(vertex[index[j]], vertex[index[j + 1]], vertex[index[j + 2]]);
                        if (test < distance) {
                            m_selectedGeom = triList;
                            m_selectedPartIndex = i;
                            m_selectedTriListIndex = t;
                            m_selectedTriangleIndex = j;
                        }
                    }

                    if (triList->twoSided) {
                        for (int j = 0; j < index.size(); j += 3) {
                            test = osRay.intersectionTime(vertex[index[j + 2]], vertex[index[j + 1]], vertex[index[j]]);
                            if (test < distance) {
                                m_selectedGeom = triList;
                                m_selectedPartIndex = i;
                                m_selectedTriListIndex = t;
                                m_selectedTriangleIndex = j;
                            }
                        }
                    }
                }
            }
        }

        return true;
    }

    return false;
}

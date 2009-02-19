/**
  @file PosedModel.cpp
  
  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2003-11-15
  @edited  2009-02-25
 */ 

#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/ArticulatedModel.h"
#include "G3D/GCamera.h"
#include "G3D/debugPrintf.h"
#include "G3D/Log.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "GLG3D/PosedModel.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"

namespace G3D {

void PosedModel::getBoxBounds(const Array<PosedModel::Ref>& models, AABox& bounds) {
    if (models.size() == 0) {
        bounds = AABox();
        return;
    }

    models[0]->worldSpaceBoundingBox().getBounds(bounds);

    for (int i = 1; i < models.size(); ++i) {
        AABox temp;
        models[i]->worldSpaceBoundingBox().getBounds(temp);
        bounds.merge(temp);
    }
}


void PosedModel::getSphereBounds(const Array<PosedModel::Ref>& models, Sphere& bounds) {
    AABox temp;
    getBoxBounds(models, temp);
    bounds = Sphere(temp.center(), temp.extent().length() / 2.0f);
}


void PosedModel::sortAndRender
(
 RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<PosedModelRef>&    allModels, 
 const LightingRef&             _lighting, 
 const Array<ShadowMapRef>&     shadowMaps,
 const Array<SuperShader::PassRef>& extraAdditivePasses) {

    static bool recurse = false;

    alwaysAssertM(! recurse, "Cannot call PosedModel::sortAndRender recursively");
    recurse = true;

    static Array<PosedModel::Ref> opaqueGeneric, otherOpaque, transparent, posed3D;

    LightingRef lighting = _lighting->clone();

    bool renderShadows =
        (shadowMaps.size() > 0) && 
        (lighting->shadowedLightArray.size() > 0) && 
        shadowMaps[0]->enabled();

    if (renderShadows) {
        // Remove excess lights
        if (shadowMaps.size() < lighting->shadowedLightArray.size()) {
            for (int L = shadowMaps.size() - 1; L < lighting->shadowedLightArray.size(); ++L) {
                lighting->lightArray.append(lighting->shadowedLightArray[L]);
            }
            lighting->shadowedLightArray.resize(shadowMaps.size());
        }
 
        // Find the scene bounds
        AABox sceneBounds;
        PosedModel::getBoxBounds(allModels, sceneBounds);

        // Generate shadow maps
        for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
            float lightProjX = 12, lightProjY = 12, lightProjNear = 0.5f, lightProjFar = 60;
            const GLight& light = lighting->shadowedLightArray[L];

            if (lighting->shadowedLightArray[L].spotCutoff <= 90) {
                // Spot light; we can set the lightProj bounds intelligently

                debugAssert(lighting->shadowedLightArray[L].position.w == 1.0f);

                CFrame lightFrame;
                lightFrame.lookAt(lighting->shadowedLightArray[L].spotDirection);
                lightFrame.translation = lighting->shadowedLightArray[L].position.xyz();

                // Find nearest and farthest corners of the scene bounding box
                lightProjNear = inf();
                lightProjFar  = 0;
                for (int c = 0; c < 8; ++c) {
                    Vector3 v = sceneBounds.corner(c);
                    v = lightFrame.pointToObjectSpace(v);
                    lightProjNear = min(lightProjNear, -v.z);
                    lightProjFar = max(lightProjFar, -v.z);
                }
                
                // Don't let the near get too close to the source
                lightProjNear = max(0.2f, lightProjNear);

                // Don't bother tracking shadows past the effective radius
                lightProjFar = min(light.effectSphere().radius, lightProjFar);
                lightProjFar = max(lightProjNear + 0.1f, lightProjFar);

                // The cutoff is half the angle of extent (See the Red Book, page 193)
                float angle = toRadians(lighting->shadowedLightArray[L].spotCutoff);
                lightProjX = lightProjNear * sin(angle);
                // Symmetric in x and y
                lightProjY = lightProjX;

                Matrix4 lightProjectionMatrix = 
                    Matrix4::perspectiveProjection(-lightProjX, lightProjX, -lightProjY, 
                                                              lightProjY, lightProjNear, lightProjFar);
                shadowMaps[L]->updateDepth(rd, lightFrame, lightProjectionMatrix, allModels);
            } else if (lighting->shadowedLightArray[L].position.w == 0) {
                // Directional light

                /*
                // Find the bounds on the projected character
                AABox2D bounds;
                for (int m = 0; m < allModels.size(); ++m) {
                    const PosedModelRef& model = allModels[m];
                    const Sphere& b = model->worldSpaceBoundingSphere();
                    
                }*/

                // Find bounds of all shadow casting
                shadowMaps[L]->updateDepth(rd, lighting->shadowedLightArray[L].position,
                                          lightProjX, lightProjY, lightProjNear, lightProjFar, allModels);
            } else {
                // Point light
                shadowMaps[L]->updateDepth(rd, lighting->shadowedLightArray[L].position,
                                          lightProjX, lightProjY, lightProjNear, lightProjFar, allModels);
            }
        }
    } else {
        // We're not going to be able to draw shadows, so move the shadowed lights into
        // the unshadowed category.
        lighting->lightArray.append(lighting->shadowedLightArray);
        lighting->shadowedLightArray.clear();
    }

    // Cull objects outside the view frustum
    static Array<Plane> clipPlanes;
    camera.getClipPlanes (rd->viewport(), clipPlanes);
    for (int i = 0; i < allModels.size(); ++i) {
        const Sphere& sphere = allModels[i]->worldSpaceBoundingSphere();
        if (! sphere.culledBy(clipPlanes)) {
            posed3D.append(allModels[i]);
        }
    }

    // Separate and sort the models
    GenericPosedModel::extractOpaque(posed3D, opaqueGeneric);
    PosedModel::sort(opaqueGeneric, camera.coordinateFrame().lookVector(), opaqueGeneric);
    PosedModel::sort(posed3D, camera.coordinateFrame().lookVector(), otherOpaque, transparent);
    rd->setProjectionAndCameraMatrix(camera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    // Opaque unshadowed
    for (int m = 0; m < otherOpaque.size(); ++m) {
        otherOpaque[m]->renderNonShadowed(rd, lighting);
    }
    GenericPosedModel::renderNonShadowed(opaqueGeneric, rd, lighting);

    // Opaque shadowed
    for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
        rd->pushState();
        GenericPosedModel::renderShadowMappedLightPass(opaqueGeneric, rd, lighting->shadowedLightArray[L], shadowMaps[L]);
        rd->popState();
        for (int m = 0; m < otherOpaque.size(); ++m) {
            otherOpaque[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[L], shadowMaps[L]);
        }
    }

    // Extra additive passes
    if (extraAdditivePasses.size() > 0) {
        rd->pushState();
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
            for (int p = 0; p < extraAdditivePasses.size(); ++p) {
                for (int m = 0; m < opaqueGeneric.size(); ++m) {
                    opaqueGeneric[m]->renderSuperShaderPass(rd, extraAdditivePasses[p]);
                }
                for (int m = 0; m < otherOpaque.size(); ++m) {
                    otherOpaque[m]->renderSuperShaderPass(rd, extraAdditivePasses[p]);
                }
        }
        rd->popState();
    }

    // Transparent, must be rendered from back to front
    for (int m = 0; m < transparent.size(); ++m) {
        transparent[m]->renderNonShadowed(rd, lighting);
        for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
            transparent[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[L], shadowMaps[L]);
        }
        for (int p = 0; p < extraAdditivePasses.size(); ++p) {
            transparent[m]->renderSuperShaderPass(rd, extraAdditivePasses[p]);
        }
    }

    opaqueGeneric.fastClear();
    otherOpaque.fastClear();
    transparent.fastClear();
    posed3D.fastClear();

    recurse = false;
}


void PosedModel::sortAndRender
    (
     class RenderDevice*            rd, 
     const class GCamera&           camera,
     const Array<PosedModelRef>&    allModels, 
     const LightingRef&             _lighting, 
     const Array<ShadowMapRef>&     shadowMaps) {
    sortAndRender(rd, camera, allModels, _lighting, shadowMaps, Array<SuperShader::PassRef>());
}


void PosedModel::sortAndRender
(
 RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<PosedModelRef>&    posed3D, 
 const LightingRef&             lighting, 
 const ShadowMapRef             shadowMap) {

    static Array<ShadowMapRef> shadowMaps;
    if (shadowMap.notNull()) {
        shadowMaps.append(shadowMap);
    }
    sortAndRender(rd, camera, posed3D, lighting, shadowMaps, Array<SuperShader::PassRef>());
    shadowMaps.fastClear();
}


void PosedModel2D::sortAndRender(RenderDevice* rd, Array<PosedModel2DRef>& posed2D) {
    if (posed2D.size() > 0) {
        rd->push2D();
            PosedModel2D::sort(posed2D);
            for (int i = 0; i < posed2D.size(); ++i) {
                posed2D[i]->render(rd);
            }
        rd->pop2D();
    }
}

class ModelSorter {
public:
    float                     sortKey;
    PosedModel::Ref           model;

    ModelSorter() {}

    ModelSorter(const PosedModel::Ref& m, const Vector3& axis) : model(m) {
        Sphere s;
        m->getWorldSpaceBoundingSphere(s);
        sortKey = axis.dot(s.center);
    }

    inline bool operator>(const ModelSorter& s) const {
        return sortKey > s.sortKey;
    }

    inline bool operator<(const ModelSorter& s) const {
        return sortKey < s.sortKey;
    }
};


void PosedModel::sort(
    const Array<PosedModel::Ref>& inModels, 
    const Vector3&                wsLook,
    Array<PosedModel::Ref>&       opaque,
    Array<PosedModel::Ref>&       transparent) {

    static Array<ModelSorter> op;
    static Array<ModelSorter> tr;
    
    for (int m = 0; m < inModels.size(); ++m) {
        if (inModels[m]->hasTransparency()) {
            tr.append(ModelSorter(inModels[m], wsLook));
        } else {
            op.append(ModelSorter(inModels[m], wsLook));
        }
    }

    // Sort
    tr.sort(SORT_DECREASING);
    op.sort(SORT_INCREASING);

    transparent.resize(tr.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int m = 0; m < tr.size(); ++m) {
        transparent[m] = tr[m].model;
    }

    opaque.resize(op.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int m = 0; m < op.size(); ++m) {
        opaque[m] = op[m].model;
    }

    tr.fastClear();
    op.fastClear();
}


void PosedModel::sort(
    const Array<PosedModel::Ref>& inModels, 
    const Vector3&                wsLook,
    Array<PosedModel::Ref>&       opaque) { 

    if (&inModels == &opaque) {
        // The user is trying to sort in place.  Make a separate array for them.
        Array<PosedModel::Ref> temp = inModels;
        sort(temp, wsLook, opaque);
        return;
    }

    static Array<ModelSorter> op;
    
    for (int m = 0; m < inModels.size(); ++m) {
        op.append(ModelSorter(inModels[m], wsLook));
    }

    // Sort
    op.sort(SORT_INCREASING);

    opaque.resize(op.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int m = 0; m < op.size(); ++m) {
        opaque[m] = op[m].model;
    }
    op.fastClear();
}


void PosedModel::getWorldSpaceGeometry(MeshAlg::Geometry& geometry) const {
    CoordinateFrame c;
    getCoordinateFrame(c);

    const MeshAlg::Geometry& osgeometry = objectSpaceGeometry();
    c.pointToWorldSpace(osgeometry.vertexArray, geometry.vertexArray);
    c.normalToWorldSpace(osgeometry.normalArray, geometry.normalArray);
}


CoordinateFrame PosedModel::coordinateFrame() const {
    CoordinateFrame c;
    getCoordinateFrame(c);
    return c;
}


Sphere PosedModel::objectSpaceBoundingSphere() const {
    Sphere s;
    getObjectSpaceBoundingSphere(s);
    return s;
}


void PosedModel::getWorldSpaceBoundingSphere(Sphere& s) const {
    CoordinateFrame C;
    getCoordinateFrame(C);
    getObjectSpaceBoundingSphere(s);
    s = C.toWorldSpace(s);
}


Sphere PosedModel::worldSpaceBoundingSphere() const {
    Sphere s;
    getWorldSpaceBoundingSphere(s);
    return s;
}


AABox PosedModel::objectSpaceBoundingBox() const {
    AABox b;
    getObjectSpaceBoundingBox(b);
    return b;
}


void PosedModel::getWorldSpaceBoundingBox(AABox& box) const {
    CoordinateFrame C;
    getCoordinateFrame(C);
    getObjectSpaceBoundingBox(box);
    if (! box.isFinite()) {
        box = AABox::inf();
    } else {
        C.toWorldSpace(box).getBounds(box);
    }
}


AABox PosedModel::worldSpaceBoundingBox() const {
    AABox b;
    getWorldSpaceBoundingBox(b);
    return b;
}


void PosedModel::getObjectSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize) const {
    const MeshAlg::Geometry& geometry = objectSpaceGeometry();
    const Array<MeshAlg::Face>& faceArray = faces();

    MeshAlg::computeFaceNormals(geometry.vertexArray, faceArray, faceNormals, normalize);
}


void PosedModel::getWorldSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize) const {
    MeshAlg::Geometry geometry;
    getWorldSpaceGeometry(geometry);

    const Array<MeshAlg::Face>& faceArray = faces();

    MeshAlg::computeFaceNormals(geometry.vertexArray, faceArray, faceNormals, normalize);
}


void PosedModel::renderNonShadowed(
    RenderDevice* rd,
    const LightingRef& lighting) const {

    rd->pushState();
        if (rd->colorWrite()) {
            rd->setAmbientLightColor(lighting->ambientTop);
            Color3 C = lighting->ambientBottom - lighting->ambientTop;

            int shift = 0;
            if ((C.r != 0) || (C.g != 0) || (C.b != 0)) {
                rd->setLight(0, GLight::directional(-Vector3::unitY(), C, false));
                ++shift;
            }

            for (int L = 0; L < iMin(7, lighting->lightArray.size()); ++L) {
                rd->setLight(L + shift, lighting->lightArray[L]);
            }
            rd->enableLighting();
        }
        render(rd);
    rd->popState();
}


void PosedModel::renderShadowedLightPass(
    RenderDevice* rd, 
    const GLight& light) const {

    rd->pushState();
        rd->enableLighting();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->setLight(0, light);
        rd->setAmbientLightColor(Color3::black());
        render(rd);
    rd->popState();
}


void PosedModel::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const ShadowMapRef& shadowMap) const {

    renderShadowMappedLightPass(rd, light, shadowMap->biasedLightMVP(), shadowMap->depthTexture());
}


void PosedModel::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const Matrix4& lightMVP,
    const Texture::Ref& shadowMap) const {

    rd->pushState();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->configureShadowMap(1, lightMVP, shadowMap);
        rd->setLight(0, light);
        rd->enableLighting();
        rd->setAmbientLightColor(Color3::black());
        render(rd);
    rd->popState();
}

   
void PosedModel::defaultRender(RenderDevice* rd) const {
    const MeshAlg::Geometry& geometry = objectSpaceGeometry();

    VARAreaRef area = VARArea::create(sizeof(Vector3)*2*geometry.vertexArray.size() + 16);

    rd->pushState();
        rd->setObjectToWorldMatrix(coordinateFrame());
        rd->beginIndexedPrimitives();
            rd->setNormalArray(VAR(geometry.normalArray, area));
            rd->setVertexArray(VAR(geometry.vertexArray, area));
            rd->sendIndices(RenderDevice::TRIANGLES, triangleIndices());
        rd->endIndexedPrimitives();
    rd->popState();
}


void PosedModel::render(RenderDevice* rd) const {
    defaultRender(rd);
}

void PosedModel::sendGeometry(RenderDevice* rd) const {
    const MeshAlg::Geometry& geom = objectSpaceGeometry();

    size_t s = sizeof(Vector3) * geom.vertexArray.size() * 2;

    if (hasTexCoords()) {
        s += sizeof(Vector2) * texCoords().size();
    }

    VARAreaRef area = VARArea::create(s);
    VAR vertex(geom.vertexArray, area);
    VAR normal(geom.normalArray, area);
    VAR texCoord;

    if (hasTexCoords()) {
        texCoord = VAR(texCoords(), area);
    }

    rd->beginIndexedPrimitives();
        rd->setVertexArray(vertex);
        rd->setNormalArray(normal);
        if (hasTexCoords()) {
            rd->setTexCoordArray(0, texCoord);
        }
        rd->sendIndices(RenderDevice::TRIANGLES, triangleIndices());
    rd->endIndexedPrimitives();
}

////////////////////////////////////////////////////////////////////////////////////////

static bool depthGreaterThan(const PosedModel2DRef& a, const PosedModel2DRef& b) {
    return a->depth() > b->depth();
}

void PosedModel2D::sort(Array<PosedModel2DRef>& array) {
    array.sort(depthGreaterThan);
}

}

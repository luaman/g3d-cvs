/**
  @file Surface.cpp
  
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
#include "GLG3D/Surface.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"

namespace G3D {

const Array<Vector2>& Surface::texCoords() const {
    static Array<Vector2> t;
    return t;
}


const Array<Vector3>& Surface::objectSpaceTangents() const {
    static Array<Vector3> t;
    return t;
}


void Surface::getBoxBounds(const Array<Surface::Ref>& models, AABox& bounds) {
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


void Surface::getSphereBounds(const Array<Surface::Ref>& models, Sphere& bounds) {
    AABox temp;
    getBoxBounds(models, temp);
    bounds = Sphere(temp.center(), temp.extent().length() / 2.0f);
}


void Surface::cull(const GCamera& camera, const Rect2D& viewport, const Array<Surface::Ref>& allModels, Array<Surface::Ref>& outModels) {
    outModels.fastClear();

    Array<Plane> clipPlanes;
    camera.getClipPlanes(viewport, clipPlanes);
    for (int i = 0; i < allModels.size(); ++i) {
        const Sphere& sphere = allModels[i]->worldSpaceBoundingSphere();
        if (! sphere.culledBy(clipPlanes)) {
            outModels.append(allModels[i]);
        }
    }
}


void Surface::renderDepthOnly
(RenderDevice* rd, 
 const Array<Surface::Ref>& allModels, 
 RenderDevice::CullFace cull) {

    rd->pushState();
    {
        rd->setCullFace(cull);
        rd->setDepthWrite(true);
        rd->setColorWrite(false);
        rd->setAlphaTest(RenderDevice::ALPHA_GEQUAL, 0.5f);

        // Maintain sort order while extracting generics
        Array<SuperSurface::Ref> genericModels;

        // Render non-generics while filtering
        for (int i = 0; i < allModels.size(); ++i) {
            const SuperSurface::Ref& g = allModels[i].downcast<SuperSurface>();
            if (g.notNull()) {
                genericModels.append(g);
            } else {
                allModels[i]->render(rd);
            }
        }

        // Render generics
        rd->setCullFace(cull);
        rd->beginIndexedPrimitives();
        {
            for (int g = 0; g < genericModels.size(); ++g) {
                const SuperSurface::Ref& model = genericModels[g];
                const SuperSurface::GPUGeom::Ref& geom = model->gpuGeom();

                if (geom->twoSided) {
                    rd->setCullFace(RenderDevice::CULL_NONE);
                }

                const Texture::Ref& lambertian = geom->material->bsdf()->lambertian().texture();
                if (lambertian.notNull() && ! lambertian->opaque()) {
                    // We need the texture for alpha masking
                    rd->setTexture(0, lambertian);
                } else {
                    rd->setTexture(0, NULL);
                }

                rd->setObjectToWorldMatrix(model->coordinateFrame());
                rd->setVARs(geom->vertex, VertexRange(), geom->texCoord0);
                rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);
            
                if (geom->twoSided) {
                    rd->setCullFace(cull);
                }
            }
        }
        rd->endIndexedPrimitives();
    }
    rd->popState();
}


void Surface::sortAndRender
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<SurfaceRef>&    allModels, 
 const LightingRef&             _lighting, 
 const Array<ShadowMap::Ref>&   shadowMaps,
 const Array<SuperShader::PassRef>& extraAdditivePasses) {

    static bool recurse = false;

    alwaysAssertM(! recurse, "Cannot call Surface::sortAndRender recursively");
    recurse = true;

    static Array<Surface::Ref> opaqueGeneric, otherOpaque, transparent, posed3D;

    Lighting::Ref lighting = _lighting->clone();

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
        Surface::getBoxBounds(allModels, sceneBounds);

        Array<Surface::Ref> lightVisible;
        Array<Surface::Ref> lightSorted;

        // Generate shadow maps
        for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
            const GLight& light = lighting->shadowedLightArray[L];

            GCamera lightFrame;
            Matrix4 lightProjectionMatrix;

            ShadowMap::computeMatrices(light, sceneBounds, lightFrame, lightProjectionMatrix);

            Surface::cull(lightFrame, shadowMaps[L]->rect2DBounds(), allModels, lightVisible);
            Surface::sort(lightVisible, lightFrame.coordinateFrame().lookVector(), lightSorted);
            shadowMaps[L]->updateDepth(rd, lightFrame.coordinateFrame(), lightProjectionMatrix, lightSorted, 0.001f);

            lightVisible.fastClear();
            lightSorted.fastClear();
        }
    } else {
        // We're not going to be able to draw shadows, so move the shadowed lights into
        // the unshadowed category.
        lighting->lightArray.append(lighting->shadowedLightArray);
        lighting->shadowedLightArray.clear();
    }

    // Cull objects outside the view frustum
    cull(camera, rd->viewport(), allModels, posed3D);

    // Separate and sort the models
    SuperSurface::extractOpaque(posed3D, opaqueGeneric);
    Surface::sort(opaqueGeneric, camera.coordinateFrame().lookVector(), opaqueGeneric);
    Surface::sort(posed3D, camera.coordinateFrame().lookVector(), otherOpaque, transparent);
    rd->setProjectionAndCameraMatrix(camera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    // Opaque unshadowed
    for (int m = 0; m < otherOpaque.size(); ++m) {
        otherOpaque[m]->renderNonShadowed(rd, lighting);
    }
    SuperSurface::renderNonShadowed(opaqueGeneric, rd, lighting);

    // Opaque shadowed
    for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
        rd->pushState();
        SuperSurface::renderShadowMappedLightPass(opaqueGeneric, rd, lighting->shadowedLightArray[L], shadowMaps[L]);
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
    renderTransparents(rd, transparent, lighting, extraAdditivePasses, 
                       shadowMaps, RefractionQuality::BEST);

    opaqueGeneric.fastClear();
    otherOpaque.fastClear();
    transparent.fastClear();
    posed3D.fastClear();

    recurse = false;
}


void Surface::sortAndRender
(class RenderDevice*            rd, 
 const class GCamera&           camera,
 const Array<SurfaceRef>&    allModels, 
 const LightingRef&             _lighting, 
 const Array<ShadowMap::Ref>&     shadowMaps) {
    sortAndRender(rd, camera, allModels, _lighting, shadowMaps, Array<SuperShader::PassRef>());
}


void Surface::sortAndRender
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<Surface::Ref>&  posed3D, 
 const LightingRef&             lighting, 
 const ShadowMap::Ref&          shadowMap) {

     static Array<ShadowMap::Ref> shadowMaps;
    if (shadowMap.notNull()) {
        shadowMaps.append(shadowMap);
    }
    sortAndRender(rd, camera, posed3D, lighting, shadowMaps, Array<SuperShader::PassRef>());
    shadowMaps.fastClear();
}


void Surface2D::sortAndRender(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    if (posed2D.size() > 0) {
        rd->push2D();
            Surface2D::sort(posed2D);
            for (int i = 0; i < posed2D.size(); ++i) {
                posed2D[i]->render(rd);
            }
        rd->pop2D();
    }
}

class ModelSorter {
public:
    float                     sortKey;
    Surface::Ref           model;

    ModelSorter() {}

    ModelSorter(const Surface::Ref& m, const Vector3& axis) : model(m) {
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


void Surface::sort(
    const Array<Surface::Ref>& inModels, 
    const Vector3&                wsLook,
    Array<Surface::Ref>&       opaque,
    Array<Surface::Ref>&       transparent) {

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


void Surface::sort(
    const Array<Surface::Ref>& inModels, 
    const Vector3&                wsLook,
    Array<Surface::Ref>&       opaque) { 

    if (&inModels == &opaque) {
        // The user is trying to sort in place.  Make a separate array for them.
        Array<Surface::Ref> temp = inModels;
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


void Surface::getWorldSpaceGeometry(MeshAlg::Geometry& geometry) const {
    CoordinateFrame c;
    getCoordinateFrame(c);

    const MeshAlg::Geometry& osgeometry = objectSpaceGeometry();
    c.pointToWorldSpace(osgeometry.vertexArray, geometry.vertexArray);
    c.normalToWorldSpace(osgeometry.normalArray, geometry.normalArray);
}


CoordinateFrame Surface::coordinateFrame() const {
    CoordinateFrame c;
    getCoordinateFrame(c);
    return c;
}


Sphere Surface::objectSpaceBoundingSphere() const {
    Sphere s;
    getObjectSpaceBoundingSphere(s);
    return s;
}


void Surface::getWorldSpaceBoundingSphere(Sphere& s) const {
    CoordinateFrame C;
    getCoordinateFrame(C);
    getObjectSpaceBoundingSphere(s);
    s = C.toWorldSpace(s);
}


Sphere Surface::worldSpaceBoundingSphere() const {
    Sphere s;
    getWorldSpaceBoundingSphere(s);
    return s;
}


AABox Surface::objectSpaceBoundingBox() const {
    AABox b;
    getObjectSpaceBoundingBox(b);
    return b;
}


void Surface::getWorldSpaceBoundingBox(AABox& box) const {
    getObjectSpaceBoundingBox(box);
    if (! box.isFinite()) {
        box = AABox::inf();
    } else {
        CoordinateFrame C;
        getCoordinateFrame(C);
        const Box& temp = C.toWorldSpace(box);        
        temp.getBounds(box);
    }
}


AABox Surface::worldSpaceBoundingBox() const {
    AABox b;
    getWorldSpaceBoundingBox(b);
    return b;
}


void Surface::getObjectSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize) const {
    const MeshAlg::Geometry& geometry = objectSpaceGeometry();
    const Array<MeshAlg::Face>& faceArray = faces();

    MeshAlg::computeFaceNormals(geometry.vertexArray, faceArray, faceNormals, normalize);
}


void Surface::getWorldSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize) const {
    MeshAlg::Geometry geometry;
    getWorldSpaceGeometry(geometry);

    const Array<MeshAlg::Face>& faceArray = faces();

    MeshAlg::computeFaceNormals(geometry.vertexArray, faceArray, faceNormals, normalize);
}


void Surface::renderNonShadowed(
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


void Surface::renderShadowedLightPass(
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


void Surface::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const ShadowMap::Ref& shadowMap) const {

    renderShadowMappedLightPass(rd, light, shadowMap->biasedLightMVP(), shadowMap->depthTexture());
}


void Surface::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const Matrix4& lightMVP,
    const Texture::Ref& shadowMap) const {

    rd->pushState();
    {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->configureShadowMap(1, lightMVP, shadowMap);
        rd->setLight(0, light);
        rd->enableLighting();
        rd->setAmbientLightColor(Color3::black());
        render(rd);
    }
    rd->popState();
}

   
void Surface::defaultRender(RenderDevice* rd) const {
    const MeshAlg::Geometry& geometry = objectSpaceGeometry();

    VertexBufferRef area = VertexBuffer::create(sizeof(Vector3)*2*geometry.vertexArray.size() + 16);

    rd->pushState();
        rd->setObjectToWorldMatrix(coordinateFrame());
        rd->beginIndexedPrimitives();
            rd->setNormalArray(VertexRange(geometry.normalArray, area));
            rd->setVertexArray(VertexRange(geometry.vertexArray, area));
            rd->sendIndices(PrimitiveType::TRIANGLES, triangleIndices());
        rd->endIndexedPrimitives();
    rd->popState();
}


void Surface::render(RenderDevice* rd) const {
    defaultRender(rd);
}

void Surface::sendGeometry(RenderDevice* rd) const {
    const MeshAlg::Geometry& geom = objectSpaceGeometry();

    size_t s = sizeof(Vector3) * geom.vertexArray.size() * 2;

    if (hasTexCoords()) {
        s += sizeof(Vector2) * texCoords().size();
    }

    VertexBufferRef area = VertexBuffer::create(s);
    VertexRange vertex(geom.vertexArray, area);
    VertexRange normal(geom.normalArray, area);
    VertexRange texCoord;

    if (hasTexCoords()) {
        texCoord = VertexRange(texCoords(), area);
    }

    rd->beginIndexedPrimitives();
        rd->setVertexArray(vertex);
        rd->setNormalArray(normal);
        if (hasTexCoords()) {
            rd->setTexCoordArray(0, texCoord);
        }
        rd->sendIndices(PrimitiveType::TRIANGLES, triangleIndices());
    rd->endIndexedPrimitives();
}



void Surface::renderTransparents
(RenderDevice*                  rd,
 const Array<Surface::Ref>&  modelArray,
 const Lighting::Ref&           lighting,
 const Array<SuperShader::PassRef>& extraAdditivePasses,
 const Array<ShadowMap::Ref>&   shadowMapArray,
 RefractionQuality              maxRefractionQuality) {

    rd->pushState();

    debugAssertGLOk();
    
    Framebuffer::Ref fbo = rd->framebuffer();
    const ImageFormat* screenFormat = rd->colorFormat();
    if (fbo.isNull()) {
        rd->setReadBuffer(RenderDevice::READ_BACK);
    } else {
        Framebuffer::Attachment::Ref screen = fbo->get(Framebuffer::COLOR0);
        debugAssertM(screen.notNull(), "No color attachment on framebuffer");
        rd->setReadBuffer(RenderDevice::READ_COLOR0);
    }
    debugAssertGLOk();

    static bool supportsRefract = Shader::supportsPixelShaders() && GLCaps::supports_GL_ARB_texture_non_power_of_two();

    static Texture::Ref refractBackground;
    static Shader::Ref refractShader;

    bool didReadback = false;

    const CFrame& cameraFrame = rd->cameraToWorldMatrix();
    
    // Standard G3D::SuperShader transparent rendering
    // Disable depth write so that precise ordering doesn't matter
    rd->setDepthWrite(false);

    // Transparent, must be rendered from back to front
    for (int m = 0; m < modelArray.size(); ++m) {
        Surface::Ref model = modelArray[m];
        SuperSurface::Ref gmodel = model.downcast<SuperSurface>();

        if (gmodel.notNull() && supportsRefract) {
            const float eta = gmodel->gpuGeom()->material->bsdf()->etaTransmit();

            if ((eta > 1.01f) && 
                (gmodel->gpuGeom()->refractionHint == RefractionQuality::DYNAMIC_FLAT) &&
                (maxRefractionQuality >= RefractionQuality::DYNAMIC_FLAT)) {

                if (! didReadback) {
                    if (refractBackground.isNull()) {
                        refractBackground = Texture::createEmpty("Background", rd->width(), rd->height(), screenFormat, Texture::DIM_2D_NPOT, Texture::Settings::video());
                    }

                    refractBackground->copyFromScreen(rd->viewport(), screenFormat);
                    didReadback = true;
                }

                if (refractShader.isNull()) {
                    refractShader = Shader::fromFiles(System::findDataFile("SS_Refract.vrt"), 
                                                      System::findDataFile("SS_Refract.pix"));
                }
                
                // Perform refraction.  The above test ensures that the
                // back-side of a surface (e.g., glass-to-air interface)
                // does not perform refraction
                rd->pushState();
                {
                    Sphere bounds3D = gmodel->worldSpaceBoundingSphere();
                    
                    // Estimate of distance from object to background to
                    // be constant (we could read back depth buffer, but
                    // that won't produce frame coherence)
                    
                    const float z0 = max(8.0f - (eta - 1.0f) * 5.0f, bounds3D.radius);
                    const float backZ = cameraFrame.pointToObjectSpace(bounds3D.center).z - z0;
                    refractShader->args.set("backZ", backZ);
                    
                    // Assume rays are leaving air
                    refractShader->args.set("etaRatio", 1.0f / eta);
                    
                    // Find out how big the back plane is in meters
                    float backPlaneZ = min(-0.5f, backZ);
                    GCamera cam2 = rd->projectionAndCameraMatrix();
                    cam2.setFarPlaneZ(backPlaneZ);
                    Vector3 ur, ul, ll, lr;
                    cam2.getFarViewportCorners(rd->viewport(), ur, ul, ll, lr);
                    Vector2 backSizeMeters((ur - ul).length(), (ur - lr).length());
                    refractShader->args.set("backSizeMeters", backSizeMeters);
                    
                    // Always use the same background image, rendered
                    // before transparents.  This approach is enough
                    // of an approximation that we can't expect a ray
                    // passing through two transparents to be
                    // reasonable anyway
                    refractShader->args.set("background", refractBackground);
                    rd->setShader(refractShader);        
                    rd->setObjectToWorldMatrix(model->coordinateFrame());
                    gmodel->sendGeometry(rd);
                }
                rd->popState();
            }
        }

        model->renderNonShadowed(rd, lighting);
        debugAssert(lighting->shadowedLightArray.size() <= shadowMapArray.size());
        for (int L = 0; L < lighting->shadowedLightArray.size(); ++L) {
            model->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[L], 
                                               shadowMapArray[L]);
        }
        for (int p = 0; p < extraAdditivePasses.size(); ++p) {
            model->renderSuperShaderPass(rd, extraAdditivePasses[p]);
        }
    }

    // TODO: Depth write so that z-buffer is up to date
    rd->popState();
}
////////////////////////////////////////////////////////////////////////////////////////

static bool depthGreaterThan(const Surface2DRef& a, const Surface2DRef& b) {
    return a->depth() > b->depth();
}

void Surface2D::sort(Array<Surface2DRef>& array) {
    array.sort(depthGreaterThan);
}

}

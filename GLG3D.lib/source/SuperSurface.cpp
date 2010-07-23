/**
 @file SuperSurface.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2004-11-20
  @edited  2009-02-20

  Copyright 2001-2010, Morgan McGuire
 */
#include "G3D/Log.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Lighting.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"

namespace G3D {

/** For fixed function, we detect
    reflection as having no glossy map but a packed specular
   exponent of 1 (=infinity).*/   
static bool mirrorReflectiveFF(const SuperBSDF::Ref& bsdf) { 
    return 
        (bsdf->specular().factors() == Component4::CONSTANT) &&
        (SuperBSDF::packedSpecularMirror() == bsdf->specular().constant().a);
}

/** For fixed function, we detect glossy
    reflection as having a packed specular exponent less 
    than 1.*/   
static bool glossyReflectiveFF(const SuperBSDF::Ref& bsdf) {
     return (bsdf->specular().constant().a != SuperBSDF::packedSpecularMirror()) && 
            (bsdf->specular().constant().a != SuperBSDF::packedSpecularNone());
}


bool SuperSurface::depthWriteHint(float distanceToCamera) const {
    const float d = m_gpuGeom->material->depthWriteHintDistance();
    if (isNaN(d)) {
        return ! hasTransmission();
    } else {
        return distanceToCamera < d;
    }
}


void SuperSurface::sortFrontToBack(Array<SuperSurface::Ref>& a, const Vector3& v) {
    Array<Surface::Ref> s;
    s.resize(a.size());
    for (int i = 0; i < s.size(); ++i) {
        s[i] = a[i];
    }
    Surface::sortFrontToBack(s, v);
    for (int i = 0; i < s.size(); ++i) {
        a[i] = s[i].downcast<SuperSurface>();
    }
}

SuperSurface::Ref SuperSurface::create
(const std::string&       name,
 const CFrame&            frame, 
 const GPUGeom::Ref&      gpuGeom,
 const CPUGeom&           cpuGeom,
 const ReferenceCountedPointer<ReferenceCountedObject>& source) {
    debugAssert(gpuGeom.notNull());


    // Cannot check if the gpuGeom is valid because it might not be filled out yet
    return new SuperSurface(name, frame, gpuGeom, cpuGeom, source);
}


static void setAdditive(RenderDevice* rd, bool& additive);

int SuperSurface::debugNumSendGeometryCalls = 0;

// Static to this file, not the class
static SuperSurface::GraphicsProfile graphicsProfile = SuperSurface::UNKNOWN;

void SuperSurface::setProfile(GraphicsProfile p) {
    graphicsProfile = p;
}


SuperSurface::GraphicsProfile SuperSurface::profile() {

    if (graphicsProfile == UNKNOWN) {
        if (GLCaps::supports_GL_ARB_shader_objects()) {
            graphicsProfile = PS20;

            
            if (System::findDataFile("SS_NonShadowedPass.vrt") == "") {
                graphicsProfile = UNKNOWN;
                logPrintf("\n\nWARNING: SuperSurface could not enter PS20 mode because"
                          "NonShadowedPass.vrt was not found.\n\n");
            }
        }

        
        if (graphicsProfile == UNKNOWN) {
            if (GLCaps::supports("GL_ARB_texture_env_crossbar") &&
                GLCaps::supports("GL_ARB_texture_env_combine") &&
                GLCaps::supports("GL_EXT_texture_env_add") &&
                (GLCaps::numTextureUnits() >= 4)) {
                graphicsProfile = PS14;
            } else {
                graphicsProfile = FIXED_FUNCTION;
            }
        }
    }

    return graphicsProfile;
}


const char* toString(SuperSurface::GraphicsProfile p) {
    switch (p) {
    case SuperSurface::UNKNOWN:
        return "Unknown";

    case SuperSurface::FIXED_FUNCTION:
        return "Fixed Function";

    case SuperSurface::PS14:
        return "PS 1.4";

    case SuperSurface::PS20:
        return "PS 2.0";

    default:
        return "Error!";
    }
}


void SuperSurface::renderNonShadowed(
    const Array<Surface::Ref>&      posedArray, 
    RenderDevice*                   rd, 
    const LightingRef&              lighting,
    bool                            preserveState) {

    if (posedArray.size() == 0) {
        return;
    }

    if (! rd->depthWrite() && ! rd->colorWrite()) {
        // Nothing to draw!
        return;
    }

    RenderDevice::BlendFunc srcBlend;
    RenderDevice::BlendFunc dstBlend;
    RenderDevice::BlendEq   blendEq;
    rd->getBlendFunc(srcBlend, dstBlend, blendEq);

    if (preserveState) {
        rd->pushState();
    }
        bool originalDepthWrite = rd->depthWrite();

        // Lighting will be turned on and off by subroutines
        rd->disableLighting();

        const bool ps20 = SuperSurface::profile() == SuperSurface::PS20;

        for (int p = 0; p < posedArray.size(); ++p) {
            const SuperSurface::Ref& posed = posedArray[p].downcast<SuperSurface>();

            if (! rd->colorWrite()) {
                // No need for fancy shading, just send geometry
                posed->sendGeometry2(rd);
                continue;
            }

            const Material::Ref& material = posed->m_gpuGeom->material;
            const SuperBSDF::Ref& bsdf = material->bsdf();
            (void)material;
            (void)bsdf;
            debugAssertM(bsdf->transmissive().isBlack(), 
                "Transparent object passed through the batch version of "
                "SuperSurface::renderNonShadowed, which is intended exclusively for opaque objects.");

            if (posed->m_gpuGeom->twoSided) {
                if (! ps20) {
                    rd->enableTwoSidedLighting();
                    rd->setCullFace(RenderDevice::CULL_NONE);
                } else {
                    // Even if back face culling is reversed, for two-sided objects 
                    // we always draw the front.
                    rd->setCullFace(RenderDevice::CULL_BACK);
                }
            }
            bool wroteDepth = posed->renderNonShadowedOpaqueTerms(rd, lighting, false);

            if (posed->m_gpuGeom->twoSided && ps20) {
                // gl_FrontFacing doesn't work on most cards inside
                // the shader, so we have to draw two-sided objects
                // twice
                rd->setCullFace(RenderDevice::CULL_FRONT);
                
                wroteDepth = posed->renderNonShadowedOpaqueTerms(rd, lighting, false) || wroteDepth;
            }

            rd->setDepthWrite(originalDepthWrite);
            if (! wroteDepth && originalDepthWrite) {
                // We failed to write to the depth buffer, so
                // do so now.
                rd->disableLighting();
                rd->setColor(Color3::black());
                if (posed->m_gpuGeom->twoSided) {
                    rd->setCullFace(RenderDevice::CULL_NONE);
                }
                posed->sendGeometry2(rd);
                rd->enableLighting();
            }

            if (posed->m_gpuGeom->twoSided) {
                rd->disableTwoSidedLighting();
                rd->setCullFace(RenderDevice::CULL_BACK);
            }


            // Alpha blend will be changed by subroutines so we restore it for each object
            rd->setBlendFunc(srcBlend, dstBlend, blendEq);
            rd->setDepthWrite(originalDepthWrite);
        }

    if (preserveState) {
        rd->popState();
    }
}


void SuperSurface::renderShadowMappedLightPass
(const Array<Surface::Ref>&     posedArray, 
 RenderDevice*                  rd, 
 const GLight&                  light, 
 const ShadowMap::Ref&          shadowMap,
 bool                           preserveState) {
    
    if (posedArray.size() == 0) {
        return;
    }

    RenderDevice::CullFace oldCullFace = RenderDevice::CULL_NONE;
    RenderDevice::BlendFunc oldSrcBlendFunc = RenderDevice::BLEND_ONE, oldDstBlendFunc = RenderDevice::BLEND_ONE;
    RenderDevice::BlendEq oldBlendEq = RenderDevice::BLENDEQ_MIN;
    if (preserveState) {
        rd->pushState();
    } else {
        oldCullFace = rd->cullFace();
        rd->getBlendFunc(oldSrcBlendFunc, oldDstBlendFunc, oldBlendEq);
    }
    {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        rd->setCullFace(RenderDevice::CULL_BACK);

        for (int i = 0; i < posedArray.size(); ++i) {
            const SuperSurface::Ref&    posed    = posedArray[i].downcast<SuperSurface>();
            const Material::Ref&        material = posed->m_gpuGeom->material;
            const SuperBSDF::Ref&       bsdf     = material->bsdf();

            if (bsdf->lambertian().isBlack() && bsdf->specular().isBlack()) {
                // Nothing to draw for this object
                continue;
            }

            // This switch could go outside the loop, however doing so would lead to code repetition
            // without any real increase in performance.

            switch (profile()) {
            case PS14:
                // Intentionally fall through; there is no
                // optimized PS14 path for this function.

            case FIXED_FUNCTION:
                if (posed->m_gpuGeom->twoSided) {
                    rd->enableTwoSidedLighting();
                    rd->setCullFace(RenderDevice::CULL_NONE);
                }

                posed->renderFFShadowMappedLightPass(rd, light, shadowMap);

                if (posed->m_gpuGeom->twoSided) {
                    rd->disableTwoSidedLighting();
                    rd->setCullFace(RenderDevice::CULL_BACK);
                }
                break;

            case PS20:
                // Even if back face culling is reversed, for two-sided objects 
                // we always draw the front first.
                rd->setCullFace(RenderDevice::CULL_BACK);

                posed->renderPS20ShadowMappedLightPass(rd, light, shadowMap);

                if (posed->m_gpuGeom->twoSided) {
                    // The GLSL built-in gl_FrontFacing does not work on most cards, so we have to draw 
                    // two-sided objects twice since there is no way to distinguish them in the shader.
                    rd->setCullFace(RenderDevice::CULL_FRONT);
                    posed->renderPS20ShadowMappedLightPass(rd, light, shadowMap);
                    rd->setCullFace(RenderDevice::CULL_BACK);
                }
                break;

            default:
                debugAssertM(false, "Fell through switch");
            }
        }
    }
    if (preserveState) {
        rd->popState();
    } else {
        rd->setCullFace(oldCullFace);
        rd->setBlendFunc(oldSrcBlendFunc, oldDstBlendFunc, oldBlendEq);
    }
 }


void SuperSurface::extract(
    Array<Surface::Ref>&   all, 
    Array<Surface::Ref>&   super) {
    
    for (int i = 0; i < all.size(); ++i) {
        ReferenceCountedPointer<SuperSurface> m = 
            dynamic_cast<SuperSurface*>(all[i].pointer());

        if (m.notNull()) {
            // This is a most-derived subclass and is opaque

            super.append(m);
            all.fastRemove(i);
            // Iterate over again
            --i;
        }
    }
}


/////////////////////////////////////////////////////////////////

/** PS14 often needs a dummy texture map in order to enable a combiner */
static Texture::Ref whiteMap() {
    static Texture::Ref map;

    if (map.isNull()) {
        GImage im(4,4,3);
        for (int y = 0; y < im.height(); ++y) {
            for (int x = 0; x < im.width(); ++x) {
                im.pixel3(x, y) = Color3(1, 1, 1);
            }
        }
        map = Texture::fromGImage("White", im, ImageFormat::RGB8());
    }
    return map;
}


void SuperSurface::render(RenderDevice* rd) const {

    // Infer the lighting from the fixed function

    // Avoid allocating memory for each render
    static LightingRef lighting = Lighting::create();

    rd->getFixedFunctionLighting(lighting);

    renderNonShadowed(rd, lighting);
}


bool SuperSurface::renderSuperShaderPass(
    RenderDevice* rd, 
    const SuperShader::PassRef& pass) const {

    if (m_gpuGeom->twoSided) {
        // We're going to render the front and back faces separately.
        rd->setCullFace(RenderDevice::CULL_FRONT);
        rd->setShader(pass->getConfiguredShader(*m_gpuGeom->material, rd->cullFace()));
        sendGeometry2(rd);
    }

    rd->setCullFace(RenderDevice::CULL_BACK);
    rd->setShader(pass->getConfiguredShader(*m_gpuGeom->material, rd->cullFace()));
    sendGeometry2(rd);

    return false;
}


/** 
 Switches to additive rendering, if not already in that mode.
 */
static void setAdditive(RenderDevice* rd, bool& additive) {
    if (! additive) {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->setDepthWrite(false);
        additive = true;
    }
}


bool SuperSurface::renderNonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting,
    bool preserveState) const {

    bool renderedOnce = false;

    switch (profile()) {
    case FIXED_FUNCTION:
        renderedOnce = renderFFNonShadowedOpaqueTerms(rd, lighting);
        break;

    case PS14:
        if (preserveState) {
            rd->pushState();
        }
        renderedOnce = renderPS14NonShadowedOpaqueTerms(rd, lighting);
        if (preserveState) {
            rd->popState();
        }
        break;

    case PS20:
        if (preserveState) {
            rd->pushState();
        }
        renderedOnce = renderPS20NonShadowedOpaqueTerms(rd, lighting);
        if (preserveState) {
            rd->popState();
        }
        break;

    default:
        debugAssertM(false, "Fell through switch");
    }        

    return renderedOnce;
}


bool SuperSurface::renderPS20NonShadowedOpaqueTerms(
    RenderDevice*                           rd,
    const Lighting::Ref&                    lighting) const {

    const Material::Ref& material = m_gpuGeom->material;
    const SuperBSDF::Ref&     bsdf = material->bsdf();


    RenderDevice::BlendFunc srcBlend;
    RenderDevice::BlendFunc dstBlend;
    RenderDevice::BlendEq   blendEq;
    rd->getBlendFunc(srcBlend, dstBlend, blendEq);

    // Note that partial coverage surfaces must always be rendered opaquely, even if black, because
    // the alpha value may affect the image.
    if (! ((bsdf->hasReflection() || (srcBlend != RenderDevice::BLEND_ONE))
        || (m_gpuGeom->material->emissive().notBlack() && ! lighting->emissiveScale.isZero()))) {
        // Nothing to draw
        return false;
    }

    int numLights = lighting->lightArray.size();

    if (numLights <= SuperShader::NonShadowedPass::LIGHTS_PER_PASS) {
        
        SuperShader::NonShadowedPass::instance()->setLighting(lighting);
        rd->setShader(SuperShader::NonShadowedPass::instance()->getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));

        sendGeometry2(rd);

    } else {

        // SuperShader only supports SuperShader::NonShadowedPass::LIGHTS_PER_PASS lights, so we have to make multiple passes
        LightingRef reducedLighting = lighting->clone();

        Array<GLight> lights(lighting->lightArray);

        Sphere myBounds = worldSpaceBoundingSphere();
        // Remove lights that cannot affect this object
        for (int L = 0; L < lights.size(); ++L) {
            Sphere s = lights[L].effectSphere();
            if (! s.intersects(myBounds)) {
                // This light does not affect this object
                lights.fastRemove(L);
                --L;
            }
        }
        numLights = lights.size();

        // Number of lights to use
        int x = iMin(SuperShader::NonShadowedPass::LIGHTS_PER_PASS, lights.size());

        // Copy the lights into the reduced lighting structure
        reducedLighting->lightArray.resize(x);
        SuperShader::NonShadowedPass::instance()->setLighting(reducedLighting);
        rd->setShader(SuperShader::NonShadowedPass::instance()->getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));
        sendGeometry2(rd);

        if (numLights > SuperShader::NonShadowedPass::LIGHTS_PER_PASS) {
            // Add extra lighting terms
            rd->pushState();
            rd->setBlendFunc(RenderDevice::BLEND_CURRENT, RenderDevice::BLEND_ONE);
            rd->setDepthWrite(false);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            for (int L = SuperShader::NonShadowedPass::LIGHTS_PER_PASS; 
                 L < numLights; 
                 L += SuperShader::ExtraLightPass::LIGHTS_PER_PASS) {

                SuperShader::ExtraLightPass::instance()->setLighting(lighting->lightArray, L);
                rd->setShader(SuperShader::ExtraLightPass::instance()->
                              getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));
                sendGeometry2(rd);
            }
            rd->popState();
        }
    }

    return true;
}


bool SuperSurface::renderFFNonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting) const {

    debugAssertGLOk();

    bool renderedOnce = false;

    const Material::Ref& material = m_gpuGeom->material;
    const SuperBSDF::Ref&     bsdf = material->bsdf();


    // Emissive
    if (! material->emissive().isBlack()) {
        rd->setColor(material->emissive().constant());
        rd->setTexture(0, material->emissive().texture());
        sendGeometry2(rd);
        setAdditive(rd, renderedOnce);
    }

    // Add reflective.  

    if (! mirrorReflectiveFF(bsdf) &&
        lighting.notNull() &&
        (lighting->environmentMapColor != Color3::black())) {

        rd->pushState();

            // Reflections are specular and not affected by surface texture, only
            // the reflection coefficient
            rd->setColor(bsdf->specular().constant().rgb() * lighting->environmentMapColor);

            // TODO: Use diffuse alpha map here somehow?
            rd->setTexture(0, NULL);

            // Configure reflection map
            if (lighting->environmentMap.isNull()) {
                rd->setTexture(1, NULL);
            } else if (GLCaps::supports_GL_ARB_texture_cube_map() &&
                (lighting->environmentMap->dimension() == Texture::DIM_CUBE_MAP)) {
                rd->configureReflectionMap(1, lighting->environmentMap);
            } else {
                // Use the top texture as a sphere map
                glActiveTextureARB(GL_TEXTURE0_ARB + 1);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);

                rd->setTexture(1, lighting->environmentMap);
            }

            sendGeometry2(rd);
            setAdditive(rd, renderedOnce);

            // Disable reflection map
            rd->setTexture(1, NULL);
        rd->popState();
    }
    debugAssertGLOk();

    // Add ambient + lights
    if (bsdf->lambertian().factors() != Component4::BLACK || 
        bsdf->specular().factors() != Component4::BLACK) {
        rd->enableLighting();
        rd->setTexture(0, bsdf->lambertian().texture());
        rd->setColor(bsdf->lambertian().constant());

        // Fixed function does not receive specular texture maps, only constants.
        rd->setSpecularCoefficient(bsdf->specular().constant().rgb());
        rd->setShininess(SuperBSDF::unpackSpecularExponent(bsdf->specular().constant().a));
        debugAssertGLOk();

        // Ambient
        if (lighting.notNull()) {
            rd->setAmbientLightColor(lighting->ambientTop);
            if (lighting->ambientBottom != lighting->ambientTop) {
                rd->setLight(0, GLight::directional(-Vector3::unitY(), 
                    lighting->ambientBottom - lighting->ambientTop, false)); 
            }
            
            // Lights
            for (int L = 0; L < iMin(7, lighting->lightArray.size()); ++L) {
                rd->setLight(L + 1, lighting->lightArray[L]);
            }
        }
        debugAssertGLOk();

        if (renderedOnce) {
            // Make sure we add this pass to the previous already-rendered terms
            rd->setBlendFunc(RenderDevice::BLEND_CURRENT, RenderDevice::BLEND_ONE);
        }

        sendGeometry2(rd);
        renderedOnce = true;
        rd->disableLighting();
    }

    return renderedOnce;
}


bool SuperSurface::renderPS14NonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting) const {

    bool renderedOnce = false;
    const SuperBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    // Emissive
    if (! m_gpuGeom->material->emissive().factors() != Component3::BLACK) {
        rd->disableLighting();
        rd->setColor(m_gpuGeom->material->emissive().constant());
        rd->setTexture(0, m_gpuGeom->material->emissive().texture());
        sendGeometry2(rd);
        setAdditive(rd, renderedOnce);
    }
    
    // Full combiner setup (in practice, we only use combiners that are
    // needed):
    //
    // Unit 0:
    // Mode = modulate
    // arg0 = primary
    // arg1 = texture (diffuse)
    //
    // Unit 1:
    // Mode = modulate
    // arg0 = constant (envmap constant * envmap color)
    // arg1 = texture (envmap)
    //
    // Unit 2:
    // Mode = modulate
    // arg0 = previous
    // arg1 = texture (reflectmap)
    //
    // Unit 3:
    // Mode = add
    // arg0 = previous
    // arg1 = texture0


    bool hasDiffuse = bsdf->lambertian().notBlack() || bsdf->lambertian().nonUnitAlpha();
    bool hasReflection = mirrorReflectiveFF(bsdf) && 
            lighting.notNull() &&
            (lighting->environmentMapColor != Color3::black());

    bool hasGlossy = glossyReflectiveFF(bsdf);

    // Add reflective and diffuse

    // We're going to use combiners, which G3D does not preserve
    glPushAttrib(GL_TEXTURE_BIT);
    rd->pushState();

        GLint nextUnit = 0;
        GLint diffuseUnit = GL_PRIMARY_COLOR_ARB;

        if (hasDiffuse) {

            // Add ambient + lights
            rd->enableLighting();
            if (bsdf->lambertian().notBlack() || hasGlossy || bsdf->lambertian().nonUnitAlpha()) {
                rd->setTexture(nextUnit, bsdf->lambertian().texture());
                rd->setColor(bsdf->lambertian().constant());

                // Fixed function does not receive specular texture maps, only constants.
                rd->setSpecularCoefficient(bsdf->specular().constant().rgb());
                rd->setShininess(SuperBSDF::unpackSpecularExponent(bsdf->specular().constant().a));

                // Ambient
                if (lighting.notNull()) {
                    rd->setAmbientLightColor(lighting->ambientTop);
                    if (lighting->ambientBottom != lighting->ambientTop) {
                        rd->setLight(0, GLight::directional(-Vector3::unitY(), 
                            lighting->ambientBottom - lighting->ambientTop, false)); 
                    }
            
                    // Lights
                    for (int L = 0; L < iMin(8, lighting->lightArray.size()); ++L) {
                        rd->setLight(L + 1, lighting->lightArray[L]);
                    }
                }
            }

            if (bsdf->lambertian().texture().notNull()) {
                glActiveTextureARB(GL_TEXTURE0_ARB + nextUnit);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,   GL_MODULATE);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,   GL_PRIMARY_COLOR_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,  GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,   GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,  GL_SRC_COLOR);
                diffuseUnit = GL_TEXTURE0_ARB + nextUnit;
                ++nextUnit;
            }
        }

        if (hasReflection) {

            // First configure the reflection map.  There must be one or we wouldn't
            // have taken this branch.

            alwaysAssertM(lighting->environmentMap.notNull(), "Null Lighting::environmentMap");
            if (GLCaps::supports_GL_ARB_texture_cube_map() &&
                (lighting->environmentMap->dimension() == Texture::DIM_CUBE_MAP)) {
                rd->configureReflectionMap(nextUnit, lighting->environmentMap);
            } else {
                // Use the top texture as a sphere map
                glActiveTextureARB(GL_TEXTURE0_ARB + nextUnit);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);

                rd->setTexture(nextUnit, lighting->environmentMap);
            }
            debugAssertGLOk();

            glActiveTextureARB(GL_TEXTURE0_ARB + nextUnit);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE_ARB);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,   GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,   GL_CONSTANT_ARB);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,  GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,   GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,  GL_SRC_COLOR);
            Color4 c(bsdf->specular().constant().rgb() * lighting->environmentMapColor, 1);
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, reinterpret_cast<const float*>(&c));
                
            debugAssertGLOk();

            ++nextUnit;

            rd->setTexture(nextUnit, bsdf->specular().texture());
            if (bsdf->specular().texture().notNull()) {
                // If there is a reflection map for the surface, modulate
                // the reflected color by it.
                glActiveTextureARB(GL_TEXTURE0_ARB + nextUnit);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,   GL_MODULATE);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,   GL_PREVIOUS_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,  GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,   GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,  GL_SRC_COLOR);
                ++nextUnit;
                debugAssertGLOk();
            }

            if (hasDiffuse) {
                // Need a dummy texture
                rd->setTexture(nextUnit, whiteMap());

                // Add diffuse to the previous (reflective) unit
                glActiveTextureARB(GL_TEXTURE0_ARB + nextUnit);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,   GL_ADD);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,   GL_PREVIOUS_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,  GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,   diffuseUnit);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,  GL_SRC_COLOR);
                debugAssertGLOk();
                ++nextUnit;
            }
        }

        sendGeometry2(rd);
        setAdditive(rd, renderedOnce);

    rd->popState();
    glPopAttrib();

    return renderedOnce;
}


void SuperSurface::renderNonShadowed
(RenderDevice*                   rd,
 const LightingRef&              lighting) const {

    // The transparent rendering path is not optimized to amortize state changes because 
    // it is only called by the single-object version of this function.  Only
    // opaque objects are batched together.
    const SuperBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    if (hasTransmission()) {
        rd->pushState();
            // Transparent
            bool oldDepthWrite = rd->depthWrite();

            // Render backfaces first, and then front faces.  Each will be culled exactly
            // once, so we aren't going to overdraw.
            int passes = m_gpuGeom->twoSided ? 2 : 1;

            if (m_gpuGeom->twoSided) {
                // We're going to render the front and back faces separately.
                rd->setCullFace(RenderDevice::CULL_BACK);
            }

            for (int i = 0; i < passes; ++i) {
                // Modulate background by transparent color
                // TODO: Fresnel for shader-driven pipeline
                rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_SRC_COLOR);
                rd->setTexture(0, bsdf->transmissive().texture());
                rd->setColor(bsdf->transmissive().constant());
                sendGeometry2(rd);

                // Draw normal terms on top
                bool alreadyAdditive = false;
                setAdditive(rd, alreadyAdditive);
                renderNonShadowedOpaqueTerms(rd, lighting, false);
                      
                // restore depth write
                rd->setDepthWrite(oldDepthWrite);
                rd->setCullFace(RenderDevice::CULL_BACK);
            }
        rd->popState();
    } else {

        // This is the unoptimized, single-object version of renderNonShadowed.
        // It just calls the optimized version with a single-element array.

        static Array<Surface::Ref> posedArray;
        posedArray.resize(1);
        posedArray[0] = Ref(const_cast<SuperSurface*>(this));
        renderNonShadowed(posedArray, rd, lighting);
        posedArray.fastClear();
    }
}


void SuperSurface::renderShadowedLightPass(
    RenderDevice*       rd, 
    const GLight&       light) const {

    (void)rd;
    (void)light;
    // TODO
    debugAssertM(false, "Unimplemented");
}


void SuperSurface::renderShadowMappedLightPass(
    RenderDevice*       rd, 
    const GLight&       light, 
    const Matrix4&      lightMVP, 
    const Texture::Ref&   shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.
    alwaysAssertM(false, "Deprecated: use the method that takes a ShadowMap");
}


void SuperSurface::renderShadowMappedLightPass(
    RenderDevice*         rd, 
    const GLight&         light, 
    const ShadowMap::Ref& shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.

    static Array<Surface::Ref> posedArray;

    posedArray.resize(1);
    posedArray[0] = Ref(const_cast<SuperSurface*>(this));
    renderShadowMappedLightPass(posedArray, rd, light, shadowMap);
    posedArray.fastClear();
}


void SuperSurface::renderPS20ShadowMappedLightPass(
    RenderDevice*       rd,
    const GLight&       light, 
    const ShadowMap::Ref& shadowMap) const {

    SuperShader::ShadowedPass::instance()->setLight(light, shadowMap);
    renderSuperShaderPass(rd, SuperShader::ShadowedPass::instance());
}


void SuperSurface::renderFFShadowMappedLightPass(
    RenderDevice*       rd,
    const GLight&       light, 
    const ShadowMap::Ref& shadowMap) const {

    rd->configureShadowMap(1, shadowMap);

    rd->setObjectToWorldMatrix(m_frame);

    const SuperBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    rd->setTexture(0, bsdf->lambertian().texture());
    rd->setColor(bsdf->lambertian().constant());

    // We disable specular highlights because they will not be modulated
    // by the shadow map.  We then make a separate pass to render specular
    // highlights.
    rd->setSpecularCoefficient(Color3::zero());

    rd->enableLighting();
    rd->setAmbientLightColor(Color3::black());

    rd->setLight(0, light);

    sendGeometry2(rd);

    if (glossyReflectiveFF(bsdf)) {
        // Make a separate pass for specular. 
        static bool separateSpecular = GLCaps::supports_GL_EXT_separate_specular_color();

        if (separateSpecular) {
            // We disable the OpenGL separate
            // specular behavior so that the texture will modulate the specular
            // pass, and then put the specularity coefficient in the texture.
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SINGLE_COLOR_EXT);
        }

        rd->setColor(Color3::white()); // TODO: when I put the specular coefficient here, it doesn't modulate.  What's wrong?
        rd->setTexture(0, bsdf->specular().texture());
        rd->setSpecularCoefficient(bsdf->specular().constant().rgb());

        // Turn off the diffuse portion of this light
        GLight light2 = light;
        light2.diffuse = false;
        rd->setLight(0, light2);
        rd->setShininess(SuperBSDF::unpackSpecularExponent(bsdf->specular().constant().a));

        sendGeometry2(rd);

        if (separateSpecular) {
            // Restore normal behavior
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, 
                          GL_SEPARATE_SPECULAR_COLOR_EXT);
        }

        // TODO: use this separate specular pass code in all fixed function 
        // cases where there is a specularity map.
    }
}


void SuperSurface::sendGeometry2(
    RenderDevice*           rd) const {

    debugAssertGLOk();

    CoordinateFrame o2w = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(m_frame);

    rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
    sendGeometry(rd);

    rd->setObjectToWorldMatrix(o2w);

}


void SuperSurface::sendGeometry(
    RenderDevice*           rd) const {
    debugAssertGLOk();
    ++debugNumSendGeometryCalls;

    // TODO: Radeon mobility cards crash rendering VertexRange in wireframe mode.
    // switch to begin/end in that case
    bool useVAR = true || (rd->renderMode() == RenderDevice::RENDER_SOLID);

    if (useVAR) {

        debugAssertGLOk();
        rd->beginIndexedPrimitives();
        {
            rd->setVertexArray(m_gpuGeom->vertex);
            debugAssertGLOk();

            debugAssert(m_gpuGeom->normal.valid());
            rd->setNormalArray(m_gpuGeom->normal);
            debugAssertGLOk();

            if (m_gpuGeom->texCoord0.valid() && (m_gpuGeom->texCoord0.size() > 0)){
                rd->setTexCoordArray(0, m_gpuGeom->texCoord0);
                debugAssertGLOk();
            }

            // In programmable pipeline mode, load the tangents into tex coord 1
            if (m_gpuGeom->packedTangent.valid() && (profile() == PS20) && (m_gpuGeom->packedTangent.size() > 0)) {
                rd->setTexCoordArray(1, m_gpuGeom->packedTangent);
                debugAssertGLOk();
            }

            debugAssertGLOk();
            rd->sendIndices((RenderDevice::Primitive)m_gpuGeom->primitive, m_gpuGeom->index);
        }
        rd->endIndexedPrimitives();

    } else {

        debugAssertM(m_cpuGeom.index != NULL, 
                     "This graphics card cannot render in wireframe mode"
                     " without explicit CPU data structures.");

        if (m_cpuGeom.index != NULL) {

            rd->beginPrimitive((RenderDevice::Primitive)m_gpuGeom->primitive);
            for (int i = 0; i < m_cpuGeom.index->size(); ++i) {
                int v = (*m_cpuGeom.index)[i];
                if ((m_cpuGeom.texCoord0 != NULL) &&
                    (m_cpuGeom.texCoord0->size() > 0)) { 
                    rd->setTexCoord(0, (*m_cpuGeom.texCoord0)[v]);
                }
                if ((m_cpuGeom.packedTangent != NULL) &&
                    (m_cpuGeom.packedTangent->size() > 0)) {
                    rd->setTexCoord(1, (*m_cpuGeom.packedTangent)[v]);
                }

                rd->setNormal(m_cpuGeom.geometry->normalArray[v]);
                rd->sendVertex(m_cpuGeom.geometry->vertexArray[v]);
            }
            rd->endPrimitive();
        }
    }
}

std::string SuperSurface::name() const {
    return m_name;
}


bool SuperSurface::hasTransmission() const {
    return ! m_gpuGeom->material->bsdf()->transmissive().isBlack();
}


bool SuperSurface::hasPartialCoverage() const {
    return m_gpuGeom->material->bsdf()->lambertian().nonUnitAlpha();
}


void SuperSurface::getCoordinateFrame(CoordinateFrame& c) const {
    c = m_frame;
}


const MeshAlg::Geometry& SuperSurface::objectSpaceGeometry() const {
    return *(m_cpuGeom.geometry);
}


const Array<Vector4>& SuperSurface::objectSpacePackedTangents() const {
    if (m_cpuGeom.packedTangent == NULL) {
        const static Array<Vector4> x;
        return x;
    } else {
        return *(m_cpuGeom.packedTangent);
    }
}


const Array<Vector3>& SuperSurface::objectSpaceFaceNormals(bool normalize) const {
    static Array<Vector3> n;
    debugAssert(false);
    return n;
    // TODO
}


const Array<MeshAlg::Face>& SuperSurface::faces() const {
    static Array<MeshAlg::Face> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Edge>& SuperSurface::edges() const {
    static Array<MeshAlg::Edge> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Vertex>& SuperSurface::vertices() const {
    static Array<MeshAlg::Vertex> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<Vector2>& SuperSurface::texCoords() const {
    if (m_cpuGeom.texCoord0 == NULL) {
        const static Array<Vector2> x;
        return x;
    } else {
        return *(m_cpuGeom.texCoord0);
    }
}


bool SuperSurface::hasTexCoords() const {
    return (m_cpuGeom.texCoord0 != 0) && (m_cpuGeom.texCoord0->size() > 0);
}


const Array<MeshAlg::Face>& SuperSurface::weldedFaces() const {
    static Array<MeshAlg::Face> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Edge>& SuperSurface::weldedEdges() const {
    static Array<MeshAlg::Edge> e;
    debugAssert(false);
    return e;
    // TODO
}


const Array<MeshAlg::Vertex>& SuperSurface::weldedVertices() const {
    static Array<MeshAlg::Vertex> v;
    return v;
    debugAssert(false);
    // TODO
}


const Array<int>& SuperSurface::triangleIndices() const {
    alwaysAssertM(m_gpuGeom->primitive == PrimitiveType::TRIANGLES, "This model is not composed of triangles."); 
    return *(m_cpuGeom.index);
}


void SuperSurface::getObjectSpaceBoundingSphere(Sphere& s) const {
    s = m_gpuGeom->sphereBounds;
}


void SuperSurface::getObjectSpaceBoundingBox(AABox& b) const {
    b = m_gpuGeom->boxBounds;
}


int SuperSurface::numBoundaryEdges() const {
    // TODO
    debugAssert(false);
    return 0;
}


int SuperSurface::numWeldedBoundaryEdges() const {
    // TODO
    return 0;
}


void SuperSurface::CPUGeom::copyVertexDataToGPU
(VertexRange&               vertex, 
 VertexRange&               normal, 
 VertexRange&               packedTangentVAR, 
 VertexRange&               texCoord0VAR, 
 VertexBuffer::UsageHint    hint) {

    int vtxSize = sizeof(Vector3) * geometry->vertexArray.size();
    int texSize = sizeof(Vector2) * texCoord0->size();
    int tanSize = sizeof(Vector4) * packedTangent->size();

    if ((vertex.maxSize() >= vtxSize) &&
        (normal.maxSize() >= vtxSize) &&
        ((tanSize == 0) || (packedTangentVAR.maxSize() >= tanSize)) &&
        ((texSize == 0) || (texCoord0VAR.maxSize() >= texSize))) {
        VertexRange::updateInterleaved
           (geometry->vertexArray,  vertex,
            geometry->normalArray,  normal,
            *packedTangent,         packedTangentVAR,
            *texCoord0,             texCoord0VAR);

    } else {

        // Maximum round-up size of varArea.
        int roundOff = 16;

        // Allocate new VARs
        VertexBuffer::Ref varArea = VertexBuffer::create(vtxSize * 2 + texSize + tanSize + roundOff, hint);
        VertexRange::createInterleaved
            (geometry->vertexArray, vertex,
             geometry->normalArray, normal,
             *packedTangent,        packedTangentVAR,
             *texCoord0,            texCoord0VAR,
             varArea);       
    }
}

}

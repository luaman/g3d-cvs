/**
 @file GenericPosedModel.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2004-11-20
  @edited  2009-02-20

  Copyright 2004-2009, Morgan McGuire
 */
#include "G3D/Log.h"
#include "GLG3D/GenericPosedModel.h"
#include "GLG3D/Lighting.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"

namespace G3D {

/** For fixed function, we detect
    reflection as having no glossy map but a packed specular
   exponent of 1 (=infinity).*/   
static bool mirrorReflectiveFF(const UberBSDF::Ref& bsdf) { 
    return ((bsdf->specular().factors() == Component4::CONSTANT) ||
     (bsdf->specular().factors() == Component4::ONE)) &&
     ! bsdf->specular().isZero();
}

/** For fixed function, we detect glossy
    reflection as having a packed specular exponent less 
    than 1.*/   
static bool glossyReflectiveFF(const UberBSDF::Ref& bsdf) {
     return (bsdf->specular().constant().a < 1) && (bsdf->specular().constant().a > 0);
}


GenericPosedModel::Ref GenericPosedModel::create
(const std::string&       name,
 const CFrame&            frame, 
 const GPUGeom::Ref&      gpuGeom,
 const CPUGeom&           cpuGeom) {
    debugAssert(gpuGeom.notNull());
    debugAssert(gpuGeom->vertex.valid());
    debugAssert(gpuGeom->material.notNull());
    return new GenericPosedModel(name, frame, gpuGeom, cpuGeom);
}


static void setAdditive(RenderDevice* rd, bool& additive);

int GenericPosedModel::debugNumSendGeometryCalls = 0;

// Static to this file, not the class
static GenericPosedModel::GraphicsProfile graphicsProfile = GenericPosedModel::UNKNOWN;

void GenericPosedModel::setProfile(GraphicsProfile p) {
    graphicsProfile = p;
}


GenericPosedModel::GraphicsProfile GenericPosedModel::profile() {

    if (graphicsProfile == UNKNOWN) {
        if (GLCaps::supports_GL_ARB_shader_objects()) {
            graphicsProfile = PS20;

            
            if (System::findDataFile("SS_NonShadowedPass.vrt") == "") {
                graphicsProfile = UNKNOWN;
                logPrintf("\n\nWARNING: GenericPosedModel could not enter PS20 mode because"
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


const char* toString(GenericPosedModel::GraphicsProfile p) {
    switch (p) {
    case GenericPosedModel::UNKNOWN:
        return "Unknown";

    case GenericPosedModel::FIXED_FUNCTION:
        return "Fixed Function";

    case GenericPosedModel::PS14:
        return "PS 1.4";

    case GenericPosedModel::PS20:
        return "PS 2.0";

    default:
        return "Error!";
    }
}


void GenericPosedModel::renderNonShadowed(
    const Array<PosedModel::Ref>& posedArray, 
    RenderDevice* rd, 
    const LightingRef& lighting) {

    if (posedArray.size() == 0) {
        return;
    }

    rd->pushState();
        rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);

        // Lighting will be turned on and off by subroutines
        rd->disableLighting();

        const bool ps20 = GenericPosedModel::profile() == GenericPosedModel::PS20;

        for (int p = 0; p < posedArray.size(); ++p) {
            const GenericPosedModel::Ref& posed = posedArray[p].downcast<GenericPosedModel>();

            if (! rd->colorWrite()) {
                // No need for fancy shading, just send geometry
                posed->sendGeometry2(rd);
                continue;
            }


            const Material::Ref& material = posed->m_gpuGeom->material;
            const UberBSDF::Ref& bsdf = material->bsdf();
            (void)material;
            (void)bsdf;
            debugAssertM(bsdf->transmissive().isZero(), 
                "Transparent object passed through the batch version of "
                "GenericPosedModel::renderNonShadowed, which is intended exclusively for opaque objects.");

            // Alpha blend will be changed by subroutines so we restore it for each object
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
            rd->setDepthWrite(true);

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

            debugAssertGLOk();  //remove
            if (posed->m_gpuGeom->twoSided && ps20) {
                // gl_FrontFacing doesn't work on most cards inside
                // the shader, so we have to draw two-sided objects
                // twice
                rd->setCullFace(RenderDevice::CULL_FRONT);
                
                wroteDepth = posed->renderNonShadowedOpaqueTerms(rd, lighting, false) || wroteDepth;

            }

            if (! wroteDepth) {
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
        }
    rd->popState();
}


void GenericPosedModel::renderShadowMappedLightPass
(const Array<PosedModel::Ref>& posedArray, 
 RenderDevice* rd, 
 const GLight& light, 
 const ShadowMapRef& shadowMap) {
    
    if (posedArray.size() == 0) {
        return;
    }

    rd->pushState();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        rd->setCullFace(RenderDevice::CULL_BACK);

        rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);

        for (int i = 0; i < posedArray.size(); ++i) {
            const GenericPosedModel::Ref& posed    = posedArray[i].downcast<GenericPosedModel>();
            const Material::Ref&          material = posed->m_gpuGeom->material;
            const UberBSDF::Ref&              bsdf     = material->bsdf();

            if (bsdf->lambertian().isZero() && bsdf->specular().isZero()) {
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
    rd->popState();
 }


void GenericPosedModel::extractOpaque(
    Array<PosedModel::Ref>&   all, 
    Array<PosedModel::Ref>&   opaqueGeneric) {
    
    for (int i = 0; i < all.size(); ++i) {
        ReferenceCountedPointer<GenericPosedModel> m = 
            dynamic_cast<GenericPosedModel*>(all[i].pointer());

        if (m.notNull() && ! m->hasTransparency()) {
            // This is a most-derived subclass and is opaque

            opaqueGeneric.append(m);
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
        for (int y = 0; y < im.height; ++y) {
            for (int x = 0; x < im.width; ++x) {
                im.pixel3(x, y) = Color3(1, 1, 1);
            }
        }
        map = Texture::fromGImage("White", im, ImageFormat::RGB8());
    }
    return map;
}


void GenericPosedModel::render(RenderDevice* rd) const {

    // Infer the lighting from the fixed function

    // Avoid allocating memory for each render
    static LightingRef lighting = Lighting::create();

    rd->getFixedFunctionLighting(lighting);

    renderNonShadowed(rd, lighting);
}


bool GenericPosedModel::renderSuperShaderPass(
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


bool GenericPosedModel::renderNonShadowedOpaqueTerms(
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


bool GenericPosedModel::renderPS20NonShadowedOpaqueTerms(
    RenderDevice*                           rd,
    const LightingRef&                      lighting) const {

    const Material::Ref& material = m_gpuGeom->material;
    const UberBSDF::Ref&     bsdf     = material->bsdf();

    if (bsdf->isZero()) {
        // Nothing to draw
        return false;
    }

    int numLights = lighting->lightArray.size();

    if (numLights <= SuperShader::NonShadowedPass::LIGHTS_PER_PASS) {
        
        SuperShader::NonShadowedPass::instance()->setLighting(lighting);
        rd->setShader(SuperShader::NonShadowedPass::instance()->getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));

        sendGeometry2(rd);
        return true;

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
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
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


bool GenericPosedModel::renderFFNonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting) const {

    bool renderedOnce = false;

    const Material::Ref& material = m_gpuGeom->material;
    const UberBSDF::Ref&     bsdf = material->bsdf();


    // Emissive
    if (! material->emissive().isZero()) {
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

    // Add ambient + lights
    if (! bsdf->lambertian().isBlack() || ! bsdf->specular().isBlack()) {
        rd->enableLighting();
        rd->setTexture(0, bsdf->lambertian().texture());
        rd->setColor(bsdf->lambertian().constant());

        // Fixed function does not receive specular texture maps, only constants.
        rd->setSpecularCoefficient(bsdf->specular().constant().rgb());
        rd->setShininess(UberBSDF::unpackGlossyExponent(bsdf->specular().constant().a));

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

        if (renderedOnce) {
            // Make sure we add this pass to the previous already-rendered terms
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        }

        sendGeometry2(rd);
        renderedOnce = true;
        rd->disableLighting();
    }

    return renderedOnce;
}


bool GenericPosedModel::renderPS14NonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting) const {

    bool renderedOnce = false;
    const UberBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    // Emissive
    if (! m_gpuGeom->material->emissive().isBlack()) {
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


    bool hasDiffuse = ! bsdf->lambertian().isBlack();
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
            if (! bsdf->lambertian().isBlack() || hasGlossy) {
                rd->setTexture(nextUnit, bsdf->lambertian().texture());
                rd->setColor(bsdf->lambertian().constant());

                // Fixed function does not receive specular texture maps, only constants.
                rd->setSpecularCoefficient(bsdf->specular().constant().rgb());
                rd->setShininess(UberBSDF::unpackGlossyExponent(bsdf->specular().constant().a));

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
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, 
                Color4(bsdf->specular().constant().rgb() * lighting->environmentMapColor, 1));
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


void GenericPosedModel::renderNonShadowed
(RenderDevice*                   rd,
 const LightingRef&              lighting) const {

    // The transparent rendering path is not optimized to amortize state changes because 
    // it is only called by the single-object version of this function.  Only
    // opaque objects are batched together.
    const UberBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    if (! bsdf->transmissive().isBlack()) {
        rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);
        rd->pushState();
            // Transparent
            bool oldDepthWrite = rd->depthWrite();

            // Render backfaces first, and then front faces
            int passes = m_gpuGeom->twoSided ? 2 : 1;

            if (m_gpuGeom->twoSided) {
                // We're going to render the front and back faces separately.
                rd->setCullFace(RenderDevice::CULL_FRONT);
                rd->enableTwoSidedLighting();
            }

            for (int i = 0; i < passes; ++i) {
                rd->disableLighting();

                // Modulate background by transparent color
                rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_SRC_COLOR);
                rd->setTexture(0, bsdf->transmissive().texture());
                rd->setColor(bsdf->transmissive().constant());
                sendGeometry2(rd);

                bool alreadyAdditive = false;
                setAdditive(rd, alreadyAdditive);
                renderNonShadowedOpaqueTerms(rd, lighting, false);
            
                // restore depth write
                rd->setDepthWrite(oldDepthWrite);
                rd->setCullFace(RenderDevice::CULL_BACK);
            }
        rd->popState();
    } else {

        // This is the unoptimized, single-object version of renderShadowMappedLightPass.
        // It just calls the optimized version with a single-element array.

        static Array<PosedModel::Ref> posedArray;
        posedArray.resize(1);
        posedArray[0] = Ref(const_cast<GenericPosedModel*>(this));
        renderNonShadowed(posedArray, rd, lighting);
        posedArray.fastClear();
    }
}


void GenericPosedModel::renderShadowedLightPass(
    RenderDevice*       rd, 
    const GLight&       light) const {

    // TODO
    debugAssertM(false, "Unimplemented");
}


void GenericPosedModel::renderShadowMappedLightPass(
    RenderDevice*       rd, 
    const GLight&       light, 
    const Matrix4&      lightMVP, 
    const Texture::Ref&   shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.
    alwaysAssertM(false, "Deprecated: use the method that takes a ShadowMap");
}


void GenericPosedModel::renderShadowMappedLightPass(
    RenderDevice*         rd, 
    const GLight&         light, 
    const ShadowMapRef&   shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.

    static Array<PosedModel::Ref> posedArray;

    posedArray.resize(1);
    posedArray[0] = Ref(const_cast<GenericPosedModel*>(this));
    renderShadowMappedLightPass(posedArray, rd, light, shadowMap);
    posedArray.fastClear();
}


void GenericPosedModel::renderPS20ShadowMappedLightPass(
    RenderDevice*       rd,
    const GLight&       light, 
    const ShadowMapRef& shadowMap) const {

    SuperShader::ShadowedPass::instance()->setLight(light, shadowMap);
    rd->setShader(SuperShader::ShadowedPass::instance()->getConfiguredShader(*m_gpuGeom->material, rd->cullFace()));
    sendGeometry2(rd);
}


void GenericPosedModel::renderFFShadowMappedLightPass(
    RenderDevice*       rd,
    const GLight&       light, 
    const ShadowMapRef& shadowMap) const {

    rd->configureShadowMap(1, shadowMap);

    rd->setObjectToWorldMatrix(m_frame);

    const UberBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

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
        static bool separateSpecular = GLCaps::supports("GL_EXT_separate_specular_color");

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
        rd->setShininess(UberBSDF::unpackGlossyExponent(bsdf->specular().constant().a));

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


void GenericPosedModel::sendGeometry2(
    RenderDevice*           rd) const {


    CoordinateFrame o2w = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(m_frame);

    rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
    sendGeometry(rd);

    rd->setObjectToWorldMatrix(o2w);

}


void GenericPosedModel::sendGeometry(
    RenderDevice*           rd) const {
    debugAssertGLOk();
    ++debugNumSendGeometryCalls;

    // TODO: Radeon mobility cards crash rendering VAR in wireframe mode.
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
            if (m_gpuGeom->tangent.valid() && (profile() == PS20) && (m_gpuGeom->tangent.size() > 0)) {
                rd->setTexCoordArray(1, m_gpuGeom->tangent);
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
                if ((m_cpuGeom.tangent != NULL) &&
                    (m_cpuGeom.tangent->size() > 0)) {
                    rd->setTexCoord(1, (*m_cpuGeom.tangent)[v]);
                }

                rd->setNormal(m_cpuGeom.geometry->normalArray[v]);
                rd->sendVertex(m_cpuGeom.geometry->vertexArray[v]);
            }
            rd->endPrimitive();
        }
    }
}

std::string GenericPosedModel::name() const {
    return m_name;
}


bool GenericPosedModel::hasTransparency() const {
    return ! m_gpuGeom->material->bsdf()->transmissive().isBlack();
}


void GenericPosedModel::getCoordinateFrame(CoordinateFrame& c) const {
    c = m_frame;
}


const MeshAlg::Geometry& GenericPosedModel::objectSpaceGeometry() const {
    return *(m_cpuGeom.geometry);
}


const Array<Vector3>& GenericPosedModel::objectSpaceTangents() const {
    if (m_cpuGeom.tangent == NULL) {
        const static Array<Vector3> x;
        return x;
    } else {
        return *(m_cpuGeom.tangent);
    }
}


const Array<Vector3>& GenericPosedModel::objectSpaceFaceNormals(bool normalize) const {
    static Array<Vector3> n;
    debugAssert(false);
    return n;
    // TODO
}


const Array<MeshAlg::Face>& GenericPosedModel::faces() const {
    static Array<MeshAlg::Face> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Edge>& GenericPosedModel::edges() const {
    static Array<MeshAlg::Edge> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Vertex>& GenericPosedModel::vertices() const {
    static Array<MeshAlg::Vertex> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<Vector2>& GenericPosedModel::texCoords() const {
    if (m_cpuGeom.texCoord0 == NULL) {
        const static Array<Vector2> x;
        return x;
    } else {
        return *(m_cpuGeom.texCoord0);
    }
}


bool GenericPosedModel::hasTexCoords() const {
    return (m_cpuGeom.texCoord0 != 0) && (m_cpuGeom.texCoord0->size() > 0);
}


const Array<MeshAlg::Face>& GenericPosedModel::weldedFaces() const {
    static Array<MeshAlg::Face> f;
    debugAssert(false);
    return f;
    // TODO
}


const Array<MeshAlg::Edge>& GenericPosedModel::weldedEdges() const {
    static Array<MeshAlg::Edge> e;
    debugAssert(false);
    return e;
    // TODO
}


const Array<MeshAlg::Vertex>& GenericPosedModel::weldedVertices() const {
    static Array<MeshAlg::Vertex> v;
    return v;
    debugAssert(false);
    // TODO
}


const Array<int>& GenericPosedModel::triangleIndices() const {
    alwaysAssertM(m_gpuGeom->primitive == MeshAlg::TRIANGLES, "This model is not composed of triangles."); 
    return *(m_cpuGeom.index);
}


void GenericPosedModel::getObjectSpaceBoundingSphere(Sphere& s) const {
    s = m_gpuGeom->sphereBounds;
}


void GenericPosedModel::getObjectSpaceBoundingBox(AABox& b) const {
    b = m_gpuGeom->boxBounds;
}


int GenericPosedModel::numBoundaryEdges() const {
    // TODO
    debugAssert(false);
    return 0;
}


int GenericPosedModel::numWeldedBoundaryEdges() const {
    // TODO
    return 0;
}

}

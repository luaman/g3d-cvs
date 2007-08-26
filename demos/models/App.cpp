/**
  @file models/App.cpp

  @author Morgan McGuire, morgan@cs.williams.edu
 */

#include <G3D/G3DAll.h>
#include "App.h"


App::App(const GApp::Settings& settings) : GApp(settings), lighting(Lighting::create()) {

    try {
        showRenderingStats = false;
        window()->setCaption("G3D Model Demo");

        shadowMap = ShadowMap::create(1024);

        defaultCamera.setPosition(Vector3(-5.8f,   1.4f,   4.0f));
        defaultCamera.lookAt(Vector3( -4.0f,  -0.1f,   0.9f));

        loadScene();
        sky = Sky::fromFile(dataDir + "sky/");

        Texture::Settings settings;
        settings.wrapMode = WrapMode::CLAMP;
        logo = Texture::fromFile("G3D-logo-tiny-alpha.tga", TextureFormat::AUTO(), Texture::DIM_2D, settings);
    } catch (const std::string& s) {
        alwaysAssertM(false, s);
        exit(-1);
    }

    toneMap->setEnabled(false);
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    for (int e = 0; e < entityArray.size(); ++e) {
        entityArray[e]->onSimulation(rdt);
    }
}

void App::onUserInput(UserInput* ui) {
    if (ui->keyPressed(' ')) {
        toneMap->setEnabled(! toneMap->enabled());
    }
}


void App::onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    for (int e = 0; e < entityArray.size(); ++e) {
        entityArray[e]->onPose(posed3D);
    }
}

/** Converts a depth map to a 16-bit luminance color map */
void depthToColor(TextureRef depth, TextureRef& color, RenderDevice* rd) {
    if (color.isNull()) {
        Texture::Settings settings = Texture::Settings::video();
        settings.interpolateMode = Texture::NEAREST_NO_MIPMAP;
        // Must be RGB16; can't render to luminance textures
        color = Texture::createEmpty("Depth map", depth->width(), depth->height(), TextureFormat::RGB16F(), Texture::DIM_2D, settings);
    }

    // Convert depth to color
    static FramebufferRef fbo = Framebuffer::create("Offscreen");
    
    fbo->set(Framebuffer::COLOR_ATTACHMENT0, color);
    
    rd->push2D(fbo);
        rd->setTexture(0, depth);
        Draw::rect2D(depth->rect2DBounds(), rd);
    rd->pop2D();
}


static void renderModels
(
 RenderDevice*                  rd, 
 const Array<PosedModelRef>&    posed3D, 
 const LightingRef&             lighting, 
 const Array<ShadowMapRef>&     shadowMaps) {

}

static void renderModels
(
 RenderDevice*                  rd, 
 const Array<PosedModelRef>&    posed3D, 
 const LightingRef&             lighting, 
 const ShadowMapRef             shadowMap) {
    static Array<ShadowMapRef> shadowMaps;
    shadowMaps.fastClear();
    shadowMaps.append(shadowMap);
    renderModels(rd, posed3D, lighting, shadowMaps);
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    LightingRef        lighting = toneMap->prepareLighting(this->lighting);
    SkyParameters skyParameters = toneMap->prepareSkyParameters(this->skyParameters);

    if (shadowMap->enabled()) {
        // Generate shadow map        
        const float lightProjX = 12, lightProjY = 12, lightProjNear = 1, lightProjFar = 40;
        shadowMap->updateDepth(rd,lighting->shadowedLightArray[0],
            lightProjX, lightProjY, lightProjNear, lightProjFar, posed3D);
    } else {
        // We're not going to be able to draw shadows, so move the shadowed lights into
        // the unshadowed category.
        lighting->lightArray.append(lighting->shadowedLightArray);
        lighting->shadowedLightArray.clear();
    }

    // Separate and sort the models
    Array<PosedModel::Ref> opaqueAModel, otherOpaque, transparent;
    ArticulatedModel::extractOpaquePosedAModels(posed3D, opaqueAModel);
    PosedModel::sort(opaqueAModel, defaultCamera.coordinateFrame().lookVector(), opaqueAModel);
    PosedModel::sort(posed3D, defaultCamera.coordinateFrame().lookVector(), otherOpaque, transparent);

    /////////////////////////////////////////////////////////////////////

    rd->setProjectionAndCameraMatrix(defaultCamera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    toneMap->beginFrame(rd);

    // Cyan background
    rd->setColorClearValue(Color3(.1f, .5f, 1));

    rd->clear(sky.notNull(), true, true);
    if (sky.notNull()) {
        sky->render(rd, skyParameters);
    }

    // Opaque unshadowed
    // TODO: Something in here destroys the shadow map if we don't render the 
    // otherOpaque array first.
    for (int m = 0; m < otherOpaque.size(); ++m) {
        otherOpaque[m]->renderNonShadowed(rd, lighting);
    }
    ArticulatedModel::renderNonShadowed(opaqueAModel, rd, lighting);

    // Opaque shadowed

    // TODO: Why doesn't the MD2 model get self-shadowing?
    if (lighting->shadowedLightArray.size() > 0) {
        ArticulatedModel::renderShadowMappedLightPass(opaqueAModel, rd, lighting->shadowedLightArray[0], shadowMap);
        for (int m = 0; m < otherOpaque.size(); ++m) {
            otherOpaque[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], shadowMap);
        }
    }

    // Transparent, must be rendered from back to front
    for (int m = 0; m < transparent.size(); ++m) {
        transparent[m]->renderNonShadowed(rd, lighting);
        if (lighting->shadowedLightArray.size() > 0) {
            transparent[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], shadowMap);
        }
    }
    
    if (sky.notNull()) {
        sky->renderLensFlare(rd, skyParameters);
    }

    toneMap->endFrame(rd);

    PosedModel2D::sortAndRender(rd, posed2D);

    screenPrintf("Tone Map %s\n", toneMap->enabled() ? "On" : "Off");
    screenPrintf("%s Profile %s\n", toString(ArticulatedModel::profile()),
        #ifdef _DEBUG
                "(DEBUG mode)"
        #else
                ""
        #endif
        );

    rd->push2D();
    rd->setTexture(0, logo);
    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    Draw::rect2D(
        Rect2D::xywh(rd->width() - 96,rd->height() - 96, 64, 64), 
        rd, Color4(1,1,1,0.7f));
    //    rd->setTexture(0, shadowMap->colorDepthTexture());
    //Draw::rect2D(shadowMap->colorDepthTexture()->rect2DBounds(), rd);
    rd->pop2D();
}


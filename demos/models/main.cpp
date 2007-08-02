/**
  @file models/main.cpp

  @author Morgan McGuire, morgan@cs.williams.edu
 */

#include <G3D/G3DAll.h>
#include "App.h"

G3D_START_AT_MAIN();

/**
 Width and height of shadow map.
 */
int shadowMapSize = 512;

App::App(const GApp::Settings& settings) : GApp(settings), lighting(Lighting::create()) {

    try {
        showRenderingStats = false;
        window()->setCaption("G3D Model Demo");

        shadowMap.setSize(64);
        defaultCamera.setPosition(Vector3(-5.8,   1.4,   4.0));
        defaultCamera.lookAt(Vector3( -4.0,  -0.1,   0.9));

        loadScene();
        sky = Sky::fromFile(dataDir + "sky/");

        Texture::Settings settings;
        settings.wrapMode = WrapMode::CLAMP;
        logo = Texture::fromFile("G3D-logo-tiny-alpha.tga", TextureFormat::AUTO(), Texture::DIM_2D, settings);
    } catch (const std::string& s) {
        alwaysAssertM(false, s);
        exit(-1);
    }
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


void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    LightingRef        lighting = toneMap->prepareLighting(this->lighting);
    SkyParameters skyParameters = toneMap->prepareSkyParameters(this->skyParameters);

    if (shadowMap.enabled()) {     
        // Generate shadow map        
        const float lightProjX = 12, lightProjY = 12, lightProjNear = 1, lightProjFar = 40;
        shadowMap.updateDepth(rd,lighting->shadowedLightArray[0],
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
    
    screenPrintf("%d opt opaque, %d opaque, %d transparent\n", opaqueAModel.size(), 
                otherOpaque.size(), transparent.size());

    toneMap->beginFrame(rd);

    // Cyan background
    rd->setColorClearValue(Color3(.1f, .5f, 1));

    rd->clear(sky.notNull(), true, true);
    if (sky.notNull()) {
        sky->render(rd, skyParameters);
    }

    // Opaque unshadowed
    ArticulatedModel::renderNonShadowed(opaqueAModel, rd, lighting);
    for (int m = 0; m < otherOpaque.size(); ++m) {
        otherOpaque[m]->renderNonShadowed(rd, lighting);
    }

    // Opaque shadowed
    
    if (lighting->shadowedLightArray.size() > 0) {
        ArticulatedModel::renderShadowMappedLightPass(opaqueAModel, rd, lighting->shadowedLightArray[0], shadowMap.lightMVP(), shadowMap.depthTexture());
        for (int m = 0; m < otherOpaque.size(); ++m) {
            otherOpaque[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], shadowMap.lightMVP(), shadowMap.depthTexture());
        }
    }

    // Transparent
    for (int m = 0; m < transparent.size(); ++m) {
        transparent[m]->renderNonShadowed(rd, lighting);
        if (lighting->shadowedLightArray.size() > 0) {
            transparent[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], shadowMap.lightMVP(), shadowMap.depthTexture());
        }
    }
    
    if (sky.notNull()) {
        sky->renderLensFlare(rd, skyParameters);
    }

    toneMap->endFrame(rd);
    PosedModel2D::sortAndRender(rd, posed2D);

    debugPrintf("Tone Map %s\n", toneMap->enabled() ? "On" : "Off");
    debugPrintf("%s Profile %s\n", toString(ArticulatedModel::profile()),
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
    rd->pop2D();
}


int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.fsaaSamples = 4;

#   ifdef G3D_WIN32
        if (! fileExists("G3D-logo-tiny-alpha.tga", false)) {
            // Running under visual studio from the wrong directory
            chdir("../demos/models/data-files");
        }
#   endif

    alwaysAssertM(fileExists("G3D-logo-tiny-alpha.tga", false), 
        "Cannot find runtime data files.");
    return App(settings).run();
}

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

        defaultCamera.setPosition(Vector3(-2,   1.4f,   4.0f));
        defaultCamera.lookAt(Vector3( 0,  -0.1f,   0.9f));

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


void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    LightingRef        lighting = toneMap->prepareLighting(this->lighting);
    SkyParameters skyParameters = toneMap->prepareSkyParameters(this->skyParameters);

    rd->setProjectionAndCameraMatrix(defaultCamera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    toneMap->beginFrame(rd);

    // Cyan background
    rd->setColorClearValue(Color3(.1f, .5f, 1));

    rd->clear(sky.notNull(), true, true);

    if (sky.notNull()) {
        sky->render(rd, skyParameters);
    }

    PosedModel::sortAndRender(rd, defaultCamera, posed3D, lighting, shadowMap);
    //Draw::axes(CoordinateFrame(), rd);

    if (sky.notNull()) {
        sky->renderLensFlare(rd, skyParameters);
    }

    Draw::lighting(lighting, rd);

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


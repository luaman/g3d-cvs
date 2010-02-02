/**
  @file models/App.cpp

  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include "App.h"

App::App(const GApp::Settings& settings) : GApp(settings), lighting(Lighting::create()) {
    catchCommonExceptions = false;
    try {
        showRenderingStats = false;
        window()->setCaption("G3D Model Demo");

        shadowMap = ShadowMap::create();

        defaultCamera.setPosition(Vector3(-2,   1.4f,   4.0f));
        defaultCamera.lookAt(Vector3( 0,  -0.1f,   0.9f));

        loadScene();

        Texture::Settings settings;
        settings.wrapMode = WrapMode::CLAMP;
        logo = Texture::fromFile("G3D-logo-tiny-alpha.tga", ImageFormat::AUTO(), 
                                 Texture::DIM_2D, settings);
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
}


void App::onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    for (int e = 0; e < entityArray.size(); ++e) {
        entityArray[e]->onPose(posed3D);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D) {
    screenPrintf("Lights: %d\n", lighting->lightArray.size());
    screenPrintf("S Lights: %d\n", lighting->shadowedLightArray.size());
    SuperSurface::debugNumSendGeometryCalls = 0;

    rd->setProjectionAndCameraMatrix(defaultCamera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    // Cyan background
    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    rd->clear(sky.notNull(), true, true);

    if (sky.notNull()) {
        sky->render(rd, skyParameters);
    }
    
    Surface::sortAndRender(rd, defaultCamera, posed3D, lighting, shadowMap);

    /*
    // See bounding volumes:
    for (int i = 0; i < posed3D.size(); ++i) {
        Draw::sphere(posed3D[i]->worldSpaceBoundingSphere(), rd, Color4::clear(), Color3::black());
    }
    */
    Draw::axes(CFrame(Vector3(0,-0.9,0)), rd);

    rd->setAlphaTest(RenderDevice::ALPHA_ALWAYS_PASS, 0.0f);
    Draw::lighting(lighting, rd, false);

    screenPrintf("%s Profile %s\n", toString(SuperSurface::profile()),
        #ifdef _DEBUG
                "(DEBUG mode)"
        #else
                ""
        #endif
        );
}

void App::onGraphics2D(RenderDevice *rd, Array< Surface2D::Ref > &surface2D) {
    rd->push2D();
        rd->setTexture(0, logo);
        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Draw::rect2D(
            Rect2D::xywh(rd->width() - 96, rd->height() - 96, 64, 64), 
            rd, Color4(1,1,1,0.7f));
    rd->pop2D();

    screenPrintf("SuperSurface::debugNumSendGeometryCalls = %d\n", 
                 SuperSurface::debugNumSendGeometryCalls);

    GApp::onGraphics2D(rd, surface2D);
}


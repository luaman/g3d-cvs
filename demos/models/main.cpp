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

App::App(const GApp2::Settings& settings) : GApp2(settings), lighting(Lighting::create()) {

    try {
    showRenderingStats = true;

    defaultController->setActive(false);

    window()->setCaption("G3D Model Demo");

    if (beginsWith(GLCaps::vendor(), "ATI")) {
        // On ATI cards, large shadow maps cause terrible performance during the
        // copy from the back buffer on recent drivers.
        shadowMapSize = 64;
    }

    if (GLCaps::supports_GL_ARB_shadow()) {        
        shadowMap = Texture::createEmpty(
			"Shadow Map",
			shadowMapSize, shadowMapSize,
			TextureFormat::depth(),
			Texture::DIM_2D, Texture::Settings::shadow());
    }

    // Called before Demo::run() beings
    defaultCamera.setPosition(Vector3(0, 0, 10));
    defaultCamera.lookAt(Vector3(0, 0, 0));

    loadScene();
    sky = Sky::fromFile(dataDir + "sky/");

    {
        Texture::Settings settings;
        settings.wrapMode = WrapMode::CLAMP;
        logo = Texture::fromFile("G3D-logo-tiny-alpha.tga", TextureFormat::AUTO, Texture::DIM_2D, settings);
    }
    } catch (const std::string& s) {
        alwaysAssertM(false, s);
        exit(-1);
    }
}


void App::onUserInput(UserInput* ui) {
    if (ui->keyPressed(' ')) {
        toneMap->setEnabled(! toneMap->enabled());
    }
}

bool debugShadows = false;

void App::generateShadowMap(const GLight& light, const Array<PosedModel::Ref>& shadowCaster) {
    debugAssert(GLCaps::supports_GL_ARB_shadow()); 

    Rect2D rect = Rect2D::xywh(0, 0, shadowMapSize, shadowMapSize);
    
    renderDevice->pushState();

        const double lightProjX = 12, lightProjY = 12, lightProjNear = 1, lightProjFar = 40;

        // Construct a projection and view matrix for the camera so we can 
        // render the scene from the light's point of view

        // Since we're working with a directional light, 
        // we want to make the center of projection for the shadow map
        // be in the direction of the light but at a finite distance 
        // to preserve z precision.
        Matrix4 lightProjectionMatrix(Matrix4::orthogonalProjection(-lightProjX, lightProjX, -lightProjY, lightProjY, lightProjNear, lightProjFar));

        CoordinateFrame lightCFrame;
        lightCFrame.translation = light.position.xyz() * 20;

        // The light will never be along the z-axis
        lightCFrame.lookAt(Vector3::zero(), Vector3::unitZ());

        debugAssert(shadowMapSize < renderDevice->height());
        debugAssert(shadowMapSize < renderDevice->width());

        renderDevice->setColorClearValue(Color3::white());
        renderDevice->clear(debugShadows, true, false);

        renderDevice->setDepthWrite(true);
        renderDevice->setViewport(rect);

	    // Draw from the light's point of view
        renderDevice->setCameraToWorldMatrix(lightCFrame);
        renderDevice->setProjectionMatrix(lightProjectionMatrix);

        // Flip the Y-axis to account for the upside down Y-axis on read back textures
        lightMVP = lightProjectionMatrix * lightCFrame.inverse();

        if (! debugShadows) {
            renderDevice->setColorWrite(false);
        }

        // Avoid z-fighting
        renderDevice->setPolygonOffset(2);

        renderDevice->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);
        for (int s = 0; s < shadowCaster.size(); ++s) {
            shadowCaster[s]->render(renderDevice);
        }
   
    renderDevice->popState();
    
    debugAssert(shadowMap.notNull());
    shadowMap->copyFromScreen(rect);
}


void App::onGraphics(RenderDevice* rd) {
    LightingRef        lighting = toneMap->prepareLighting(this->lighting);
    SkyParameters skyParameters = toneMap->prepareSkyParameters(this->skyParameters);

    if (! GLCaps::supports_GL_ARB_shadow() && (lighting->shadowedLightArray.size() > 0)) {
        // We're not going to be able to draw shadows, so move the shadowed lights into
        // the unshadowed category.
        lighting->lightArray.append(lighting->shadowedLightArray);
        lighting->shadowedLightArray.clear();
    }

    // Pose all
    Array<PosedModel2DRef> posed2D;
    Array<PosedModel::Ref> posedModels, opaqueAModel, otherOpaque, transparent;

    for (int e = 0; e < entityArray.size(); ++e) {
        static RealTime t0 = System::time();
        RealTime t = (System::time() - t0) * 10;
        entityArray[e]->pose.cframe.set("Top", 
            CoordinateFrame(Matrix3::fromAxisAngle(Vector3::unitY(), t),
                            Vector3::zero()));

        entityArray[e]->pose.cframe.set("Clouds", 
            CoordinateFrame(Matrix3::fromAxisAngle(Vector3::unitY(), t/1000),
                            Vector3::zero()));

        entityArray[e]->model->pose(posedModels, entityArray[e]->cframe, entityArray[e]->pose);
    }

    // Get any GModule models
    getPosedModel(posedModels, posed2D);

    if (GLCaps::supports_GL_ARB_shadow() && (lighting->shadowedLightArray.size() > 0)) {     
        // Generate shadow map
        generateShadowMap(lighting->shadowedLightArray[0], posedModels);
    }

    // Separate and sort the models
    ArticulatedModel::extractOpaquePosedAModels(posedModels, opaqueAModel);
    PosedModel::sort(opaqueAModel, defaultCamera.getCoordinateFrame().lookVector(), opaqueAModel);
    PosedModel::sort(posedModels, defaultCamera.getCoordinateFrame().lookVector(), otherOpaque, transparent);

    /////////////////////////////////////////////////////////////////////

    if (debugShadows) {
        return;
    }

    rd->setProjectionAndCameraMatrix(defaultCamera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    debugPrintf("%d opt opaque, %d opaque, %d transparent\n", opaqueAModel.size(), otherOpaque.size(), transparent.size());

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
        ArticulatedModel::renderShadowMappedLightPass(opaqueAModel, rd, lighting->shadowedLightArray[0], lightMVP, shadowMap);
        for (int m = 0; m < otherOpaque.size(); ++m) {
            otherOpaque[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], lightMVP, shadowMap);
        }
    }

    // Transparent
    for (int m = 0; m < transparent.size(); ++m) {
        transparent[m]->renderNonShadowed(rd, lighting);
        if (lighting->shadowedLightArray.size() > 0) {
            transparent[m]->renderShadowMappedLightPass(rd, lighting->shadowedLightArray[0], lightMVP, shadowMap);
        }
    }

    if (sky.notNull()) {
        sky->renderLensFlare(rd, skyParameters);
    }

    toneMap->endFrame(rd);

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

    debugPrintf("\n");
    debugPrintf("TAB to control camera\n");
    debugPrintf("SPACE to toggle ToneMap\n");
    debugPrintf("ESC to quit\n");

}


int main(int argc, char** argv) {
	GApp2::Settings settings;
    /*
    settings.window.depthBits = 16;
    settings.window.stencilBits = 8;
    settings.window.alphaBits = 0;
    settings.window.rgbBits = 8;
    settings.window.fsaaSamples = 1;
    settings.window.width = 800;
    settings.window.height = 600;
    settings.window.fullScreen = true;
    */
    
    return App(settings).run();
}

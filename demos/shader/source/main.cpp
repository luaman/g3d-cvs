/**
  @file shader/main.cpp

  Example of G3D shaders and GUIs.
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class App : public GApp2 {
private:
    /** Lighting environment */
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;

    /** For dragging the model */
    ThirdPersonManipulatorRef manipulator;
    IFSModelRef         model;

    /** Material properties and shader */
    ShaderRef           phongShader;
    float               reflect;
    float               shine;
    float               diffuse;
    float               specular;
    int                 diffuseColorIndex;
    int                 specularColorIndex;
    Array<GuiCaption>   colorList;

    void makeGui();
    void makeColorList(GFontRef iconFont);
    void configureShaderArgs(const LightingRef localLighting);

public:

    App(const GApp2::Settings& settings = GApp2::Settings());

    virtual void onInit();
    virtual void onGraphics(RenderDevice* rd);
};

App::App(const GApp2::Settings& settings) : GApp2(settings), diffuse(0.6f), specular(0.5f), shine(20.0f), reflect(0.1f) {}

void App::onInit() {
    window()->setCaption("G3D Shader Demo");

    // Called before the application loop beings.  Load data here
    // and not in the constructor so that common exceptions will be
    // automatically caught.
    sky = Sky::fromFile(dataDir + "sky/");

    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();

#   ifdef G3D_WIN32
        if (! fileExists("phong.pix", false) && fileExists("G3D.sln", false)) {
            // Running in the solution directory
            chdir("../demos/shader/data-files");
        }
        toneMap->setEnabled(true);
#   else
        toneMap->setEnabled(false);
#   endif

    phongShader = Shader::fromFiles("phong.vrt", "phong.pix");
    model = IFSModel::fromFile(System::findDataFile("teapot.ifs"));

    makeGui();

    // Color 1 is red
    diffuseColorIndex = 1;
    // Last color is white
    specularColorIndex = colorList.size() - 1;
    
    defaultCamera.setPosition(Vector3(1.0f, 1.0f, 2.5f));
    defaultCamera.lookAt(Vector3::zero());

    // Add axes for dragging the camera
    manipulator = ThirdPersonManipulator::create();
    addWidget(manipulator);

    // Turn off the default first-person camera controller and developer UI
    defaultController->setActive(false);
    developerWindow->setVisible(false);
}

void App::makeColorList(GFontRef iconFont) {
    // Characters in icon font that make a solid block of color
    const char* block = "gggggg";

    float size = 18;
    int N = 10;
    colorList.append(GuiCaption(block, iconFont, size, Color3::black(), Color4::clear()));
    for (int i = 0; i < N; ++i) {
        colorList.append(GuiCaption(block, iconFont, size, Color3::rainbowColorMap((float)i / N), Color4::clear()));
    }
    colorList.append(GuiCaption(block, iconFont, size, Color3::white(), Color4::clear()));
}

void App::makeGui() {
    GuiSkinRef skin = GuiSkin::fromFile(System::findDataFile("twilight.skn"), debugFont);
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GuiWindow::Ref gui = GuiWindow::create("Material Parameters", skin, Rect2D::xywh(10, 10, 0, 0));

    makeColorList(iconFont);

    GuiPane* pane = gui->pane();
    pane->addDropDownList("Diffuse", &diffuseColorIndex, &colorList);
    pane->addSlider("Intensity", &diffuse, 0.0f, 1.0f);

    pane->addDropDownList("Specular", &specularColorIndex, &colorList);
    pane->addSlider("Intensity", &specular, 0.0f, 1.0f);

    pane->addSlider("Shininess", &shine, 1.0f, 100.0f);
    pane->addSlider("Reflectivity", &reflect, 0.0f, 1.0f);

    addWidget(gui);

    showRenderingStats = false;
}

void App::onGraphics(RenderDevice* rd) {
    toneMap->beginFrame(rd);

    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    //////////////////////////////////////////////////////////////////////
    // Shader example

    rd->pushState();
        // Pose our model based on the manipulator axes
        PosedModel::Ref posedModel = model->pose(manipulator->frame());
        
        // Enable the sahder
        configureShaderArgs(localLighting);
        rd->setShader(phongShader);

        // Send model geometry to the graphics card
        rd->setObjectToWorldMatrix(posedModel->coordinateFrame());
        posedModel->sendGeometry(rd);
    rd->popState();

    //////////////////////////////////////////////////////////////////////
    // Normal rendering loop boilerplate

    // Process the installed widgets.

    Array<PosedModel::Ref> posedArray, opaque, transparent; 
    Array<PosedModel2DRef> posed2DArray;    
    getPosedModel(posedArray, posed2DArray);

    // Use fixed-function lighting for the 3D widgets for convenience.
    rd->pushState();
        rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

        // 3D
        if (posedArray.size() > 0) {
            Vector3 lookVector = renderDevice->getCameraToWorldMatrix().lookVector();
            PosedModel::sort(posedArray, lookVector, opaque, transparent);

            for (int i = 0; i < opaque.size(); ++i) {
                opaque[i]->render(renderDevice);
            }

            for (int i = 0; i < transparent.size(); ++i) {
                transparent[i]->render(renderDevice);
            }
        }
    rd->popState();

    // Don't apply the tone map to the 2D widgets
    toneMap->endFrame(rd);

    // Render the 2D widgets
    if (posed2DArray.size() > 0) {
        renderDevice->push2D();
            PosedModel2D::sort(posed2DArray);
            for (int i = 0; i < posed2DArray.size(); ++i) {
                posed2DArray[i]->render(renderDevice);
            }
        renderDevice->pop2D();
    }

    sky->renderLensFlare(rd, localSky);
}

void App::configureShaderArgs(const LightingRef localLighting) {
    const GLight& light = localLighting->lightArray[0]; 

    phongShader->args.set("wsLight", light.position.xyz().direction());
    phongShader->args.set("lightColor", light.color);
    phongShader->args.set("wsEyePosition", defaultCamera.getCoordinateFrame().translation);
    phongShader->args.set("ambientLightColor", localLighting->ambientAverage());

    phongShader->args.set("diffuseColor", colorList[diffuseColorIndex].color(Color3::white()).rgb());
    phongShader->args.set("diffuse", diffuse);

    phongShader->args.set("specularColor", colorList[specularColorIndex].color(Color3::white()).rgb());
    phongShader->args.set("specular", specular);
    phongShader->args.set("shine", shine);
    phongShader->args.set("reflect", reflect);

    phongShader->args.set("environmentMap", localLighting->environmentMap);
    phongShader->args.set("environmentMapColor", localLighting->environmentMapColor);
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    return App().run();
}

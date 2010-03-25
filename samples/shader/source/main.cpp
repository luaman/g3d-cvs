/**
  @file shader/main.cpp

  Example of G3D shaders and GUIs.
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class App : public GApp {
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
    Array<GuiText>   colorList;

    void makeGui();
    void makeColorList(GFontRef iconFont);
    void configureShaderArgs(const LightingRef localLighting);

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);
};

App::App(const GApp::Settings& settings) : GApp(settings), reflect(0.1f), shine(20.0f), diffuse(0.6f), specular(0.5f) {}

void App::onInit() {
    window()->setCaption("G3D Shader Demo");

    // Called before the application loop beings.  Load data here
    // and not in the constructor so that common exceptions will be
    // automatically caught.
    sky = Sky::fromFile(pathConcat(dataDir, "sky/"));

    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();
    setDesiredFrameRate(60);

#   ifdef G3D_WIN32
	if (! FileSystem::exists("phong.pix", false) && FileSystem::exists("G3D.sln", false)) {
            // Running in the solution directory
            chdir("../samples/shader/data-files");
        }
#   endif

    phongShader = Shader::fromFiles("phong.vrt", "phong.pix");
    model = IFSModel::fromFile(System::findDataFile("teapot.ifs"));

    makeColorList(GFont::fromFile(System::findDataFile("icon.fnt")));
    makeGui();

    // Color 1 is red
    diffuseColorIndex = 1;
    // Last color is white
    specularColorIndex = colorList.size() - 1;
    
    defaultCamera.setPosition(Vector3(1.0f, 1.0f, 2.5f));
    defaultCamera.lookAt(Vector3::zero());

    // Add axes for dragging and turning the model
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
    colorList.append(GuiText(block, iconFont, size, Color3::black(), Color4::clear()));
    for (int i = 0; i < N; ++i) {
        colorList.append(GuiText(block, iconFont, size, Color3::rainbowColorMap((float)i / N), Color4::clear()));
    }
    colorList.append(GuiText(block, iconFont, size, Color3::white(), Color4::clear()));
}


void App::makeGui() {
    GuiThemeRef skin = GuiTheme::fromFile(System::findDataFile("osx.skn"), debugFont);
    GuiWindow::Ref gui = GuiWindow::create("Material Parameters", skin);
    
    GuiPane* pane = gui->pane();
    pane->addDropDownList("Diffuse", colorList, &diffuseColorIndex);
    pane->addSlider("Intensity", &diffuse, 0.0f, 1.0f);
    
    pane->addDropDownList("Specular", colorList, &specularColorIndex);
    pane->addSlider("Intensity", &specular, 0.0f, 1.0f);
    
    pane->addSlider("Shininess", &shine, 1.0f, 100.0f);
    pane->addSlider("Reflectivity", &reflect, 0.0f, 1.0f);
    
    addWidget(gui);
}


void App::onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    LightingRef   localLighting = lighting;
    SkyParameters localSky      = skyParameters;
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    //////////////////////////////////////////////////////////////////////
    // Shader example

    rd->pushState();
        // Pose our model based on the manipulator axes
        Surface::Ref posedModel = model->pose(manipulator->frame());
        
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

    Array<Surface::Ref> translucent; 

    // Use fixed-function lighting for the 3D widgets for convenience.
    rd->pushState();
        rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

        // 3D
        if (posed3D.size() > 0) {
            Vector3 lookVector = renderDevice->cameraToWorldMatrix().lookVector();
            Surface::extractTranslucent(posed3D, translucent, false);
            Surface::sortFrontToBack(posed3D, lookVector);
            Surface::sortBackToFront(translucent, lookVector);

            for (int i = 0; i < posed3D.size(); ++i) {
                posed3D[i]->render(renderDevice);
            }

            for (int i = 0; i < translucent.size(); ++i) {
                translucent[i]->render(renderDevice);
            }
        }
    rd->popState();

    Surface2D::sortAndRender(rd, posed2D);
}

void App::configureShaderArgs(const LightingRef lighting) {
	const GLight& light = lighting->lightArray[0]; 

	phongShader->args.set("wsLight", light.position.xyz().direction());
	phongShader->args.set("lightColor", light.color);
	phongShader->args.set("wsEyePosition", defaultCamera.coordinateFrame().translation);
	phongShader->args.set("ambientLightColor", lighting->ambientAverage());

	Color3 color = colorList[diffuseColorIndex].element(0).color(Color3::white()).rgb();
	phongShader->args.set("diffuseColor", color);
	phongShader->args.set("diffuse", diffuse);

	color = colorList[specularColorIndex].element(0).color(Color3::white()).rgb();
	phongShader->args.set("specularColor", color);
	phongShader->args.set("specular", specular);
	phongShader->args.set("shine", shine);
	phongShader->args.set("reflect", reflect);

	phongShader->args.set("environmentMap", lighting->environmentMap);
	phongShader->args.set("environmentMapColor", lighting->environmentMapColor);
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;

    settings.window.width       = 960; 
    settings.window.height      = 600;

    return App(settings).run();
}

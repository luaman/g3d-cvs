/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is designed to make writing an
  application easy.  Although the GApp2 infrastructure is helpful for most projects, you are not 
  restricted to using it--choose the level of support that is best for your project.  You can 
  also customize GApp2 through its members and change its behavior by overriding methods.

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif

class App : public GApp2 {
private:
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;

    ShaderRef           phongShader;
    IFSModelRef         model;

    float               reflect;
    float               shine;
    float               diffuse;
    float               specular;

    void makeGui();

public:

    App(const GApp2::Settings& settings = GApp2::Settings());

    virtual void onInit();
    virtual void onLogic();
    virtual void onNetwork();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onGraphics(RenderDevice* rd);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
};

App::App(const GApp2::Settings& settings) : GApp2(settings) {}

void App::onInit() {
    // Called before the application loop beings.  Load data here
    // and not in the constructor so that common exceptions will be
    // automatically caught.
    sky = Sky::fromFile(dataDir + "sky/");

    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();

    toneMap->setEnabled(false);
#   ifdef G3D_WIN32
    if (! fileExists("phong.pix", false) && fileExists("G3D.sln", false)) {
        // Running in the solution directory
        chdir("../demos/shader/data-files");
    }
#   endif

    phongShader = Shader::fromFiles("", "phong.pix");
    model = IFSModel::fromFile(System::findDataFile("teapot.ifs"));

    makeGui();
}

void App::makeGui() {
    GuiSkinRef skin = GuiSkin::fromFile(System::findDataFile("twilight.skn"), debugFont);
    GuiWindow::Ref gui = GuiWindow::create("Parameters", skin);

    GuiPane* pane = gui->pane();
//    pane->add

    addWidget(gui);
}

void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}

void App::onLogic() {
    // Add non-simulation game logic and AI code here
}

void App::onNetwork() {
    // Poll net messages here
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    // Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
}

void App::onUserInput(UserInput* ui) {
    // Add key handling here	
}


void App::onGraphics(RenderDevice* rd) {
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    const GLight& light = localLighting->lightArray[0]; 
    Color3 ambientLightColor = localLighting->ambientAverage();

    // Pose all models
    Array<PosedModelRef> posed;
    posed.append(model->pose());

    // Configure shader
    phongShader->args.set("wsLight", light.position.xyz().direction());
    phongShader->args.set("lightColor", light.color);
    phongShader->args.set("wsEye", defaultCamera.getCoordinateFrame().translation);
    phongShader->args.set("ambientLightColor", ambientLightColor);

    rd->setShader(phongShader);
        // Send model geometry to the graphics card
        for (int i = 0; i < posed.size(); ++i) {
            posed[i]->sendGeometry(rd);
        }
    // Disable shader
    rd->setShader(NULL);


    // Always render the installed widgets or the console and other
    // features will not appear.  We use fixed-function rendering here for
    // convenience.
    rd->enableLighting();
        rd->setLight(0, light);
        rd->setAmbientLightColor(ambientLightColor);
        renderWidgets(rd);
    rd->disableLighting();

    sky->renderLensFlare(rd, localSky);
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    return App().run();
}

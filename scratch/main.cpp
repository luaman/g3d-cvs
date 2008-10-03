/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is designed to make writing an
  application easy.  Although the GApp infrastructure is helpful for most projects, you are not 
  restricted to using it--choose the level of support that is best for your project.  You can 
  also customize GApp through its members and change its behavior by overriding methods.

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif

class App : public GApp {
public:
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;
    BSPMapRef           map;
    IFSModelRef         ifsModel;
    ArticulatedModelRef model;
    Array<Vector3>      points;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onLogic();
    virtual void onNetwork();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};

App::App(const GApp::Settings& settings) : GApp(settings) {
}

enum None {NONE};
enum Scale {LOG, LINEAR};
typedef int Both;

Both x = NONE;

void App::onInit() {

    setDesiredFrameRate(20);

    for (int i= 0;i < 1000; ++i) {
        points.append(Vector3::cosRandom(Vector3::unitY()));
    }

    ifsModel = IFSModel::fromFile("c:/temp/db/2/m213/m213.off");
//	map = BSPMap::fromFile("X:/morgan/data/quake3/tremulous/map-arachnid2-1.1.0.pk3/", "arachnid2.bsp");

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

    static std::string text = "hi";
    debugPane->addTextBox("Text", &text);    // Indent and display a caption
    debugPane->addTextBox(" ", &text);       // Indent, but display no caption
    debugPane->addTextBox("", &text);        // Align the text box to the left
    debugWindow->setVisible(true);

    static float f = 0.5f;

    float low = 0.0f;
    float high = 100.0f;
/*
    Pointer<float> ptr(&f);
    
    Pointer<float> ptr2 = LogScaleAdapter<float>::wrap(ptr, low, high);
//    debugPane->addSlider("Log", ptr2, low, high);
//    debugPane->addSlider("Linear", ptr, low, high);
    debugPane->addNumberBox("Log", ptr2, "s", true, low, high);
    debugPane->addNumberBox("Linear", ptr, "s", true, low, high);
    */
    debugPane->addNumberBox("Log", &f, "s", GuiTheme::LOG_SLIDER, low, high);
    debugPane->addNumberBox("Linear", &f, "s", GuiTheme::LINEAR_SLIDER, low, high);
//    debugPane->addNumberBox("Exposure", &f, "s", true, 0.1f, 1000.0f, 0.5f);


    static Array<std::string> list;
    list.append("First");
    for (int i = 0; i < 10; ++i) {
        list.append(format("Item %d", i + 2));
    }
    list.append("Last");
    static int index = 0;
    debugPane->addDropDownList("List", &index, &list);

    model = ArticulatedModel::createCornellBox();

    // NumberBox: textbox for numbers: label, ptr, suffix, slider?, min, max, roundToNearest
    // defaults:                        /   , ptr, "", false, -inf, inf, 0

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
    screenPrintf("LS %d  RS %d   LC %d RC %d\n", 
           ui->keyDown(GKey::LSHIFT),
           ui->keyDown(GKey::RSHIFT),
           ui->keyDown(GKey::LCTRL),
           ui->keyDown(GKey::RCTRL));
}

void App::onConsoleCommand(const std::string& str) {
    // Add console processing here

    TextInput t(TextInput::FROM_STRING, str);
    if (t.hasMore() && (t.peek().type() == Token::SYMBOL)) {
        std::string cmd = toLower(t.readSymbol());
        if (cmd == "exit") {
            exit(0);
            return;
        } else if (cmd == "help") {
            printConsoleHelp();
            return;
        }

        // Add commands here
    }

    console->printf("Unknown command\n");
    printConsoleHelp();
}

void App::printConsoleHelp() {
    console->printf("exit          - Quit the program\n");
    console->printf("help          - Display this text\n\n");
    console->printf("~/ESC         - Open/Close console\n");
    console->printf("TAB           - Enable first-person camera control\n");
}

void App::onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    // Append any models to the array that you want rendered by onGraphics
    model->pose(posed3D);
    posed3D.append(ifsModel->pose());
}


CoordinateFrame fromXYZYPR(float x, float y, float z, float yaw, float pitch, float roll) {
    Matrix3 rotation = Matrix3::fromAxisAngle(Vector3::unitY(), yaw);
    
    rotation = Matrix3::fromAxisAngle(rotation.column(0), pitch) * rotation;
    rotation = Matrix3::fromAxisAngle(rotation.column(2), roll) * rotation;
    
    const Vector3 translation(x, y, z);

    return CoordinateFrame(rotation, translation);
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting);

    // Setup lighting
    rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

        // Sample rendering code
        Draw::axes(CoordinateFrame(Vector3(0, 4, 0)), rd);
//        Draw::sphere(Sphere(Vector3::zero(), 0.5f), rd, Color3::white());
//        Draw::box(AABox(Vector3(-3,-0.5,-0.5), Vector3(-2,0.5,0.5)), rd, Color3::green());

        
        rd->beginPrimitive(RenderDevice::POINTS);
        rd->setColor(Color3::black());
        for (int i = 0; i < points.size(); ++i) {
            rd->sendVertex(points[i]);
        }
        rd->endPrimitive();

        Draw::axes(fromXYZYPR(0,0,0,toRadians(45), toRadians(90), toRadians(45)), rd);

    rd->disableLighting();

    sky->renderLensFlare(rd, localSky);

    PosedModel2D::sortAndRender(rd, posed2D);
}


G3D_START_AT_MAIN();



int main(int argc, char** argv) {
/*
    RenderDevice* rd = new RenderDevice();
    rd->init();
    GuiTheme::makeThemeFromSourceFiles("C:/Projects/data/source/themes/osx/", "white.png", "black.png", "osx.txt", "C:/Projects/G3D/data-files/gui/osx.skn");
//    return 0;
    */

    GApp::Settings set;
//    set.window.width = 1440;
//    set.window.height = 900;
//    set.window.fsaaSamples = 4;
    return App(set).run();
}

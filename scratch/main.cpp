/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the GApp
  infrastructure is helpful for most projects, you are not restricted
  to using it--choose the level of support that is best for your
  project.  You can also customize GApp through its members and change
  its behavior by overriding methods.

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
    VideoOutput::Ref    video;
	ArticulatedModelRef model;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onLogic();
    virtual void onNetwork();
    virtual bool onEvent(const GEvent& e) {
        /*
        if (e.type == GEventType::KEY_DOWN) {
            debugPrintf("Received key code %d\n", e.key.keysym.sym);
            }*/
        return GApp::onEvent(e);
    }
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};

App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
}


void App::onInit() {

    setDesiredFrameRate(60);

//	map = BSPMap::fromFile("D:/morgan/data/quake3/AriaDeCapo/ariadecapo.pk3", "ariadecapo.bsp");
//	map = BSPMap::fromFile("D:/morgan/data/quake3/charon/map-charon3dm11v2.pk3", "charon3dm11v2.bsp");

    sky = Sky::fromFile(System::findDataFile("sky"));

	model = ArticulatedModel::fromFile(System::findDataFile("horse.ifs"), 4.0f);

    skyParameters = SkyParameters(G3D::toSeconds(10, 00, 00, AM));
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

    debugPane->addNumberBox("Log", &f, "s", GuiTheme::LOG_SLIDER, low, high);
    debugPane->addNumberBox("Linear", &f, "s", GuiTheme::LINEAR_SLIDER, low, high);


    static Array<std::string> list;
    list.append("First");
    for (int i = 0; i < 10; ++i) {
        list.append(format("Item %d", i + 2));
    }
    list.append("Last");
    static int index = 0;
    debugPane->addDropDownList("List", &index, &list);

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
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    if (map.notNull()) {
        map->render(rd, defaultCamera);
    //    Draw::box(map->bounds(), rd);
    }
    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting);

    /*
    // Show normals
    for (int i = 0; i < posed3D.size(); ++i) {
        rd->setObjectToWorldMatrix(posed3D[i]->coordinateFrame());
        Draw::vertexNormals(posed3D[i]->objectSpaceGeometry(), rd);
    }
    */

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

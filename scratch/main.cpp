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
#include "CameraSplineManipulator.h"
#include "GuiWindow.h"
#include "GuiPane.h"

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif

class App : public GApp2 {
public:
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;

    Vector2 lastMouse;
    CameraSplineManipulator::Ref splineManipulator;

    App(const GApp2::Settings& settings = GApp2::Settings());

    virtual void onInit();
    virtual void onLogic();
    virtual void onNetwork();
    virtual bool onEvent(const GEvent& e) {
        return GApp2::onEvent(e);
    }
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onGraphics(RenderDevice* rd);
    virtual void onUserInput(UserInput* ui);
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
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
    
    splineManipulator = CameraSplineManipulator::create(&defaultCamera);
    addModule(splineManipulator);
    
    dataDir = "/Volumes/McGuire/Projects/data/";
    //dataDir = "X:/morgan/data/";

    GuiSkinRef skin = GuiSkin::fromFile(dataDir + "gui/osx.skn");
    GFontRef arialFont = GFont::fromFile(dataDir + "font/arial.fnt");
    GFontRef iconFont = GFont::fromFile(dataDir + "font/icon.fnt");
    skin->setFont(arialFont, 12, Color3::black(), Color4::clear());

    GuiWindow::Ref gui = GuiWindow::create
        (GuiText("Camera Spline", NULL, 9),
         Rect2D::xywh(600, 200, 150, 120),
         skin,
         GuiWindow::TOOL_FRAME_STYLE);

    GuiPane* pane = gui->pane();

    pane->addLabel("Record");
 
    //    gui->addCheckBox("Controller active", defaultController.pointer(), &FirstPersonManipulator::active, &FirstPersonManipulator::setActive);
    static bool active = true;
    pane->addCheckBox("Controller active", &active);//defaultController.pointer(), &FirstPersonManipulator::active, &FirstPersonManipulator::setActive);

    const std::string STOP = "<";
    const std::string PLAY = "4";
    const std::string RECORD = "=";

    enum Mode {STOP_MODE, PLAY_MODE, RECORD_MODE};
    static Mode mode = STOP_MODE;

    GuiRadioButton* b;
    b = pane->addRadioButton(GuiText(RECORD, iconFont, 16, Color3::red() * 0.5f), RECORD_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
    Rect2D baseRect = Rect2D::xywh(b->rect().x0(), b->rect().y0(), 30, 30);
    b->setRect(baseRect + Vector2(baseRect.width() * 0, 0));

    b = pane->addRadioButton(GuiText(PLAY, iconFont, 16), PLAY_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
    b->setRect(baseRect + Vector2(baseRect.width() * 1, 0));

    b = pane->addRadioButton(GuiText(STOP, iconFont, 16), STOP_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
    b->setRect(baseRect + Vector2(baseRect.width() * 2, 0));
    
    addModule(gui);

    {
    enum Fruit {ORANGE, BANANA, PLUM};
    static Fruit fruit = ORANGE;
    static float f = 0.5;
    static bool b = false;

    {
        GuiWindow::Ref gui2 = GuiWindow::create("Second Window", Rect2D::xywh(100,100,400,240), skin);

        GuiPane* pane = gui2->pane();
        pane->addCheckBox("Option", &b);
        pane->addCheckBox("Other window visible", gui.pointer(), &GuiWindow::visible, &GuiWindow::setVisible);

        GuiPane* radioPane = pane->addPane("", 100, GuiPane::ORNATE_FRAME_STYLE);
        radioPane->addRadioButton("Orange", ORANGE, &fruit);
        radioPane->addRadioButton("Banana", BANANA, &fruit);
        radioPane->addRadioButton("Plum", PLUM, &fruit);

        pane->addSlider("Slider", &f, 0.0f, 1.0f);
        addModule(gui2);
    }
    }
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
    debugPrintf("Mode = %d", splineManipulator->mode());

    if (ui->keyPressed(GKey::F1)) {
        setCameraManipulator(defaultController);
        defaultController->setActive(true);
        splineManipulator->setMode(CameraSplineManipulator::RECORD_KEY_MODE);
        splineManipulator->clear();
    }

    if (ui->keyPressed(GKey::F2)) {
        defaultController->setActive(false);
        setCameraManipulator(splineManipulator);
        splineManipulator->setMode(CameraSplineManipulator::PLAY_MODE);
        splineManipulator->setTime(0);
    } 

    if (ui->keyPressed(GKey::F3)) {
        setCameraManipulator(defaultController);
        splineManipulator->setMode(CameraSplineManipulator::INACTIVE_MODE);
        defaultController->setActive(true);
    } 
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

void App::onGraphics(RenderDevice* rd) {
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    // Setup lighting
    rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

        // Sample rendering code
        Draw::axes(CoordinateFrame(Vector3(0, 4, 0)), rd);
        Draw::sphere(Sphere(Vector3::zero(), 0.5f), rd, Color3::white());
        Draw::box(AABox(Vector3(-3,-0.5,-0.5), Vector3(-2,0.5,0.5)), rd, Color3::green());

        // Always render the installed GModules or the console and other
        // features will not appear.
        renderGModules(rd);
    rd->disableLighting();

    sky->renderLensFlare(rd, localSky);
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp2::Settings s;
    s.window.resizable = true;
    return App(s).run();
}

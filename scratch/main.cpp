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
#include "CameraControl.h"

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
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onGraphics(RenderDevice* rd);
    virtual void onUserInput(UserInput* ui);
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};

App::App(const GApp2::Settings& settings) : GApp2(settings) {}

class Person {
private:
    bool    myFriend;

public:
    enum Gender {MALE, FEMALE};

    Gender  gender;        
    float   height;
    bool    likesCats;
    std::string name;

    void setIsMyFriend(bool f) {
        myFriend = f;
    }

    bool getIsMyFriend() const {
        return myFriend;
    }
    
};

Person player;

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
    addWidget(splineManipulator);
    
    GFontRef arialFont = GFont::fromFile(System::findDataFile("arial.fnt"));
    GuiSkinRef skin = GuiSkin::fromFile(System::findDataFile("osx.skn"), arialFont);

    // OS X doesn't have a two button mouse by default.  On all other platforms,
    // default to right button for flying so that the mouse can be used with the Gui.
#   ifndef G3D_OSX
        defaultController->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
#   endif

    GuiWindow::Ref gui = CameraControlWindow::create(defaultController, skin);
    addWidget(gui);
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

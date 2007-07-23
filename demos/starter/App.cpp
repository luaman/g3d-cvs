/**
  @file App.cpp
 */
#include "App.h"

App::App(const GApp::Settings& settings) : GApp(settings) {
    // Uncomment the next line if you are running under a debugger:
    // catchCommonExceptions = false;

    // Uncomment the next line to hide the developer tools:
    //developerWindow->setVisible(false);
}

void App::onInit() {
    // Called before the application loop beings.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.
    sky = Sky::fromFile(dataDir + "sky/");

    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();

    // Example debug GUI:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugWindow->setVisible(true);

    toneMap->setEnabled(false);
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

bool App::onEvent(const GEvent& e) {
    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    return false;
}

void App::onUserInput(UserInput* ui) {
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}

void App::onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    // Append any models to the array that you want rendered by onGraphics
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    toneMap->beginFrame(rd);
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    // Setup lighting
    rd->enableLighting();
        rd->setLight(0, localLighting->lightArray[0]);
        rd->setAmbientLightColor(localLighting->ambientAverage());

        // Sample immediate-mode rendering code
        Draw::axes(CoordinateFrame(Vector3(0, 4, 0)), rd);
        Draw::sphere(Sphere(Vector3::zero(), 0.5f), rd, Color3::white());
        Draw::box(AABox(Vector3(-3,-0.5,-0.5), Vector3(-2,0.5,0.5)), rd, Color3::green());

        // Always render the posed models passed in or the Developer Window and
        // other Widget features will not appear.
        if (posed3D.size() > 0) {
            Vector3 lookVector = renderDevice->getCameraToWorldMatrix().lookVector();
            PosedModel::sort(posed3D, lookVector, opaque, transparent);
            
            for (int i = 0; i < opaque.size(); ++i) {
                opaque[i]->render(renderDevice);
            }

            for (int i = 0; i < transparent.size(); ++i) {
                transparent[i]->render(renderDevice);
            }
        }
    rd->disableLighting();

    sky->renderLensFlare(rd, localSky);
    toneMap->endFrame(rd);

    PosedModel2D::sortAndRender(rd, posed2D);
}

void App::onConsoleCommand(const std::string& str) {
    // Add console processing here

    TextInput t(TextInput::FROM_STRING, str);
    if (t.hasMore() && (t.peek().type() == Token::SYMBOL)) {
        std::string cmd = toLower(t.readSymbol());
        if (cmd == "exit") {
            setExitCode(0);
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
    console->printf("F2            - Enable first-person camera control\n");
}

void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}

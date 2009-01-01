/**
  @file App.cpp
 */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 800; 
    settings.window.height      = 600;

#   ifdef G3D_WIN32
        // On unix-like operating systems, icompile automatically
        // copies data files.  On Windows, we just run from the data
        // directory.
        if (fileExists("data-files")) {
            chdir("data-files");
        }
#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
}


void App::onInit() {
    // Called before the application loop beings.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.

    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);
    showRenderingStats = true;

    sky = Sky::fromFile(System::findDataFile("sky"));

    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();

    toneMap->setEnabled(false);

    /////////////////////////////////////////////////////////////
    // Example of how to add debugging controls
    debugPane->addButton("Exit", this, &App::endProgram);
    
    debugPane->addLabel("Add more debug controls");
    debugPane->addLabel("in App::onInit().");

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));
}


void App::onAI() {
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.
}


bool App::onEvent(const GEvent& e) {
    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((e.type == GEventType::GUI_ACTION) && (e.gui.control == m_button)) { ... return true;}
    // if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::TAB)) { ... return true; }

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
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    toneMap->beginFrame(rd);
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    // Render all objects (or, you can call PosedModel methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)
    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting);

    // Sample immediate-mode rendering code
    rd->pushState();
        rd->enableLighting();

        for (int i = 0; i < localLighting->lightArray.size(); ++i) {
            rd->setLight(i, localLighting->lightArray[i]);
        }
        rd->setAmbientLightColor(localLighting->ambientAverage());

        Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);
        Draw::sphere(Sphere(Vector3(2.5f, 0, 0), 0.5f), rd, Color3::white());
        Draw::box(AABox(Vector3(-3.0f, -0.5f, -0.5f), Vector3(-2.0f, 0.5f, 0.5f)), rd, Color3::green());

        // Call to make the GApp show the output of debugDraw
        drawDebugShapes();
    rd->popState();

    sky->renderLensFlare(rd, localSky);
    toneMap->endFrame(rd);

    // Render 2D objects like Widgets
    PosedModel2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}


void App::endProgram() {
    m_endProgram = true;
}

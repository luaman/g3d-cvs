/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 960; 
    settings.window.height      = 600;

    std::string s = FileSystem::currentDirectory();

    chdir("D:/morgan/G3D8/scratch-morgan");

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

    // Example of using a callback; you can also listen for events in onEvent or bind controls to data
    m_sceneDropDownList = debugPane->addDropDownList("Scene", Scene::sceneNames(), NULL, GuiControl::Callback(this, &App::loadScene));
    debugPane->addButton(GuiText("q", GFont::fromFile(System::findDataFile("icon.fnt")), 14), this, &App::loadScene, GuiTheme::TOOL_BUTTON_STYLE)->moveRightOf(m_sceneDropDownList);


    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    m_shadowMap = ShadowMap::create();

    loadScene();

    {
        RenderDevice* rd = renderDevice;
        Texture::Ref t = Texture::createEmpty("Test", 32, 32, ImageFormat::RGBA32F(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
        Shader::Ref s = Shader::fromStrings(
            "",
            STR(
            void main() {
                gl_FragColor.rgba = vec4(-1.0, 0.5, 1.0, 1.0);
            }
            ));
        Framebuffer::Ref f = Framebuffer::create("Frambuffer");

        // Write some constant
        f->set(Framebuffer::COLOR0, t);
        rd->push2D(f);
        rd->clear();
        rd->setShader(s);
        Draw::rect2D(f->rect2DBounds(), rd);
        rd->pop2D();

        debugPane->addTextureBox(t);
    }

    debugWindow->pack();
    debugWindow->moveTo(Vector2(0, window()->height() - debugWindow->rect().height()));
}


void App::loadScene() {
    const std::string& sceneName = m_sceneDropDownList->selectedValue().text();

    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    // Load the scene
    m_scene = Scene::create(sceneName, defaultCamera);
}


void App::onAI() {
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt; (void)sdt; (void)rdt;
    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.
    m_scene->onSimulation(sdt);
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }
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
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    // Append any models to the arrays that you want to later be rendered by onGraphics()
    m_scene->onPose(surfaceArray);
    (void)surface2D;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    if (m_scene->lighting()->environmentMap.notNull()) {
        Draw::skyBox(rd, m_scene->lighting()->environmentMap);
    }

    // Render all objects (or, you can call Surface methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);

    //////////////////////////////////////////////////////
    // Sample immediate-mode rendering code
    rd->enableLighting();
    for (int i = 0; i < m_scene->lighting()->lightArray.size(); ++i) {
        rd->setLight(i, m_scene->lighting()->lightArray[i]);
    }
    for (int i = 0; i < m_scene->lighting()->shadowedLightArray.size(); ++i) {
        rd->setLight(i + m_scene->lighting()->lightArray.size(), m_scene->lighting()->shadowedLightArray[i]);
    }
    rd->setAmbientLightColor(m_scene->lighting()->ambientAverage());

    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);
    Draw::sphere(Sphere(Vector3(2.5f, 0.5f, 0), 0.5f), rd, Color3::white(), Color4::clear());
    Draw::box(AABox(Vector3(-2.0f, 0.0f, -0.5f), Vector3(-1.0f, 1.0f, 0.5f)), rd, Color4(Color3::orange(), 0.25f),
              Color3::black());

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}


void App::endProgram() {
    m_endProgram = true;
}

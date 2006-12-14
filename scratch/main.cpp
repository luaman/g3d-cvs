/**
  @file demos/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the
  GApp/GApplet infrastructure is helpful for most projects,
  you are not restricted to using it-- choose the level of
  support that is best for your project (see the G3D Map in the
  documentation).

  @author Morgan McGuire, matrix@graphics3d.com
 */

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 70000)
    #error Requires G3D 7.00
#endif


/**
 This simple demo applet uses the debug mode as the regular
 rendering mode so you can fly around the scene.
 */
class Demo : public GApplet {
public:

    // Add state that should be visible to this applet.
    // If you have multiple applets that need to share
    // state, put it in the App.

    class App*          app;

    IFSModelRef         model;

    ToneMapRef          toneMap;

    TextureRef          screen;
    TextureEffects      effects;

    Demo(App* app);

    virtual ~Demo() {}

    virtual void onInit();

    virtual void onLogic();

	virtual void onNetwork();

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onGraphics(RenderDevice* rd);

    virtual void onUserInput(UserInput* ui);

    virtual void onCleanup();

};



class App : public GApp {
protected:
    int main();
public:
    SkyRef              sky;

    Demo*               applet;

    App(const GApp::Settings& settings);

    ~App();
};


Demo::Demo(App* _app) : GApplet(_app), app(_app) {

    model = IFSModel::fromFile(app->dataDir + "ifs/teapot.ifs");
}


void Demo::onInit()  {
    // Called before Demo::run() beings
    app->debugCamera.setPosition(Vector3(0, 2, 10));
    app->debugCamera.lookAt(Vector3(0, 2, 0));

    toneMap = ToneMap::create();

    screen = Texture::createEmpty("Screen", app->renderDevice->width(), app->renderDevice->height(), TextureFormat::RGBA8, Texture::DIM_2D_NPOT, Texture::Settings::video());
    GApplet::onInit();
}


void Demo::onCleanup() {
    // Called when Demo::run() exits
}


void Demo::onLogic() {
    // Add non-simulation game logic and AI code here
}


void Demo::onNetwork() {
	// Poll net messages here
}


void Demo::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
}


void Demo::onUserInput(UserInput* ui) {
    if (ui->keyPressed(SDLK_ESCAPE)) {
        // Even when we aren't in debug mode, quit on escape.
        endApplet = true;
        app->endProgram = true;
    }

	// Add other key handling here
	
	//must call GApplet::onUserInput
	GApplet::onUserInput(ui);
}


void Demo::onGraphics(RenderDevice* rd) {

//    toneMap->beginFrame(rd);
    
    LightingParameters lighting(G3D::toSeconds(11, 00, 00, AM));

    rd->setProjectionAndCameraMatrix(app->debugCamera);

    // Cyan background
    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    rd->clear(app->sky.isNull(), true, true);
    if (app->sky.notNull()) {
        app->sky->render(rd, lighting);
    }

    // Setup lighting
    rd->enableLighting();
		rd->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
		rd->setAmbientLightColor(lighting.ambient);

		Draw::axes(CoordinateFrame(Vector3(0, 4, 0)), rd);

        Draw::sphere(Sphere(Vector3::zero(), 0.5f), rd, Color3::white());
        Draw::box(AABox(Vector3(-3,-0.5,-0.5), Vector3(-2,0.5,0.5)), rd, Color3::green());

        rd->setSpecularCoefficient(1.0);
        rd->setShininess(100);
        rd->setColor(Color3::blue());
        model->pose(Vector3(2.5f, 0, 0))->render(rd);

    rd->disableLighting();

    if (app->sky.notNull()) {
        app->sky->renderLensFlare(rd, lighting);
    }

    // Gaussian blur the screen
    rd->push2D();
        screen->copyFromScreen(rd->viewport());
        effects.gaussianBlur(rd, screen, screen);
        rd->setTexture(0, screen);
        Draw::rect2D(rd->viewport(), rd);
    rd->pop2D();

  //  toneMap->endFrame(rd);
}


int App::main() {
	setDebugMode(true);
	debugController->setActive(false);
    debugController->setPosition(Vector3(4, 2, -2));
    debugController->lookAt(Vector3::zero());

    // Load objects here
    sky = Sky::fromFile(dataDir + "sky/");
    
    applet->run();

    return 0;
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    applet = new Demo(this);
}


App::~App() {
    delete applet;
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
	GApp::Settings settings;
    App(settings).run();
    return 0;
}

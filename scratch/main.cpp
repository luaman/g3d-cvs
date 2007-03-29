/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the
  GApp2 infrastructure is helpful for most projects, you are not 
  restricted to using it--choose the level of support that is best 
  for your project.  You can also customize GApp2 through its 
  members and change

  @author Morgan McGuire, morgan3d@users.sf.net
 */

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 70000)
    #error Requires G3D 7.00
#endif



class App : public GApp2 {
public:
    SkyRef              sky;

    App(const GApp2::Settings& settings = GApp2::Settings());

    virtual void onInit();

    virtual void onLogic();

	virtual void onNetwork();

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onGraphics(RenderDevice* rd);

    virtual void onUserInput(UserInput* ui);

    virtual void onCleanup();

    ~App();
};


void App::onInit()  {
    // Called before Demo::run() beings
    defaultCamera.setPosition(Vector3(0, 2, 10));
    defaultCamera.lookAt(Vector3(0, 2, 0));
}


void App::onCleanup() {
    // Called when Demo::run() exits
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

    static FramebufferRef fbo = FrameBuffer::create("Fbo");
    static TextureRef texture = Texture::createEmpty("Screen", rd->width(), rd->height(), TextureFormat::RGBA8, Texture::DIM_2D_NPOT, Texture::Settings::video());

    fbo->set(FrameBuffer::COLOR_ATTACHMENT0, texture);
    fbo->set(FrameBuffer::DEPTH_ATTACHMENT, RenderBuffer::createEmpty("Depth", rd->width(), rd->height(), TextureFormat::DEPTH32));

    rd->setFramebuffer(fbo);
    LightingParameters lighting(G3D::toSeconds(11, 00, 00, AM));

    rd->setProjectionAndCameraMatrix(defaultCamera);

    // Cyan background
    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    rd->clear(sky.isNull(), true, true);
    if (sky.notNull()) {
        sky->render(rd, lighting);
    }

    // Setup lighting
    rd->enableLighting();
		rd->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
		rd->setAmbientLightColor(lighting.ambient);

		Draw::axes(CoordinateFrame(Vector3(0, 4, 0)), rd);

        Draw::sphere(Sphere(Vector3::zero(), 0.5f), rd, Color3::white());
        Draw::box(AABox(Vector3(-3,-0.5,-0.5), Vector3(-2,0.5,0.5)), rd, Color3::green());

    rd->disableLighting();

    if (sky.notNull()) {
        sky->renderLensFlare(rd, lighting);
    }

    rd->setFramebuffer(NULL);

    rd->push2D();
        GaussianBlur::apply(rd, texture);
        //rd->setTexture(0, texture);
        //Draw::rect2D(texture->rect2DBounds(), rd);
    rd->pop2D();

    renderGModules(rd);
}


App::App(const GApp2::Settings& settings) : GApp2(settings) {
    defaultController->setActive(false);
    
    // Load objects here
    if (fileExists(dataDir + "sky/sun.jpg")) {
        sky = Sky::fromFile(dataDir + "sky/");
    }
}


App::~App() {
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    return App().run();
}

/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the
  GApp/GApplet infrastructure is helpful for most projects,
  you are not restricted to using it-- choose the level of
  support that is best for your project.

  @author Morgan McGuire, morgan3d@users.sf.net
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
}


void Demo::onInit()  {
    // Called before Demo::run() beings
    app->debugCamera.setPosition(Vector3(0, 2, 10));
    app->debugCamera.lookAt(Vector3(0, 2, 0));

    GApplet::onInit();
}


void Demo::onCleanup() {
    // Called when Demo::run() exits
  GApplet::onCleanup();
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
    if (ui->keyPressed(GKey::ESCAPE)) {
        // Even when we aren't in debug mode, quit on escape.
        endApplet = true;
        app->endProgram = true;
    }

	// Add other key handling here
	
	//must call GApplet::onUserInput
	GApplet::onUserInput(ui);
}


void Demo::onGraphics(RenderDevice* rd) {

    static FramebufferRef fbo = FrameBuffer::create("Fbo");
    static TextureRef texture = Texture::createEmpty("Screen", rd->width(), rd->height(), TextureFormat::RGBA8, Texture::DIM_2D_NPOT, Texture::Settings::video());

    fbo->set(FrameBuffer::COLOR_ATTACHMENT0, texture);
    fbo->set(FrameBuffer::DEPTH_ATTACHMENT, RenderBuffer::createEmpty("Depth", rd->width(), rd->height(), TextureFormat::DEPTH32));

    rd->setFramebuffer(fbo);

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

    rd->disableLighting();

    if (app->sky.notNull()) {
        app->sky->renderLensFlare(rd, lighting);
    }

    rd->setFramebuffer(NULL);

    rd->push2D();
        GaussianBlur::apply(rd, texture);
        //rd->setTexture(0, texture);
        //Draw::rect2D(texture->rect2DBounds(), rd);
    rd->pop2D();

}


int App::main() {
    /*
    Shader::fromStrings("",
        "   uniform sampler2D source;\n"
        "   uniform vec2      pixelStep;\n"
        "   \n"
        "    void main() {\n"
        "       const int kernelSize = 3;\n"
        "       float gaussCoef[3] = float[3](1.0,1.0,1.0);\n"
        "       \n"
        "       vec2 pixel = gl_TexCoord[0].xy;         \n"
        "       vec4 sum = texture2D(source, pixel) * gaussCoef[0];\n"
        "       \n"
        "       for (int tap = 1; tap < kernelSize; ++tap) {\n"
        "           sum += texture2D(source, pixelStep * (float(tap) - float(kernelSize - 1) * 0.5) + pixel) * gaussCoef[tap];\n"
        "       }\n"
        "       \n"
        "       gl_FragColor = sum;\n"
        "   }"
    );

                        exit(0);
                        */
    setDebugMode(true);
    debugController->setActive(false);

    // Load objects here
    if (fileExists(dataDir + "sky/sun.jpg")) {
        sky = Sky::fromFile(dataDir + "sky/");
    }
    
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
    settings.useNetwork = false;
    return App(settings).run();
}

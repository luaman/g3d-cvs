/**
  @file scratch/main.cpp
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

    // for on-screen rendering
    Framebuffer::Ref    fb;
    Texture::Ref        colorBuffer;

    ShadowMap::Ref      shadowMap;
    VideoOutput::Ref    video;
    ArticulatedModel::Ref model;

    ArticulatedModel::Ref   ground;

    Film::Ref           film;

    DirectionHistogram* histogram;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onAI();
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


App::App(const GApp::Settings& settings) : GApp(settings), histogram(NULL) {
    catchCommonExceptions = false;
}


void App::onInit() {

    film = Film::create();

    Stopwatch timer("Load 3DS");
    ArticulatedModel::PreProcess preprocess;
    preprocess.addBumpMaps = true;
    preprocess.textureDimension = Texture::DIM_2D_NPOT;
    preprocess.parallaxSteps = 0;
    model = ArticulatedModel::fromFile(System::findDataFile("/Volumes/McGuire/Projects/data/3ds/fantasy/sponza/sponza.3DS"), preprocess);
    timer.after("load");

/*
    model = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"));
    Material::Settings spec;
    spec.setLambertian(Color3::black());
    spec.setSpecular(Color3::blue());
    spec.setShininess(20);
    model->partArray[0].triList[0]->material = Material::create(spec);
*/
//    ground = ArticulatedModel::fromFile(System::findDataFile("cube.ifs"), Vector3(6, 0.5f, 6) * sqrtf(3));

    setDesiredFrameRate(1000);

    sky = Sky::fromFile(System::findDataFile("sky"));

    if (sky.notNull()) {
        skyParameters = SkyParameters(G3D::toSeconds(10, 00, 00, AM));
    }

    lighting = Lighting::create();
    {
        GLight L = GLight::spot(Vector3(0, 40, 0), -Vector3::unitY(), 45, Color3::white());
        L.spotSquare = false;
        lighting->shadowedLightArray.append(L);
    }
    shadowMap = ShadowMap::create("Shadow Map");

    fb = Framebuffer::create("Offscreen");
    colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height(), ImageFormat::RGB16F(), Texture::DIM_2D_NPOT, Texture::Settings::video());
    fb->set(Framebuffer::COLOR_ATTACHMENT0, colorBuffer);
    fb->set(Framebuffer::DEPTH_ATTACHMENT, 
        Texture::createEmpty("Depth", renderDevice->width(), renderDevice->height(), ImageFormat::DEPTH24(), Texture::DIM_2D_NPOT, Texture::Settings::video()));

    film->makeGui(debugPane);
/*
    histogram = new DirectionHistogram(220);
    Array<Vector3> v(5000000);
    for (int i = 0; i < v.size(); ++i) {
//        histogram->insert(Vector3::cosHemiRandom(Vector3::unitY()));
//        histogram->insert(Vector3::random());
        v[i] =  Vector3::hemiRandom(Vector3::unitY());
    }
    histogram->insert(v);
*/
    toneMap->setEnabled(false);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
    delete histogram;
    histogram = NULL;
}

void App::onAI() {
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
    if (model.notNull()) {
        model->pose(posed3D, Vector3(0,1,0));
    }

    if (ground.notNull()) {
        ground->pose(posed3D, Vector3(0,-0.5,0));
    }
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);

    rd->pushState(fb);
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3::white() * 0.8f);
    rd->clear(sky.isNull(), true, true);
    if (sky.notNull()) {
        sky->render(rd, localSky);
    }

    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting, shadowMap);

    /*
    Draw::sphere(Sphere(Vector3(0,3,0), 0.2f), rd, Color3::white());
    Draw::axes(rd);
    Draw::sphere(Sphere(Vector3::zero(), 3), rd);
    Draw::box(AABox(Vector3(-3,0,-3), Vector3(3,6,3)), rd);
    */

    if (histogram != NULL) {
        histogram->render(rd);
        Draw::plane(Plane(Vector3::unitY(), Vector3::zero()), rd, Color4(Color3(1.0f, 0.92f, 0.85f), 0.4f), Color4(Color3(1.0f, 0.5f, 0.3f) * 0.3f, 0.5f));
        Draw::axes(rd, Color3::red(), Color3::green(), Color3::blue(), 1.3f);
    }


    /*
    // Show normals
    for (int i = 0; i < posed3D.size(); ++i) {
        rd->setObjectToWorldMatrix(posed3D[i]->coordinateFrame());
        Draw::vertexNormals(posed3D[i]->objectSpaceGeometry(), rd);
    }
    */
    if (sky.notNull()) {
        sky->renderLensFlare(rd, localSky);
    }
    rd->popState();

    film->exposeAndRender(rd, colorBuffer);

    PosedModel2D::sortAndRender(rd, posed2D);
}


G3D_START_AT_MAIN();

int main(int argc, char** argv) {

    //GFont::makeFont(256, "c:/font/courier-128-bold");    exit(0);
    //BinaryOutput b("d:/morgan/test.txt", G3D_LITTLE_ENDIAN);
    //b.writeInt32(1);
    //b.commit(true);

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

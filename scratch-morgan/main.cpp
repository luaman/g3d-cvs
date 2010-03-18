/**
  @file scratch/main.cpp
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#define HISTOGRAM 0


class App : public GApp {
public:

    Lighting::Ref lighting;

    int x;

    ArticulatedModel::Ref model;
    ArticulatedModel::Ref glassModel;

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
    virtual void onGraphics3D (RenderDevice *rd, Array< Surface::Ref > &surface);    
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D);
    virtual void onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};


App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
//    lighting = defaultLighting();
    lighting = Lighting::create();
    lighting->lightArray.append(GLight::directional(Vector3::unitY(), Color3::white()));
    renderDevice->setColorClearValue(Color3::black());
}
/*

static void renderSomething(RenderDevice* rd) {
    rd->setObjectToWorldMatrix(CFrame());
    GCamera camera;
    camera.setPosition(Vector3(0,0,10));
    rd->clear();
    rd->setProjectionAndCameraMatrix(camera);
    Draw::box(Box(Vector3(-5, 3, -1), Vector3(-3, 5, 1)), rd, Color3::red(), Color3::black());
    Draw::box(Box(Vector3(3, -5, -1), Vector3(5, -3, 1)), rd, Color3::white(), Color3::black());  
}
*/

void drawRect(const Rect2D& rect, RenderDevice* rd) {
    rd->beginPrimitive(PrimitiveType::QUADS);
    rd->setTexCoord(0,Vector2(0,0));
    rd->sendVertex(rect.x0y0());

    rd->setTexCoord(0,Vector2(0,1));
    rd->sendVertex(rect.x0y1());

    rd->setTexCoord(0,Vector2(1,1));
    rd->sendVertex(rect.x1y1());

    rd->setTexCoord(0,Vector2(1,0));
    rd->sendVertex(rect.x1y0());

    rd->endPrimitive();
}


void App::onInit() {

    showRenderingStats = true;
    developerWindow->cameraControlWindow->setVisible(false);
    debugWindow->setVisible(true);
    debugWindow->moveTo(Vector2(0, 300));

//    setDesiredFrameRate(30);

    model = ArticulatedModel::fromFile("D:/morgan/data/quake3/charon/map-charon3dm11v2.pk3/maps/charon3dm11v2.bsp");
    {
        m_film->setBloomStrength(0.3f);
        m_film->setBloomRadiusFraction(0.02f);
        ArticulatedModel::Preprocess p;
        Material::Specification s;
        s.setLambertian(Color3(1.0f, 0.7f, 0.2f));
        p.materialOverride = Material::create(s);
        p.xform = Matrix4::scale(2.0f, 2.0f, 2.0f);
        glassModel = ArticulatedModel::fromFile(System::findDataFile("sphere.ifs"), p);
    }

//    debugPane->addTextureBox("", m_colorBuffer0)->zoomToFit();

    debugPane->addButton("Foo");

    x = 5;
    debugPane->addNumberBox("x=", &x, "", GuiTheme::LINEAR_SLIDER, 0, 16, 0);

    GuiTabPane* t = debugPane->addTabPane();

    GuiPane* p0 = t->addTab("Alpha");
    GuiPane* p1 = t->addTab("Beta");
    GuiPane* p2 = t->addTab("Very long long long tab");
    (void)p2;

    p0->addButton("Hello");
    p0->addNumberBox("x=", &x, "", GuiTheme::LINEAR_SLIDER, 0, 16, 0);
    p0->addButton("There");
    p0->addButton("There");
    p0->addButton("There");
    p0->addButton("There");
    p0->addLabel("There");
    p1->addLabel("Second pane");

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));
}



void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
    if (model.notNull()) {
        model->pose(posed3D);
    }
    glassModel->pose(posed3D, Vector3(0,8,0));
}

void App::onGraphics3D (RenderDevice *rd, Array< Surface::Ref >& surface) {
    (void)surface;
    Surface::sortAndRender(rd, defaultCamera, surface, lighting);
    Draw::axes(rd);
}

void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {

    Surface2D::sortAndRender(rd, posed2D);
}

void App::onCleanup() {
}

void App::onAI() {
    // Add non-simulation game logic and AI code here
    
}

void App::onNetwork() {
    // Poll net messages here
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	(void)rdt;
	(void)sdt;
	(void)idt;
    // Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
}

void App::onUserInput(UserInput* ui) {
	(void)ui;

}

void App::onConsoleCommand(const std::string& str) {
	(void)str;
}

void App::printConsoleHelp() {
}


G3D_START_AT_MAIN();


int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    
    //GFont::makeFont(256, "c:/font/arial2");    exit(0);
    //BinaryOutput b("d:/morgan/test.txt", G3D_LITTLE_ENDIAN);
    //b.writeInt32(1);
    //b.commit(true);


    if (false) {
        // Create data resources
        RenderDevice* rd = new RenderDevice();
        rd->init();
        
        if (false) GuiTheme::makeThemeFromSourceFiles("d:/morgan/data/source/themes/osx/", "white.png", "black.png", "osx.txt", "d:/morgan/G3D/data-files/gui/osx.skn");
        if (false) IconSet::makeIconSet("d:/morgan/data/source/icons/tango/", "d:/morgan/G3D/data-files/icon/tango.icn");
        return 0;
    } 
 
    GApp::Settings set;
    set.film.enabled = true;
    return App(set).run();
}

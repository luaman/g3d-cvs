/**
  @file scratch/main.cpp
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#define HISTOGRAM 0

class App : public GApp {
public:

    Lighting::Ref lighting;

    ArticulatedModel::Ref model;

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
    lighting = defaultLighting();
}


static void renderSomething(RenderDevice* rd) {
    rd->setObjectToWorldMatrix(CFrame());
    GCamera camera;
    camera.setPosition(Vector3(0,0,10));
    rd->clear();
    rd->setProjectionAndCameraMatrix(camera);
    Draw::box(Box(Vector3(-5, 3, -1), Vector3(-3, 5, 1)), rd, Color3::red(), Color3::black());
    Draw::box(Box(Vector3(3, -5, -1), Vector3(5, -3, 1)), rd, Color3::white(), Color3::black());

    
}

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


ArticulatedModel::Ref createHeightfield(const Image1::Ref& height, float xzExtent, float yExtent, const Vector2& texScale) {
    ArticulatedModel::Ref model = ArticulatedModel::createEmpty();
    ArticulatedModel::Part& part = model->partArray.next();
    ArticulatedModel::Part::TriList::Ref triList = part.newTriList();

    bool spaceCentered = true;
    bool twoSided = false;
    Vector2 texScale(1.0, 1.0);
    int xCells = 10;
    int zCells = 10;

    MeshAlg::generateGrid(part.geometry.vertexArray, part.texCoordArray, triList->indexArray, height->width() - 1, height->height() - 1, texScale, 
        spaceCentered, twoSided, CFrame(Matrix4::scale(xzExtent, yExtent, xzExtent).upper3x3()), height);
    part.name = "Root";

    triList->primitive = PrimitiveType::TRIANGLES;
    triList->twoSided = false;

    model->updateAll();

    return model;
}

void App::onInit() {

    showRenderingStats = false;
    developerWindow->cameraControlWindow->setVisible(false);
    debugWindow->setVisible(true);
    debugWindow->moveTo(Vector2(0, 300));

    setDesiredFrameRate(30);

    model = createHeightfield(Image1::fromFile("c:/temp/test.png"), 10, 2);

    GuiPane* p = debugPane->addPane("Configuration");
    p->addButton("Hello");

    static int x = 0;
    p->addRadioButton("Test", 0, &x);

    GuiPane* p2 = debugPane->addPane("Configuration", GuiTheme::ORNATE_PANE_STYLE);
    p2->addButton("Hello");

}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
    if (model.notNull()) {
        model->pose(posed3D);
    }
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

    
    //GFont::makeFont(256, "c:/font/courier-128-bold");    exit(0);
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
    set.film.enabled = false;
    return App(set).run();
}

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


void App::onInit() {

    showRenderingStats = true;
    developerWindow->cameraControlWindow->setVisible(false);
    debugWindow->setVisible(true);
    debugWindow->moveTo(Vector2(0, 300));

//    setDesiredFrameRate(30);

    model = ArticulatedModel::createHeightfield("c:/temp/test.png");
    glassModel = ArticulatedModel::fromFile(System::findDataFile("sphere.ifs"), Matrix4::scale(2.0f, 2.0f, 2.0f));

    Material::Settings glass;
    float eta = 2.0f;
    glass.setEta(eta, 1.0f);
    glass.setSpecular(Color3::white() * 0.02f);
    glass.setShininess(SuperBSDF::packedSpecularMirror());
    glass.setTransmissive(Color3::white() * 0.8f);
    glass.setLambertian(0.1f);

    Material::Settings air;
    air.setEta(1.0f, eta);
    air.setTransmissive(Color3::white() * 0.9f);
    air.setSpecular(Color3::black());
    air.setLambertian(0.0f);

    // Outside of model
    ArticulatedModel::Part::TriList::Ref outside = glassModel->partArray[0].triList[0];
    outside->material = Material::create(glass);
    outside->refractionHint = RefractionQuality::DYNAMIC_FLAT;

    // Peak: ~ 750 fps

    // Inside (inverted)
    ArticulatedModel::Part::TriList::Ref inside = glassModel->partArray[0].newTriList(Material::create(air));
    inside->indexArray = outside->indexArray;
    inside->indexArray.reverse();
    inside->refractionHint = RefractionQuality::NONE;
    glassModel->updateAll();

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
    set.film.enabled = true;
    return App(set).run();
}

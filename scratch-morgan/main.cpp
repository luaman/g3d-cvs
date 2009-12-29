/**
  @file scratch/main.cpp
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#define HISTOGRAM 0

class App : public GApp {
public:

    /** As loaded from disk. */
    Texture::Ref   fromDisk;

    /** As rendered-to-texture. */
    Texture::Ref   rendered;

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
}


void App::onInit() {

    showRenderingStats = false;
    defaultCamera.setCoordinateFrame(bookmark("Home"));
    debugWindow->setVisible(true);
    debugWindow->moveTo(Vector2(600, 0));

    fromDisk = Texture::fromFile("sample.png", ImageFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    rendered = Texture::createEmpty("Rendered", fromDisk->width(), fromDisk->height(), ImageFormat::RGBA8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());

    debugWindow->pane()->addTextureBox(fromDisk);
    debugWindow->pane()->addTextureBox(rendered);
}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
}

void App::onGraphics3D (RenderDevice *rd, Array< Surface::Ref >& surface) {
    (void)rd;
    (void)surface;
}

void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    rd->setTexture(0, fromDisk);
    Draw::rect2D(fromDisk->rect2DBounds(), rd);

    rd->setTexture(0, rendered);
    Draw::rect2D(rendered->rect2DBounds() + Vector2(fromDisk->width(), 0), rd);

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


    /*
    RenderDevice* rd = new RenderDevice();
    rd->init();
    GuiTheme::makeThemeFromSourceFiles("d:/morgan/data/source/themes/osx/", "white.png", "black.png", "osx.txt", "d:/morgan/G3D/data-files/gui/osx.skn");
    return 0;
    */
 
 
    GApp::Settings set;
    return App(set).run();
}

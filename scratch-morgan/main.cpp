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

    /** As rendered-to-texture by copying from fromDisk */
    Texture::Ref   copied;

    /** As rendered-to-texture. */
    Texture::Ref   rendered3D;

    Texture::Ref   copiedFromScreen;

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

    showRenderingStats = false;
    developerWindow->cameraControlWindow->setVisible(false);
    debugWindow->setVisible(true);
    debugWindow->moveTo(Vector2(0, 300));

    setDesiredFrameRate(2);

    fromDisk = Texture::fromFile("sample.png", ImageFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    rendered = Texture::createEmpty("Rendered", fromDisk->width(), fromDisk->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    copied = Texture::createEmpty("Copied in shader", fromDisk->width(), fromDisk->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    rendered3D = Texture::createEmpty("Rendered3D", window()->width(), window()->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    copiedFromScreen = Texture::createEmpty("Copied from Screen", window()->width(), window()->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());

    renderDevice->clear();
    renderSomething(renderDevice);
    copiedFromScreen->copyFromScreen(renderDevice->viewport());

    Framebuffer::Ref fb = Framebuffer::create("FB");
    fb->set(Framebuffer::COLOR0, rendered);
    renderDevice->push2D(fb);
        renderDevice->setColorClearValue(Color3::white());
        renderDevice->clear();
        Draw::rect2D(Rect2D::xywh(0, 0, 20, 10), renderDevice, Color3::red());
    renderDevice->pop2D();
    

    fb->set(Framebuffer::COLOR0, copied);
    renderDevice->push2D(fb);
        renderDevice->setColorClearValue(Color3::white());
        renderDevice->clear();

        Shader::Ref shader = Shader::fromStrings(
            STR(#version 150 compatibility\n
            void main() {
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
            }),

            STR(#version 150 compatibility\n
            /* layout(origin_upper_left)*/
            in vec4 gl_FragCoord;
            out vec4 gl_FragColor;
            uniform sampler2D src;
 
            void main() {
                gl_FragColor.rgb = texelFetch(src, ivec2(gl_FragCoord.xy), 0).rgb;
            }
            ));
        shader->args.set("src", fromDisk);
        renderDevice->setShader(shader);
        Draw::rect2D(fromDisk->rect2DBounds(), renderDevice);
    renderDevice->pop2D();

    fb->set(Framebuffer::COLOR0, rendered3D);
    renderDevice->pushState(fb);
        debugAssert(renderDevice->invertY());
        renderSomething(renderDevice);
    renderDevice->popState();



    GuiTextureBox* a = debugWindow->pane()->addTextureBox(fromDisk);
    GuiTextureBox* b = debugWindow->pane()->addTextureBox(rendered);
    b->moveRightOf(a);
    GuiTextureBox* c = debugWindow->pane()->addTextureBox(copied);
    c->moveRightOf(b);
    GuiTextureBox* d = debugWindow->pane()->addTextureBox(rendered3D);
    d->moveRightOf(c);
    GuiTextureBox* e = debugWindow->pane()->addTextureBox(copiedFromScreen);
    e->moveRightOf(d);
    debugWindow->pack();
}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
}

void App::onGraphics3D (RenderDevice *rd, Array< Surface::Ref >& surface) {
    (void)surface;
    renderSomething(rd);
}

void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    rd->setTexture(0, fromDisk);
    drawRect(fromDisk->rect2DBounds(), rd);

    rd->setTexture(0, rendered);
    drawRect(rendered->rect2DBounds() + Vector2(fromDisk->width(), 0), rd);

    rd->setTexture(0, copied);
    drawRect(copied->rect2DBounds() + Vector2(fromDisk->width() * 2, 0), rd);

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
    set.film.enabled = false;
    return App(set).run();
}

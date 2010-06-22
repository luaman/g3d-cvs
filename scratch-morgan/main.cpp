/**
  @file scratch/main.cpp
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Scene.h"

#define HISTOGRAM 0


class App : public GApp {
public:

    ShadowMap::Ref          m_shadowMap;

    PhysicsFrameSpline      m_spline;

    int                     x;

    Shader::Ref             shader;
    Scene::Ref              m_scene;

    MD3Model::Ref           m_model;
    MD3Model::Pose          m_pose;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onAI();
    virtual void onNetwork();
    virtual bool onEvent(const GEvent& e) {
        
        if (e.type == GEventType::KEY_DOWN) {
            if (e.key.keysym.sym == 'r') {
GCamera c;
    m_scene = Scene::create("Crates", c);
  //          debugPrintf("Received key code %d\n", e.key.keysym.sym);
                return true;
            }
        }
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
    renderDevice->setColorClearValue(Color3::black());
    chdir("d:/morgan/G3D/scratch-morgan");

    defaultCamera.setFieldOfView(toRadians(90), GCamera::VERTICAL);
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

static bool half = false;
void App::onInit() {


    Material::Ref a = Material::createDiffuse(Color3::red());
    Material::Ref b = Material::createDiffuse(Color3::red());

    bool c = *a == *b;

    showRenderingStats = true;
    developerWindow->cameraControlWindow->setVisible(false);
    developerWindow->setVisible(false);
    debugWindow->setVisible(false);
    debugWindow->moveTo(Vector2(0, 300));

    setDesiredFrameRate(60);

    std::string path = "d:/morgan/data/md3/chaos-marine/models/players/Chaos-Marine/";
    MD3Model::Skin::Ref skin = MD3Model::Skin::create(path, "lower_blue.skin", "upper_blue.skin", "head_blue.skin");

//    std::string path = "D:/morgan/data/md3/dragon/models/players/dragon/";
//    MD3Model::Skin::Ref skin = MD3Model::Skin::create(path, "Percy");

    m_model = MD3Model::fromDirectory(path, skin);

    m_shadowMap = ShadowMap::create();

    debugPane->addCheckBox("Half size viewport", &half);

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

//    m_scene = Scene::create("Crates", defaultCamera);


    //shader = Shader::fromStrings("", "uniform vec2 viewportOrigin; void main() {  gl_FragColor.rgb = (gl_FragCoord.xy - viewportOrigin).xyy / 100; }");
}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
    if (m_scene.notNull()) {
        m_scene->onPose(posed3D);
    }
    static float a = 0;
    a += 0.5f;
    m_pose.rotation[1] = Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(a));
    m_model->simulatePose(m_pose, 1.0f / 60.0f);
    m_model->pose(posed3D, CFrame::fromXYZYPRDegrees(0,10,0,00,0), m_pose);
}


void App::onGraphics3D (RenderDevice *rd, Array< Surface::Ref >& surface) {
    (void)surface;
    rd->setAmbientLightColor(Color3::white() * 0.5f);
    rd->setLight(0, GLight::directional(Vector3(1,1,1), Color3::white()));
    for (int i = 0; i < surface.size(); ++i) {
        surface[i]->render(rd);
    }

    if (m_scene.notNull()) {

        Draw::skyBox(rd, m_scene->skyBox());

        /*
            static RealTime t0 = System::time();
            float t = System::time() - t0;
            CFrame c = m_spline.evaluate(t);
            Draw::axes(c, rd, Color3::black(), Color3::black(), Color3::black(), 0.5f);
            Draw::sphere(Sphere(c.translation, 0.1f), rd);
            Draw::physicsFrameSpline(m_spline, rd);
        */
        Surface::sortAndRender(rd, defaultCamera, surface, m_scene->lighting(), m_shadowMap);
 //       Draw::lighting(m_scene->lighting(), rd);
/*
        rd->setLineWidth(5);
        rd->beginPrimitive(PrimitiveType::LINES);
            Vector3 c(0,2,0);

            // 30 degrees
            float k = sqrt(3.0f)/2.0;
            rd->setColor(Color3::blue());
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(0.5,k,0)*5);
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(-0.5,k,0)*5);

            // 45 degrees
            float p = sqrt(2.0f)/2.0;
            rd->setColor(Color3::yellow());
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(p,p,0)*5);
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(-p,p,0)*5);


            // 60 degrees
            rd->setColor(Color3::red());
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(k,0.5,0)*5);
            rd->sendVertex(c);
            rd->sendVertex(c - Vector3(-k,0.5,0)*5);
        rd->endPrimitive();
        */
    } else {
        rd->push2D();
        rd->setColor(Color3::white());
        rd->beginPrimitive(PrimitiveType::TRIANGLES);
        rd->sendVertex(Vector2(0,0));
        rd->sendVertex(Vector2(10,100));
        rd->sendVertex(Vector2(100,50));
        rd->endPrimitive();
        rd->pop2D();
    }
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
    if (m_scene.notNull()) {
        m_scene->onSimulation(idt);
    }
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
        
        if (false) GuiTheme::makeThemeFromSourceFiles("d:/morgan/data/source/themes/osx/", "white.png", "black.png", "osx.txt", "d:/morgan/G3D/data-files/gui/osx.gtm");
        if (false) IconSet::makeIconSet("d:/morgan/data/source/icons/tango/", "d:/morgan/G3D/data-files/icon/tango.icn");
        return 0;
    } 
 
    GApp::Settings set;
    set.film.enabled = false;
    set.window.msaaSamples = 0;
    set.window.stencilBits = 0;
    set.window.resizable = true;
    return App(set).run();
}

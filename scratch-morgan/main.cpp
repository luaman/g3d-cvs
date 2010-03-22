/**
  @file scratch/main.cpp
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#define HISTOGRAM 0


class PhysicsFrameSpline : public Spline<PhysicsFrame> {
public:

    virtual void correct(PhysicsFrame& frame) const {
        frame.rotation.unitize();
    }

    virtual void ensureShortestPath(PhysicsFrame* A, int N) const {
        for (int i = 1; i < N; ++i) {
            const Quat& p = A[i - 1].rotation;
            Quat& q = A[i].rotation;

            float cosphi = p.dot(q);

            if (cosphi < 0) {
                // Going the long way
                q = -q;
            }        
        }
    }
};

// TODO: move to spline
void renderPhysicsFrameSpline(PhysicsFrameSpline& spline, RenderDevice* rd) {
    rd->pushState();
    for (int i = 0; i < spline.control.size(); ++i) {
        const CFrame& c = spline.control[i];

        Draw::axes(c, rd, Color3::red(), Color3::green(), Color3::blue(), 0.5f);
        Draw::sphere(Sphere(c.translation, 0.1f), rd, Color3::white(), Color4::clear());
    }

    const int N = spline.control.size() * 30;
    CFrame last = spline.evaluate(0);
    const float a = 0.5f;
    rd->setLineWidth(1);
    rd->beginPrimitive(PrimitiveType::LINES);
    for (int i = 1; i < N; ++i) {
        float t = (spline.control.size() - 1) * i / (N - 1.0f);
        const CFrame& cur = spline.evaluate(t);
        rd->setColor(Color4(1,1,1,a));
        rd->sendVertex(last.translation);
        rd->sendVertex(cur.translation);

        rd->setColor(Color4(1,0,0,a));
        rd->sendVertex(last.rightVector() + last.translation);
        rd->sendVertex(cur.rightVector() + cur.translation);

        rd->setColor(Color4(0,1,0,a));
        rd->sendVertex(last.upVector() + last.translation);
        rd->sendVertex(cur.upVector() + cur.translation);

        rd->setColor(Color4(0,0,1,a));
        rd->sendVertex(-last.lookVector() + last.translation);
        rd->sendVertex(-cur.lookVector() + cur.translation);

        last = cur;
    }
    rd->endPrimitive();
    rd->popState();
}

class PoseSpline {
public:
    typedef Table<std::string, PhysicsFrameSpline> SplineTable;
    SplineTable partSpline;

    /**
    The Any must be a table mapping part names to PhysicsFrameSplines

    
    PoseSpline {
       wheel = PhysicsSpline {
           control = (
              // Each element must be either
              (0, CFrame::fromXYZYPR(...)), // Optional: time

              CFrame::fromXYZYPR(...))
           ),

           cyclic = false
       ),
    }

    */
    PoseSpline(const Any& any) {
        (void)any;
    }
 
    void get(float t, ArticulatedModel::Pose& pose) {
        for (SplineTable::Iterator it = partSpline.begin(); it.hasMore(); ++it) {
            pose.cframe.set(it->key, it->value.evaluate(t));
        }
    }
};

class App : public GApp {
public:

    Lighting::Ref           lighting;

    ArticulatedModel::Ref   model;
    ArticulatedModel::Ref   glassModel;

    PhysicsFrameSpline      m_spline;

    int                     x;

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

    showRenderingStats = false;
    developerWindow->cameraControlWindow->setVisible(false);
    debugWindow->setVisible(false);
    debugWindow->moveTo(Vector2(0, 300));

    setDesiredFrameRate(60);

    if (false) {
        model = ArticulatedModel::fromFile("D:/morgan/data/quake3/charon/map-charon3dm11v2.pk3/maps/charon3dm11v2.bsp");
    }
    if (false) {
        m_film->setBloomStrength(0.3f);
        m_film->setBloomRadiusFraction(0.02f);
        ArticulatedModel::Preprocess p;
        Material::Specification s;
        s.setLambertian(Color3(1.0f, 0.7f, 0.2f));
        p.materialOverride = Material::create(s);
        p.xform = Matrix4::scale(2.0f, 2.0f, 2.0f);
        glassModel = ArticulatedModel::fromFile(System::findDataFile("sphere.ifs"), p);
    }


    lighting = defaultLighting();

    m_spline.append(CFrame::fromXYZYPRDegrees(0, 0, 0));
    m_spline.append(CFrame::fromXYZYPRDegrees(0, 3, 3, 0, 45));
    m_spline.append(CFrame::fromXYZYPRDegrees(3, 3, 0));
    m_spline.append(CFrame::fromXYZYPRDegrees(3, 6, 0, 45));
    m_spline.append(CFrame::fromXYZYPRDegrees(6, 6, 3, 90, 0, 45));
    m_spline.append(CFrame::fromXYZYPRDegrees(6, 3, 3, 0, -45));
    m_spline.cyclic = true;


    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));
}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    (void)posed3D;
    if (model.notNull()) {
        model->pose(posed3D);
    }
//    glassModel->pose(posed3D, Vector3(0,8,0));
}

void App::onGraphics3D (RenderDevice *rd, Array< Surface::Ref >& surface) {
    static RealTime t0 = System::time();
    float t = System::time() - t0;

    CFrame c = m_spline.evaluate(t);
    Draw::axes(c, rd, Color3::black(), Color3::black(), Color3::black(), 0.5f);
    Draw::sphere(Sphere(c.translation, 0.1f), rd);

    (void)surface;
    Draw::skyBox(rd, lighting->environmentMap);
    Surface::sortAndRender(rd, defaultCamera, surface, lighting);
    renderPhysicsFrameSpline(m_spline, rd);
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

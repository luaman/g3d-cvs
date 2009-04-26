/**
  @file scratch/main.cpp
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 80000)
#   error Requires G3D 8.00
#endif


class App : public GApp {
public:
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;
    BSPMapRef           map;

    VAR                 data;

    // for on-screen rendering
    Framebuffer::Ref    fb;
    Texture::Ref        colorBuffer;

    ShadowMap::Ref      shadowMap;
    VideoOutput::Ref    video;
    ArticulatedModel::Ref model;

    ArticulatedModel::Ref   ground;

    bool                updating;
    IFSModel::Ref       ifs;

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
    updating = true;
    debugPane->addCheckBox("Update Frustum", &updating);

    /*
    Framebuffer::Ref m_mainScene_FBO = Framebuffer::create(std::string("Main Offscreen Target"));
    Texture::Ref m_mainScene_tex = Texture::createEmpty(std::string("Main Rendered Texture"), 256, 256);
    m_mainScene_FBO->set(Framebuffer::COLOR_ATTACHMENT0, m_mainScene_tex);
    */

    //ground = ArticulatedModel::fromFile(System::findDataFile("cube.ifs"), Vector3(6, 0.5f, 6) * sqrtf(3));
    //model = ArticulatedModel::createCornellBox();

    setDesiredFrameRate(1000);


    sky = Sky::fromFile(System::findDataFile("sky"));

    if (sky.notNull()) {
        skyParameters = SkyParameters(G3D::toSeconds(5, 00, 00, PM));
    }

//    lighting = Lighting::fromSky(sky, skyParameters, Color3::white() * 0.5f);
    lighting = Lighting::create();
    lighting->ambientTop = Color3::white() * 0.2f;
    lighting->ambientBottom = Color3::zero();
    {
        CFrame c = CFrame::fromXYZYPRDegrees(0, 32, -9, 180, -60, 0);
        GLight L = GLight::spot(c.translation, c.lookVector(), 45, Color3::white());

  //      L = GLight::directional(-c.lookVector(), Color3::white());
 //       L.rightDirection = c.rightVector();

      L = GLight::directional(Vector3(0, 0.86f, -0.5f), Color3::white());

//        L = skyParameters.directionalLight();

        lighting->lightArray.append(L);
    }
    shadowMap = ShadowMap::create("Shadow Map");

    Stopwatch timer("Load 3DS");
    ArticulatedModel::PreProcess preprocess;
    preprocess.addBumpMaps = false;
    preprocess.textureDimension = Texture::DIM_2D_NPOT;
    preprocess.parallaxSteps = 0;
//    model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/data/3ds/fantasy/sponza/sponza.3DS"), preprocess);
    model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/data/ifs/horse.ifs"), preprocess);
//    model = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"));


//    model = ArticulatedModel::fromFile(System::findDataFile("/Volumes/McGuire/Projects/data/3ds/fantasy/sponza/sponza.3DS"), preprocess);
    timer.after("load 3DS");

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

    defaultCamera.setCoordinateFrame(bookmark("Home"));
    defaultCamera.setFieldOfView(toRadians(60), GCamera::HORIZONTAL);
    defaultCamera.setFarPlaneZ(-inf());

    toneMap->setEnabled(false);
}


void App::onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    if (model.notNull()) {
        static float a = 0;
        //a += 0.001f;
        CFrame f = Matrix3::fromAxisAngle(Vector3::unitY(), a);
        model->pose(posed3D, f);
    }

    if (ifs.notNull()) {
        posed3D.append(ifs->pose());
    }

    if (ground.notNull()) {
        ground->pose(posed3D, Vector3(0,-0.5,0));
    }
}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {

    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);

//    rd->pushState(fb);

    rd->setColorClearValue(Color3(0.2f, 1.0f, 2.0f));
    rd->setProjectionAndCameraMatrix(defaultCamera);
    rd->clear(true, true, true);

    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3::white() * 0.8f);
    rd->clear(true, true, true);
    if (sky.notNull()) {
        sky->render(rd, localSky);
    }

    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting);//, shadowMap);
/*
    {
        GLight& light = lighting->shadowedLightArray[0];
        Draw::sphere(Sphere(light.position.xyz(), 0.1f), rd, Color3::white());
        
        CFrame lightCFrame = light.position.xyz();
        lightCFrame.lookAt(light.position.xyz() + light.spotDirection);
        GCamera lightCamera;
        lightCamera.lookAt(light.spotDirection);
        lightCamera.setCoordinateFrame(lightCFrame);
        lightCamera.setFieldOfView(toRadians(light.spotCutoff) * 2, GCamera::HORIZONTAL);
        lightCamera.setNearPlaneZ(-0.01f);
        lightCamera.setFarPlaneZ(-10.01f);
        Draw::frustum(lightCamera.frustum(shadowMap->rect2DBounds()), rd);
    }
*/
    for (int i = 0; i < posed3D.size(); ++i) {
        Draw::sphere(posed3D[i]->worldSpaceBoundingSphere(), rd, Color4::clear(), Color3::black());
    }
    Draw::axes(rd);


    static GCamera::Frustum f;
    static Ray r0, r1;
    
    if (updating) {
       f = defaultCamera.frustum(rd->viewport());
       r0 = defaultCamera.worldRay(0,0,rd->viewport());
       r1 = defaultCamera.worldRay(rd->viewport().width(),rd->viewport().height(),rd->viewport());
    }
    Draw::ray(r0, rd);
    Draw::ray(r1, rd);
    rd->setDepthWrite(false);
    for (int i = 1; i < 5; ++i) {
        Draw::plane(f.faceArray[i].plane, rd);
    }
    Draw::frustum(f, rd);
    


    /*
    Draw::sphere(Sphere(Vector3(0,3,0), 0.2f), rd, Color3::white());
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
//    rd->popState();
//    film->exposeAndRender(rd, colorBuffer);

    PosedModel2D::sortAndRender(rd, posed2D);
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
}

void App::printConsoleHelp() {
}


G3D_START_AT_MAIN();


/** Embeds \a N elements to reduce allocation time and increase 
    memory coherence when working with arrays of arrays.
    Offers a limited subset of the functionality of G3D::Array.*/
template<class T, int N>
class SmallArray {
private:
    int                 m_size;

    /** First N elements */
    T                   m_embedded[N];

    /** Remaining elements */
    Array<T>            m_rest;

public:

    SmallArray() : m_size(0) {}

    inline int size() const {
        return m_size;
    }

    inline T& operator[](int i) {
        debugAssert(i < m_size && i >= 0);
        if (i < N) {
            return m_embedded[i];
        } else {
            return m_rest[i - N];
        }
    }

    inline const T& operator[](int i) const {
        debugAssert(i < m_size && i >= 0);
        if (i < N) {
            return m_embedded[i];
        } else {
            return m_rest[i - N];
        }
    }

    inline void push(const T& v) {
        ++m_size;
        if (m_size <= N) {
            m_embedded[m_size - 1] = v;
        } else {
            m_rest.append(v);
        }
    }

    inline void append(const T& v) {
        push(v);
    }

    void fastRemove(int i) {
        debugAssert(i < m_size && i >= 0);
        if (i < N) {
            if (m_size <= N) {
                // Exclusively embedded
                m_embedded[i] = m_embedded[m_size - 1];
            } else {
                // Move one down from the rest array
                m_embedded[i] = m_rest.pop();
            }
        } else {
            // Removing from the rest array
            m_rest.fastRemove(i - N);
        }
        --m_size;
    }

    T pop() {
        debugAssert(m_size > 0);
        if (m_size <= N) {
            // Popping from embedded, don't need a temporary
            --m_size;
            return m_embedded[m_size];
        } else {
            // Popping from rest
            --m_size;
            return m_rest.pop();
        }
    }

    void popDiscard() {
        debugAssert(m_size > 0);
        if (m_size > N) {
            m_rest.popDiscard();
        }
        --m_size;
    }
};


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
    /*
    set.window.fullScreen = true;
    set.window.framed = false;
    set.window.width = 1600;
    set.window.height = 1200;
    set.window.alphaBits = 0;
    set.window.stencilBits = 0;
    set.window.refreshRate = 0;
    */

    return App(set).run();
}

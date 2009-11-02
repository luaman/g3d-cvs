/**
  @file scratch/main.cpp
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#define HISTOGRAM 0

class App : public GApp {
public:
    LightingRef             lighting;
    SkyParameters           skyParameters;
    SkyRef                  sky;
    BSPMapRef               map;

    VertexRange             data;

    ShadowMap::Ref          shadowMap;
    VideoOutput::Ref        video;
    ArticulatedModel::Ref   model;

    ArticulatedModel::Ref   ground;

    bool                    updating;
    IFSModel::Ref           ifs;


    DirectionHistogram*     histogram;

    // Direct probability from evaluate()
    DirectionHistogram*     backwardHistogram;

    // Russian roulette
    DirectionHistogram*     backwardRRHistogram;

    Array<Surface::Ref> transparent;

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
    virtual void onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);
    virtual void onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};


App::App(const GApp::Settings& settings) : GApp(settings), histogram(NULL) {
    catchCommonExceptions = false;
}


void App::onInit() {

    if (true) {
        Texture::Ref t = Texture::fromFile("D:/morgan/G3D/data-files/image/checkerboard.jpg");
        GuiTextureBox* b = debugPane->addTextureBox("Texture", t);
        b->setSize(300, 300);
        b->zoomTo1();
        debugPane->pack();
    }
    

    //ground = ArticulatedModel::fromFile(System::findDataFile("cube.ifs"), Vector3(6, 0.5f, 6) * sqrtf(3));
    //ground = ArticulatedModel::createCornellBox(11.0f);

    /*
    {
        Stopwatch timer1("load3DS");
        ArticulatedModel::Ref sphere = ArticulatedModel::fromFile(System::findDataFile("bunny.ifs"));
        timer1.after("done");

        Material::Settings glass;
        glass.setEta(1.4f);
        //glass.setTransmissive(Color3::green() * 0.7f);
        glass.setTransmissive(Color3::white());
        glass.setSpecular(Color3::black());
        glass.setLambertian(Color3::black());
        
        Material::Settings air;
        air.setEta(1.0f);
        air.setTransmissive(Color3::white() * 1.0f);
        air.setSpecular(Color3::black());
        air.setLambertian(0.0f);
        
        // Outside of sphere
        ArticulatedModel::Part::TriList::Ref outside = sphere->partArray[0].triList[0];
        outside->material = Material::create(glass);
        // Inside (inverted)
        ArticulatedModel::Part::TriList::Ref inside = 
            sphere->partArray[0].newTriList(Material::create(air));
        inside->indexArray = outside->indexArray;
        inside->indexArray.reverse();
        Stopwatch timer("updateAll");
        sphere->updateAll();
        timer.after("done");
        model = sphere;
    }
    */

    setDesiredFrameRate(30);

    //sky = Sky::fromFile(System::findDataFile("sky"));

    if (sky.notNull()) {
        skyParameters = SkyParameters(G3D::toSeconds(5, 00, 00, PM));

        lighting = Lighting::fromSky(sky, skyParameters, Color3::white() * 0.5f);
        lighting->lightArray.append(lighting->shadowedLightArray);
        lighting->shadowedLightArray.clear();
    }

    lighting = Lighting::create();
    lighting->ambientTop = Color3::white() * 0.2f;
    lighting->ambientBottom = Color3::zero();
    {
        CFrame c = CFrame::fromXYZYPRDegrees(0, 32, -9, 180, -60, 0);
        GLight L = GLight::spot(c.translation, c.lookVector(), 45, Color3::white());

  //      L = GLight::directional(-c.lookVector(), Color3::white());
 //       L.rightDirection = c.rightVector();

      L = GLight::directional(Vector3(-1, 1, 0.5f), Color3::white());

//        L = skyParameters.directionalLight();

        lighting->lightArray.append(L);
    }
    shadowMap = ShadowMap::create("Shadow Map");
    
    Stopwatch timer("Load 3DS");
    if (false) {
        ArticulatedModel::PreProcess preprocess;
        preprocess.addBumpMaps = true;
        preprocess.textureDimension = Texture::DIM_2D_NPOT;
        preprocess.parallaxSteps = 1;
        ArticulatedModel::Settings s;
        s.weld.normalSmoothingAngle = inf();
        model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/G3D/data-files/3ds/fantasy/sponza/sponza.3DS"), preprocess);
//        model = ArticulatedModel::fromFile(System::findDataFile("sphere.ifs"), preprocess);
    }
//    model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/data/3ds/fantasy/sponza/sponza.3DS"), preprocess);
//    model = ArticulatedModel::fromFile(System::findDataFile("/Volumes/McGuire/Projects/data/3ds/fantasy/sponza/sponza.3DS"), preprocess);
//    model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/data/ifs/horse.ifs"), preprocess);

//    model = ArticulatedModel::fromFile(System::findDataFile("d:/morgan/data/ifs/teapot.ifs"), 3);

    timer.after("load 3DS");
/*
    fb = Framebuffer::create("Offscreen");
    colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height(), ImageFormat::RGB16F(), Texture::DIM_2D_NPOT, Texture::Settings::video());
    fb->set(Framebuffer::COLOR_ATTACHMENT0, colorBuffer);
    fb->set(Framebuffer::DEPTH_ATTACHMENT, 
        Texture::createEmpty("Depth", renderDevice->width(), renderDevice->height(), ImageFormat::DEPTH24(), Texture::DIM_2D_NPOT, Texture::Settings::video()));

    film->makeGui(debugPane);
    */
#if HISTOGRAM
    
    G3D::Random r;
    // Num samples
    const int N = 100000;//1200000;
    int slices = 24;

    histogram = new DirectionHistogram(slices);
    Array<Vector3> v(N);
    Array<float> weight(N);
    v.fastClear();
    weight.fastClear();

    // Glossiness
    const float g = 20;
    SuperBSDF::Ref bsdf = SuperBSDF::create(Color4(Color3::white() * 0.6f),
        Color4(Color3::white() * 0.3f, SuperBSDF::packSpecularExponent(g)), Color3::black());
    const Vector3 n = Vector3::unitZ();
    const Vector2 t = Vector2::zero();
    const float theta = toRadians(45);
    const Vector3 w_i(sin(theta), 0, cos(theta));
    const Color3  P_i = Color3::white();

    // Scattering according to f(w_i, w_o)*(w_o dot n)
    timer.after("");
    while (v.size() < N) {
        Color3 P_o;
        Vector3 w_o;
        float  eta_o;
        Color3 extinction_o;
        if (bsdf->scatter(n, t, w_i, P_i, w_o, P_o, eta_o, extinction_o, r)) {
            debugAssert(w_o.isUnit());
            v.append(w_o);
            weight.append(P_o.average());
        }
   
        // Simple hemi
        //v.next(); r.hemi(v.last().x, v.last().y, v.last().z); weight.append(v.last().z); 
    }
    timer.after("scatter");


    histogram->insert(v, weight);

    v.clear();
    weight.clear();
    backwardHistogram = new DirectionHistogram(slices);

    timer.after("");
    // explicitly sampling f(w_i, w_o)
    while (v.size() < N) {
        Vector3 w_o;
        r.cosHemi(w_o.x, w_o.y, w_o.z);
        const Color3& P_o = bsdf->evaluate(n, t, w_i, P_i, w_o).rgb();

        v.append(w_o);
        weight.append(P_o.average());
        
        // v.next(); r.cosHemi(v.last().x, v.last().y, v.last().z);  weight.append(1.0f);

    }    
    timer.after("explicit sample");

    backwardHistogram->insert(v, weight);
    
    v.clear();
    weight.clear();
    backwardRRHistogram = new DirectionHistogram(slices);

    timer.after("");
    // Russian roulette against f(w_i, w_o)
    while (v.size() < N) {
        Vector3 w_o;
        r.cosHemi(w_o.x, w_o.y, w_o.z);
        const Color3& P_o = bsdf->evaluate(n, t, w_i, P_i, w_o).rgb();

        if (P_o.average() >= r.uniform()) {
            // Accept
            v.append(w_o);
            weight.append(1.0f);

            alwaysAssertM(P_o.average() < 1.0f, "Energy conservation violated");
        }
    }    
    timer.after("russian roulette");
    backwardRRHistogram->insert(v, weight);
#endif

    defaultCamera.setCoordinateFrame(bookmark("Home"));
    defaultCamera.setFieldOfView(toRadians(60), GCamera::HORIZONTAL);
    defaultCamera.setFarPlaneZ(-inf());

}


void App::onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    (void)posed2D;
    if (model.notNull()) {
        model->pose(posed3D, Vector3(-1,0,0));
    }

    if (ifs.notNull()) {
        posed3D.append(ifs->pose());
    }

    if (ground.notNull()) {
        ground->pose(posed3D, Vector3(0,2.0,0));
    }
}


void App::onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    screenPrintf("Desired frame rate: %f\n", desiredFrameRate());

    (void)posed3D;
    rd->setColorClearValue(Color3::white());
    rd->clear();
    rd->setProjectionAndCameraMatrix(defaultCamera);

    if (histogram != NULL) {
        screenPrintf("White  = Importance sample f()");
        screenPrintf("Red    = Direct evaluation of f()");
        screenPrintf("Yellow = Russian roulette on f()");

        rd->pushState();
        rd->setObjectToWorldMatrix(CFrame(Matrix3::fromAxisAngle(Vector3::unitX(), -toRadians(90)), Vector3(0,0,-1.0f)));
        histogram->render(rd, Color3::white());
        rd->popState();

        rd->pushState();
        rd->setObjectToWorldMatrix(CFrame(Matrix3::fromAxisAngle(Vector3::unitX(), -toRadians(90)), Vector3(0,0,1.0f)));
        backwardHistogram->render(rd, Color3::red());
        rd->popState();

        rd->pushState();
        rd->setObjectToWorldMatrix(CFrame(Matrix3::fromAxisAngle(Vector3::unitX(), -toRadians(90)), Vector3(0,0,3.0f)));
        backwardRRHistogram->render(rd, Color3::yellow());
        rd->popState();

        Draw::plane(Plane(Vector3::unitY(), Vector3::zero()), rd, Color4(Color3(1.0f, 0.92f, 0.85f), 0.4f), Color4(Color3(1.0f, 0.5f, 0.3f) * 0.3f, 0.5f));
    }


    Surface::sortAndRender(rd, defaultCamera, posed3D, lighting);
#if 0
    // Show normals
    for (int i = 0; i < posed3D.size(); ++i) {
        rd->setObjectToWorldMatrix(posed3D[i]->coordinateFrame());
        Draw::vertexNormals(posed3D[i]->objectSpaceGeometry(), rd);
    }
#endif
#if 0
    Array<Color3> data(colorBuffer->width() * colorBuffer->height());

    GLuint pbo;
    glGenBuffersARB(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER_ARB, data.size() * sizeof(Color3), NULL, GL_STREAM_READ);
    // Prime the buffer by performing one readback
    colorBuffer->getTexImage((void*)0, ImageFormat::RGB32F());
    glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

    debugAssertGLOk();

    // Let everything finish so that we aren't measuring the blocking time
    glFinish();
    System::sleep(1);


    debugPrintf("\n");
    Stopwatch st;
    // readpixels    
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb->openGLID());
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadPixels(0, 0, colorBuffer->width(), colorBuffer->height(), GL_RGB, GL_FLOAT, (void*)0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    glReadBuffer(GL_NONE);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    st.after("glReadPixels (PBO) ");

    // Clear the pending read 
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    
   
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb->openGLID());
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadPixels(0, 0, colorBuffer->width(), colorBuffer->height(), GL_RGB, GL_FLOAT, data.getCArray());
    glReadBuffer(GL_NONE);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    st.after("glReadPixels (mem) ");


    colorBuffer->getTexImage(data.getCArray(), ImageFormat::RGB32F());
    st.after("glGetTexImage (mem)");


    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    colorBuffer->getTexImage((void*)0, ImageFormat::RGB32F());
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    st.after("glGetTexImage (PBO)");


    // Clear the read 
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
#endif
/*
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo);
    void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
*/

//    film->exposeAndRender(rd, colorBuffer);

    Surface2D::sortAndRender(rd, posed2D);
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
    /*
    set.window.fullScreen = true;
    set.window.framed = false;
    set.window.width = 1600;
    set.window.height = 1200;
    set.window.alphaBits = 0;
    set.window.stencilBits = 0;
    set.window.refreshRate = 0;
    */
//    set.window.width = 512;
//    set.window.height = 512;

    return App(set).run();
}

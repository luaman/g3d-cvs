/**
 @file GApp.cpp
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2003-11-03
 @edited  2010-03-12
 */

#include "G3D/platform.h"

#include "G3D/AnyVal.h"
#include "GLG3D/GApp.h"
#include "G3D/GCamera.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VideoRecordDialog.h"
#include "G3D/ParseError.h"
#include "G3D/FileSystem.h"


namespace G3D {

static GApp* lastGApp = NULL;

void screenPrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    if (lastGApp) {
        lastGApp->vscreenPrintf(fmt, argList);
    }
    va_end(argList);
}

void GApp::vscreenPrintf
(
 const char*                 fmt,
 va_list                     argPtr) {
    if (showDebugText) {
        std::string s = G3D::vformat(fmt, argPtr);
        m_debugTextMutex.lock();
        debugText.append(s);
        m_debugTextMutex.unlock();
    }
}

void debugDraw(const Shape::Ref& shape, const Color4& solidColor, 
               const Color4& wireColor, const CFrame& frame) {
    if (lastGApp) {
        debugAssert(shape.notNull());
        GApp::DebugShape& s = lastGApp->debugShapeArray.next();
        s.shape = shape;
        s.solidColor = solidColor;
        s.wireColor = wireColor;
        s.frame = frame;
    }
}


/** Attempt to write license file */
static void writeLicense() {
    FILE* f = FileSystem::fopen("g3d-license.txt", "wt");
    if (f != NULL) {
        fprintf(f, "%s", license().c_str());
        FileSystem::fclose(f);
    }
}


GApp::GApp(const Settings& settings, OSWindow* window) :
    m_activeVideoRecordDialog(NULL),
    m_renderPeriod(1),
    m_endProgram(false),
    m_exitCode(0),
    m_useFilm(settings.film.enabled),
    m_settings(settings),
    debugPane(NULL),
    renderDevice(NULL),
    userInput(NULL),
    m_debugTextColor(Color3::white()),
    m_debugTextOutlineColor(Color3::black()),
    m_lastFrameOverWait(0),
    lastWaitTime(System::time()),
    m_desiredFrameRate(5000),
    m_simTimeStep(1.0f / 60.0f),
    m_realTime(0), 
    m_simTime(0) {

    lastGApp = this;

    char b[2048];
    getcwd(b, 2048);
    logLazyPrintf("cwd = %s\n", b);
    
    if (settings.dataDir == "<AUTO>") {
        dataDir = demoFindData(false);
    } else {
        dataDir = settings.dataDir;
    }
    System::setAppDataDir(dataDir);

    if (settings.writeLicenseFile && ! FileSystem::exists("g3d-license.txt")) {
        writeLicense();
    }

    renderDevice = new RenderDevice();

    if (window != NULL) {
        _hasUserCreatedWindow = true;
        renderDevice->init(window);
    } else {
        _hasUserCreatedWindow = false;    
        renderDevice->init(settings.window);
    }
    debugAssertGLOk();

    _window = renderDevice->window();
    _window->makeCurrent();
    debugAssertGLOk();

    m_widgetManager = WidgetManager::create(_window);
    userInput = new UserInput(_window);
    defaultController = FirstPersonManipulator::create(userInput);

    {
        TextOutput t;

        t.writeSymbols("System","{");
        t.pushIndent();
        t.writeNewline();
        System::describeSystem(t);
        if (renderDevice) {
            renderDevice->describeSystem(t);
        }

        NetworkDevice::instance()->describeSystem(t);
        t.writeNewline();
        t.writeSymbol("}");
        t.writeNewline();

        std::string s;
        t.commitString(s);
        logPrintf("%s\n", s.c_str());
    }
    defaultCamera  = GCamera();

    debugAssertGLOk();
    loadFont(settings.debugFontName);
    debugAssertGLOk();

    if (defaultController.notNull()) {
        defaultController->onUserInput(userInput);
        defaultController->setMoveRate(10);
        defaultController->setPosition(Vector3(0, 0, 4));
        defaultController->lookAt(Vector3::zero());
        defaultController->setActive(false);
        defaultCamera.setPosition(defaultController->position());
        defaultCamera.lookAt(Vector3::zero());
        addWidget(defaultController);
        setCameraManipulator(defaultController);
    }
 
    showDebugText               = true;
    escapeKeyAction             = ACTION_QUIT;
    showRenderingStats          = true;
    fastSwitchCamera            = true;
    catchCommonExceptions       = true;
    manageUserInput             = true;

    {
        GConsole::Settings settings;
        settings.backgroundColor = Color3::green() * 0.1f;
        console = GConsole::create(debugFont, settings, staticConsoleCallback, this);
        console->setActive(false);
        addWidget(console);
    }

    if (m_useFilm) {
        if (! GLCaps::supports_GL_ARB_shading_language_100() || ! GLCaps::supports_GL_ARB_texture_non_power_of_two()) {
            // This GPU can't support the film class
            *const_cast<bool*>(&m_useFilm) = false;
            logPrintf("Warning: Disabled GApp::Settings::film.enabled because it could not be supported on this GPU.");
        } else {
            const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);
            if (colorFormat == NULL) {
                // This GPU can't support the film class
                *const_cast<bool*>(&m_useFilm) = false;
                logPrintf("Warning: Disabled GApp::Settings::film.enabled because none of the provided color formats could be supported on this GPU.");
            } else {
                m_film = Film::create(colorFormat);
                m_frameBuffer = FrameBuffer::create("GApp::m_frameBuffer");
                resize(renderDevice->width(), renderDevice->height());
            }
        }
    }

    defaultController->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
    defaultController->setActive(true);

    if (settings.useDeveloperTools) {
        UprightSplineManipulator::Ref splineManipulator = UprightSplineManipulator::create(&defaultCamera);
        addWidget(splineManipulator);
        
        GFont::Ref arialFont = GFont::fromFile(System::findDataFile("icon.fnt"));
        GuiTheme::Ref skin = GuiTheme::fromFile(System::findDataFile("osx.skn"), arialFont);

        debugWindow = GuiWindow::create("Debug Controls", skin, 
            Rect2D::xywh(0, settings.window.height - 150, 150, 150), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE);
        debugWindow->setVisible(false);
        debugPane = debugWindow->pane();
        addWidget(debugWindow);

        developerWindow = DeveloperWindow::create
            (this,
             defaultController, 
             splineManipulator,
             Pointer<Manipulator::Ref>(this, &GApp::cameraManipulator, &GApp::setCameraManipulator), 
             m_film,
             skin,
             console,
             Pointer<bool>(debugWindow, &GuiWindow::visible, &GuiWindow::setVisible),
             &showRenderingStats,
             &showDebugText);

        addWidget(developerWindow);
    } else {
        debugPane = NULL;
    }

    debugAssertGLOk();

    m_simTime     = 0;
    m_realTime    = 0;
    lastWaitTime  = System::time();

    renderDevice->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
}


void GApp::drawMessage(const std::string& message) {
    renderDevice->push2D();
    {
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Draw::fastRect2D(renderDevice->viewport(), renderDevice, Color4(Color3::white(), 0.8f));
        debugWindow->theme()->defaultStyle().font->draw2D(renderDevice, message, renderDevice->viewport().center(), 30, 
            Color3::black(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    }
    renderDevice->pop2D();
    renderDevice->swapBuffers();
}


void GApp::setExitCode(int code) {
    m_endProgram = true;
    m_exitCode = code;
}


void GApp::loadFont(const std::string& fontName) {
    std::string filename = System::findDataFile(fontName);
    if (FileSystem::exists(filename)) {
        debugFont = GFont::fromFile(filename);
    } else {
        logPrintf(
            "Warning: G3D::GApp could not load font \"%s\".\n"
            "This may be because the G3D::GApp::Settings::dataDir was not\n"
            "properly set in main().\n",
            filename.c_str());

        debugFont = NULL;
    }
}


GApp::~GApp() {
    if (lastGApp == this) {
        lastGApp = NULL;
    }

    NetworkDevice::cleanup();

    debugFont = NULL;
    delete userInput;
    userInput = NULL;

    renderDevice->cleanup();
    delete renderDevice;
    renderDevice = NULL;

    if (_hasUserCreatedWindow) {
        delete _window;
        _window = NULL;
    }

    VertexBuffer::cleanupAllVARAreas();
}


CoordinateFrame GApp::bookmark(const std::string& name, const CoordinateFrame& defaultValue) const {
    return developerWindow->cameraControlWindow->bookmark(name, defaultValue);
}


int GApp::run() {
    int ret = 0;
    if (catchCommonExceptions) {
        try {
            onRun();
            ret = m_exitCode;
        } catch (const char* e) {
            alwaysAssertM(false, e);
            ret = -1;
        } catch (const GImage::Error& e) {
            alwaysAssertM(false, e.reason + "\n" + e.filename);
            ret = -1;
        } catch (const std::string& s) {
            alwaysAssertM(false, s);
            ret = -1;
        } catch (const TextInput::WrongTokenType& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const TextInput::WrongSymbol& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const VertexAndPixelShader::ArgumentError& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        } catch (const LightweightConduit::PacketSizeException& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        } catch (const ParseError& e) {
            if (e.byte == -1) {
                alwaysAssertM(false, e.filename + format(":%d(%d): ", e.line, e.character) + e.message);
            } else {
                alwaysAssertM(false, e.filename + format(":(byte %d): ", (int)e.byte) + e.message);
            }
            ret = -1;
        } catch (const AnyVal::WrongType& e) {
            alwaysAssertM(false, format("AnyVal::WrongType.  Expected %d, got %d.", e.expected, e.actual));
            ret = -1;
        }
    } else {
        onRun();
        ret = m_exitCode;
    }

    return ret;
}


void GApp::onRun() {
    if (window()->requiresMainLoop()) {
        
        // The window push/pop will take care of 
        // calling beginRun/oneFrame/endRun for us.
        window()->pushLoopBody(this);

    } else {
        beginRun();

        // Main loop
        do {
            oneFrame();   
        } while (! m_endProgram);

        endRun();
    }
}


void GApp::renderDebugInfo() {
    if (debugFont.notNull() && (showRenderingStats || (showDebugText && (debugText.length() > 0)))) {
        // Capture these values before we render debug output
        int majGL  = renderDevice->stats().majorOpenGLStateChanges;
        int majAll = renderDevice->stats().majorStateChanges;
        int minGL  = renderDevice->stats().minorOpenGLStateChanges;
        int minAll = renderDevice->stats().minorStateChanges;
        int pushCalls = renderDevice->stats().pushStates;

        renderDevice->push2D();
            const static float size = 10;
            if (showRenderingStats) {
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                Draw::fastRect2D(Rect2D::xywh(2, 2, renderDevice->width() - 4, size * 5.8 + 2), renderDevice, Color4(0, 0, 0, 0.3f));
            }

            debugFont->begin2DQuads(renderDevice);
            float x = 5;
            Vector2 pos(x, 5);

            if (showRenderingStats) {

                Color3 statColor = Color3::yellow();
                debugFont->begin2DQuads(renderDevice);

                const char* build = 
#               ifdef G3D_DEBUG
                    " (Debug)";
#               else
                    " (Optimized)";
#               endif

                static const std::string description = renderDevice->getCardDescription() + "   " + System::version() + build;
                debugFont->send2DQuads(renderDevice, description, pos, size, Color3::white());
                pos.y += size * 1.5f;
                
                float fps = renderDevice->stats().smoothFrameRate;
                const std::string& s = format(
                    "% 4d fps (% 3d ms)  % 5.1fM tris  GL Calls: %d/%d Maj;  %d/%d Min;  %d push", 
                    iRound(fps),
                    iRound(1000.0f / fps),
                    iRound(renderDevice->stats().smoothTriangles / 1e5) * 0.1f,
                    /*iRound(renderDevice->stats().smoothTriangleRate / 1e4) * 0.01f,*/
                    majGL, majAll, minGL, minAll, pushCalls);
                debugFont->send2DQuads(renderDevice, s, pos, size, statColor);

                pos.x = x;
                pos.y += size * 1.5;

                {
                float g = m_graphicsWatch.smoothElapsedTime();
                float n = m_networkWatch.smoothElapsedTime();
                float s = m_simulationWatch.smoothElapsedTime();
                float L = m_logicWatch.smoothElapsedTime();
                float u = m_userInputWatch.smoothElapsedTime();
                float w = m_waitWatch.smoothElapsedTime();

                float swapTime = renderDevice->swapBufferTimer().smoothElapsedTime();
                float total = g + n + s + L + u + w + swapTime;

                float norm = 100.0f / total;


                // Normalize the numbers
                g *= norm;
                swapTime *= norm;
                n *= norm;
                s *= norm;
                L *= norm;
                u *= norm;
                w *= norm;

                const std::string& str = 
                    format("Time:%3.0f%% Gfx,%3.0f%% Swap,%3.0f%% Sim,%3.0f%% AI,%3.0f%% Net,%3.0f%% UI,%3.0f%% idle", 
                        g, swapTime, s, L, n, u, w);
                debugFont->send2DQuads(renderDevice, str, pos, size, statColor);
                }

                pos.x = x;
                pos.y += size * 1.5;

                const char* esc = NULL;
                switch (escapeKeyAction) {
                case ACTION_QUIT:
                    esc = "ESC: QUIT      ";
                    break;
                case ACTION_SHOW_CONSOLE:
                    esc = "ESC: CONSOLE   ";
                    break;
                case ACTION_NONE:
                    esc = "               ";
                    break;
                }

                const char* video = 
                    (developerWindow.notNull() && 
                     developerWindow->videoRecordDialog.notNull() &&
                     developerWindow->videoRecordDialog->enabled()) ?
                    "F4: SCREENSHOT  F6: MOVIE     " :
                    "                              ";

                const char* camera = 
                    (cameraManipulator().notNull() && 
                     defaultController.notNull()) ? 
                    "F2: CAMERA     " :
                    "               ";
                    
                const char* dev = developerWindow.notNull() ? 
                    "F11: DEV WINDOW":
                    "               ";

                const std::string& Fstr = format("%s     %s     %s    %s", esc, camera, video, dev);
                debugFont->send2DQuads(renderDevice, Fstr, pos, 8, Color3::white());

                pos.x = x;
                pos.y += size;

            }

            m_debugTextMutex.lock();
            for (int i = 0; i < debugText.length(); ++i) {
                debugFont->send2DQuads(renderDevice, debugText[i], pos, size, m_debugTextColor, m_debugTextOutlineColor);
                pos.y += size * 1.5;
            }
            m_debugTextMutex.unlock();
            debugFont->end2DQuads(renderDevice);
        renderDevice->pop2D();
    }
}


bool GApp::onEvent(const GEvent& event) {
    if (event.type == GEventType::VIDEO_RESIZE) {
        resize(event.resize.w, event.resize.h);
        return true;
    }

    return false;
}


Lighting::Ref GApp::defaultLighting() {
    Lighting::Ref lighting = Lighting::create();

    lighting->shadowedLightArray.append(GLight::directional(Vector3(1,2,1), Color3::fromARGB(0xfcf6eb)));
    lighting->lightArray.append(GLight::directional(Vector3(-1,-0.5f,-1), Color3::fromARGB(0x1e324d)));
    lighting->ambientTop    = Color3::fromARGB(0x303842);
    lighting->ambientBottom = Color3::fromARGB(0x262627);

    // Perform our own search first, since we have a better idea of where this directory might be
    // than the general System::findDataFile.  This speeds up loading of the starter app.
    std::string cubePath = "cubemap";
    if (! FileSystem::exists(cubePath)) {
        cubePath = "../data-files/cubemap";
        if (! FileSystem::exists(cubePath)) {
            cubePath = System::findDataFile("cubemap");
        }
    }
    lighting->environmentMap = 
        Texture::fromFile(pathConcat(cubePath, "nooncloudspng/noonclouds_*.png"), 
                          TextureFormat::RGB8(), Texture::DIM_CUBE_MAP,
                          Texture::Settings::cubeMap(), 
                          Texture::Preprocess::gamma(2.1f));
    lighting->environmentMapColor = Color3::one();

    return lighting;
}


void GApp::onGraphics3D(RenderDevice* rd, Array<SurfaceRef>& posed3D) {
    alwaysAssertM(false, "Override onGraphics3D");
    //Surface::sortAndRender(rd, defaultCamera, posed3D, m_lighting);
    drawDebugShapes();
}


void GApp::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    Surface2D::sortAndRender(rd, posed2D);
}


void GApp::onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    // Clear the entire screen (needed even though we'll render over it because
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();
    if (m_useFilm) {
        // Clear the frameBuffer
        rd->pushState(m_frameBuffer);
        rd->clear();
        if (m_colorBuffer0->format()->floatingPoint) {
            // Float render targets don't support line smoothing
            rd->setMinLineWidth(1);
        }
        renderDevice->setMinLineWidth(1);
    } else {
        rd->pushState();
    }

    rd->setProjectionAndCameraMatrix(defaultCamera);
    onGraphics3D(rd, posed3D);

    rd->popState();
    if (m_useFilm) {
        // Expose the film
        m_film->exposeAndRender(rd, m_colorBuffer0, 1);
        rd->setMinLineWidth(0);
    }

    rd->push2D();
    {
        onGraphics2D(rd, posed2D);
    }
    rd->pop2D();
}


void GApp::addWidget(const Widget::Ref& module) {
    m_widgetManager->add(module);
    
    // Ensure that background widgets do not end up on top
    GuiWindow::Ref w = module.downcast<GuiWindow>();
    if (w.notNull()) {
        m_widgetManager->moveWidgetToBack(module);
    }
}


void GApp::removeWidget(const Widget::Ref& module) {
    m_widgetManager->remove(module);
}


void GApp::resize(int w, int h) {
    if (m_useFilm &&
        (m_colorBuffer0.isNull() ||
        (m_colorBuffer0->width() != w) ||
        (m_colorBuffer0->height() != h))) {

        m_frameBuffer->clear();

        const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);
        const ImageFormat* depthFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredDepthFormats);

        m_colorBuffer0 = Texture::createEmpty("GApp::m_colorBuffer0", w, h, 
            colorFormat, Texture::DIM_2D_NPOT, Texture::Settings::video(), 1);

        if (depthFormat) {
            m_depthBuffer  = Texture::createEmpty("GApp::m_depthBuffer", w, h, 
                depthFormat, Texture::DIM_2D_NPOT, Texture::Settings::video(), 1);
        }
        
        m_frameBuffer->set(FrameBuffer::COLOR0, m_colorBuffer0);
        if (depthFormat) {
            m_frameBuffer->set(FrameBuffer::DEPTH, m_depthBuffer);
        }
    }
}

void GApp::oneFrame() {
    for (int repeat = 0; repeat < max(1, m_renderPeriod); ++repeat) {
        lastTime = now;
        now = System::time();
        RealTime timeStep = now - lastTime;

        // User input
        m_userInputWatch.tick();
        if (manageUserInput) {
            processGEventQueue();
        }
        debugAssertGLOk();
        onUserInput(userInput);
        m_widgetManager->onUserInput(userInput);
        m_userInputWatch.tock();

        // Network
        m_networkWatch.tick();
        onNetwork();
        m_widgetManager->onNetwork();
        m_networkWatch.tock();

        // Logic
        m_logicWatch.tick();
        {
            onAI();
            m_widgetManager->onAI();
        }
        m_logicWatch.tock();

        // Simulation
        m_simulationWatch.tick();
        {
            RealTime rdt = timeStep;
            SimTime  sdt = m_simTimeStep / m_renderPeriod;
            SimTime  idt = desiredFrameDuration() / m_renderPeriod;

            onBeforeSimulation(rdt, sdt, idt);
            onSimulation(rdt, sdt, idt);
            m_widgetManager->onSimulation(rdt, sdt, idt);
            onAfterSimulation(rdt, sdt, idt);

            if (m_cameraManipulator.notNull()) {
                defaultCamera.setCoordinateFrame(m_cameraManipulator->frame());
            }

            setRealTime(realTime() + rdt);
            setSimTime(simTime() + sdt);
        }
        m_simulationWatch.tock();
    }


    // Pose
    m_posed3D.fastClear();
    m_posed2D.fastClear();
    m_widgetManager->onPose(m_posed3D, m_posed2D);
    onPose(m_posed3D, m_posed2D);

    // Wait 
    // Note: we might end up spending all of our time inside of
    // RenderDevice::beginFrame.  Waiting here isn't double waiting,
    // though, because while we're sleeping the CPU the GPU is working
    // to catch up.

    m_waitWatch.tick();
    {
        RealTime now = System::time();

        // Compute accumulated time
        RealTime cumulativeTime = now - lastWaitTime;

        // Perform wait for actual time needed
        RealTime desiredWaitTime = max(0.0, desiredFrameDuration() - cumulativeTime);
        onWait(max(0.0, desiredWaitTime - m_lastFrameOverWait) * 0.97);

        // Update wait timers
        lastWaitTime = System::time();
        RealTime actualWaitTime = lastWaitTime - now;

        // Learn how much onWait appears to overshoot by and compensate
        double thisOverWait = actualWaitTime - desiredWaitTime;
        if (abs(thisOverWait - m_lastFrameOverWait) / max(abs(m_lastFrameOverWait), abs(thisOverWait)) > 0.4) {
            // Abruptly change our estimate
            m_lastFrameOverWait = thisOverWait;
        } else {
            // Smoothly change our estimate
            m_lastFrameOverWait = lerp(m_lastFrameOverWait, thisOverWait, 0.1);
        }
    }
    m_waitWatch.tock();


    // Graphics
    renderDevice->beginFrame();
    m_graphicsWatch.tick();
    {
        debugAssertGLOk();
        {
            debugAssertGLOk();
            renderDevice->pushState();
            {
                onGraphics(renderDevice, m_posed3D, m_posed2D);
            }
            renderDevice->popState();
            renderDebugInfo();
        }
    }
    m_graphicsWatch.tock();
    if (m_activeVideoRecordDialog) {
        m_activeVideoRecordDialog->maybeRecord(renderDevice);        
    }
    renderDevice->endFrame();
    debugShapeArray.fastClear();
    debugText.fastClear();

    if (m_endProgram && window()->requiresMainLoop()) {
        window()->popLoopBody();
    }
}


void GApp::drawDebugShapes() {
    renderDevice->setObjectToWorldMatrix(CFrame());
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        const DebugShape& s = debugShapeArray[i];
        s.shape->render(renderDevice, s.frame, s.solidColor, s.wireColor); 
    }
}


void GApp::onWait(RealTime t) {
    System::sleep(max(0.0, t));
}


void GApp::setSimTimeStep(float s) {
    m_simTimeStep = s;
}


void GApp::setRealTime(RealTime r) {
    m_realTime = r;
}


void GApp::setSimTime(SimTime s) {
    m_simTime = s;
}

void GApp::setDesiredFrameRate(float fps) {
    debugAssert(fps > 0);
    m_desiredFrameRate = fps;
}


void GApp::onInit() {}


void GApp::onCleanup() {}


void GApp::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt;
    (void)rdt;
    (void)sdt;
}


void GApp::onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt) {        
    (void)idt;
    (void)rdt;
    (void)sdt;
}

void GApp::onAfterSimulation(RealTime rdt, SimTime sdt, SimTime idt) {        
    (void)idt;
    (void)rdt;
    (void)sdt;
}

void GApp::onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    (void)posed3D;
    (void)posed2D;
}

void GApp::onNetwork() {}


void GApp::onAI() {}


void GApp::beginRun() {

    m_endProgram = false;
    m_exitCode = 0;

    onInit();

    // Move the controller to the camera's location
    if (defaultController.notNull()) {
        defaultController->setFrame(defaultCamera.coordinateFrame());
    }

    now = System::time() - 0.001;
}


void GApp::endRun() {
    onCleanup();

    Log::common()->section("Files Used");
    Set<std::string>::Iterator end = _internal::currentFilesUsed.end();
    Set<std::string>::Iterator f = _internal::currentFilesUsed.begin();
    
    while (f != end) {
        Log::common()->println(*f);
        ++f;
    }
    Log::common()->println("");


    if (window()->requiresMainLoop() && m_endProgram) {
        ::exit(m_exitCode);
    }
}


void GApp::staticConsoleCallback(const std::string& command, void* me) {
    ((GApp*)me)->onConsoleCommand(command);
}


void GApp::onConsoleCommand(const std::string& cmd) {
    if (trimWhitespace(cmd) == "exit") {
        setExitCode(0);
        return;
    }
}


void GApp::onUserInput(UserInput* userInput) {
}

void GApp::processGEventQueue() {
    userInput->beginEvents();

    // Event handling
    GEvent event;
    while (window()->pollEvent(event)) {
        
        // For event debugging
        //if (event.type != GEventType::MOUSE_MOTION) {
        //    printf("%s\n", event.toString().c_str());
        //}

        if (WidgetManager::onEvent(event, m_widgetManager)) {
            continue;
        }

        if (onEvent(event)) {
            // Event was consumed
            continue;
        }

        switch(event.type) {
        case GEventType::QUIT:
            setExitCode(0);
            break;

        case GEventType::KEY_DOWN:

            if (console.isNull() || ! console->active()) {
                switch (event.key.keysym.sym) {
                case GKey::ESCAPE:
                    switch (escapeKeyAction) {
                    case ACTION_QUIT:
                        setExitCode(0);
                        break;
                    
                    case ACTION_SHOW_CONSOLE:
                        console->setActive(true);
                        continue;
                        break;

                    case ACTION_NONE:
                        break;
                    }
                    break;

                case GKey::F2:
                    if (fastSwitchCamera && developerWindow.isNull() && defaultController.notNull()) {
                        defaultController->setActive(! defaultController->active());
                        // Consume event
                        continue;
                    }
                    break;

                // Add other key handlers here
                default:;
                }
            }
        break;

        // Add other event handlers here

        default:;
        }

        userInput->processEvent(event);
    }

    userInput->endEvents();
}

}

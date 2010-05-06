/**
  @file App.cpp
 */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    GApp::Settings settings;

    settings.window.caption = "G3D MultiView Demo";

    settings.window.width       = 1024; 
    settings.window.height      = 600;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
}


void App::onInit() {

    debugWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->cameraControlWindow->moveTo(Vector2(developerWindow->cameraControlWindow->rect().x0(), 0));
    developerWindow->setVisible(false);
    showRenderingStats = false;
    
    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(-0.61369f, 0.734589f, 0.934322f, 314.163f, -12.1352f));

    m_shadowMap = ShadowMap::create();

    m_scene = Scene::create();

    GBuffer::Specification specification;
    specification.wsPosition = true;
    specification.wsNormal = true;
    specification.lambertian = true;
    specification.specular = true;
    m_gbuffer = GBuffer::create("GBuffer", specification);
    m_gbuffer->resize((window()->width() - 4) / 2, window()->height() - GUI_HEIGHT - 2);

    GuiTheme::Ref theme = debugWindow->theme();

    GuiWindow::Ref toolBar = GuiWindow::create("Tools", theme, Rect2D(), GuiTheme::TOOL_WINDOW_STYLE);

    IconSet::Ref icons = IconSet::fromFile(System::findDataFile("tango.icn"));
    GuiPane* toolPane = toolBar->pane();
    toolPane->addButton(icons->get("22x22/uwe/CreateCylinder.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/CreateBox.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/Emitter.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/PointLight.png"), GuiTheme::TOOL_BUTTON_STYLE)->moveBy(Vector2(10,0));
    toolPane->addButton(icons->get("22x22/categories/applications-multimedia.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-graphics.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-system.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolBar->pack();
    addWidget(toolBar);


    GuiWindow::Ref background = GuiWindow::create("", theme, renderDevice->viewport(), GuiTheme::NO_WINDOW_STYLE);

    Vector2 gbufferViewSize(160, 160 * m_gbuffer->height() / m_gbuffer->width());
    GuiPane* pane = background->pane();

    pane->addLabel("Buffers:");
    GuiTextureBox* posBox = pane->addTextureBox("Position", m_gbuffer->wsPosition(), GuiTextureBox::Settings::unitVector());
    posBox->setSizeFromInterior(gbufferViewSize);
    posBox->setShowInfo(false);
    posBox->zoomToFit();

    GuiTextureBox* norBox = pane->addTextureBox("Normal", m_gbuffer->wsNormal(), GuiTextureBox::Settings::unitVector());
    norBox->moveRightOf(posBox);
    norBox->setSizeFromInterior(gbufferViewSize);
    norBox->setShowInfo(false);
    norBox->zoomToFit();

    GuiTextureBox* depBox = pane->addTextureBox("Depth", m_gbuffer->depth(), GuiTextureBox::Settings::depthBuffer());
    depBox->setSizeFromInterior(gbufferViewSize);
    depBox->moveRightOf(norBox);
    depBox->setShowInfo(false);
    depBox->zoomToFit();

    GuiTextureBox* lamBox = pane->addTextureBox("Lambertian", m_gbuffer->lambertian(), GuiTextureBox::Settings::reflectivity());
    lamBox->setSizeFromInterior(gbufferViewSize);
    lamBox->moveRightOf(depBox);
    lamBox->setShowInfo(false);
    lamBox->zoomToFit();

    GuiTextureBox* gloBox = pane->addTextureBox("Glossy", m_gbuffer->specular(), GuiTextureBox::Settings::reflectivity());
    gloBox->setSizeFromInterior(gbufferViewSize);
    gloBox->moveRightOf(lamBox);
    gloBox->setShowInfo(false);
    gloBox->zoomToFit();
    
    GuiTextureBox* shaBox = pane->addTextureBox("Shadow Map", m_shadowMap->depthTexture(), GuiTextureBox::Settings::depthBuffer());
    shaBox->setSizeFromInterior(gbufferViewSize);
    shaBox->moveRightOf(gloBox);
    shaBox->setShowInfo(false);
    shaBox->zoomToFit();
    
    pane->setHeight(GUI_HEIGHT);
    pane->pack();
    background->pack();
    background->setRect(Rect2D::xywh(0, renderDevice->height() - GUI_HEIGHT, renderDevice->width(), GUI_HEIGHT));

    addWidget(background);    

    renderDevice->setColorClearValue(Color3::white());
}

void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    // Append any models to the arrays that you want to later be rendered by onGraphics()
    if (m_scene.notNull()) {
        m_scene->onPose(surfaceArray);
    }
    (void)surface2D;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    // Render G-Buffer
    m_gbuffer->compute(rd, defaultCamera, surface3D);
   

    // Render full shading viewport
    Rect2D shadeViewport = m_gbuffer->rect2DBounds() + Vector2(m_gbuffer->width(), 0) + Vector2(3, 1);
    rd->setViewport(shadeViewport);
    Draw::skyBox(rd, m_scene->lighting()->environmentMap);
    // For convenience we'll just forward render again; a real
    // application would actually use the G-Buffer to perform deferred
    // shading.
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);

    // Render wireframe viewport
    Rect2D wireViewport = m_gbuffer->rect2DBounds() + Vector2(1, 1);
    rd->setViewport(wireViewport);
    rd->push2D();
        Draw::rect2D(wireViewport, rd, Color3::white() * 0.5f);
    rd->pop2D();
    rd->pushState();
    {
        Draw::axes(rd);
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setColor(Color3::black());
        rd->setLineWidth(1);
        for (int i = 0; i < surface3D.size(); ++i) {
            rd->setObjectToWorldMatrix(surface3D[i]->coordinateFrame());
            surface3D[i]->sendGeometry(rd);
        }

        rd->setRenderMode(RenderDevice::RENDER_SOLID);
        Draw::lighting(m_scene->lighting(), rd, true);

        // Call to make the GApp show the output of debugDraw calls
        drawDebugShapes();
    }
    rd->popState();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    Rect2D miniViewport = m_gbuffer->rect2DBounds() + Vector2(1, 1);
    Draw::rect2DBorder(miniViewport, rd);
    Draw::rect2DBorder(miniViewport + Vector2(2 + miniViewport.width(), 0), rd);

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}

/**
  @file App.cpp
 */
#include "App.h"
#include "World.h"
#include "Hit.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.caption     = "G3D Ray Trace Demo";
    settings.window.width       = 640; 
    settings.window.height      = 400;
    settings.film.enabled       = false;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_mode(MODE_RECURSIVE), m_world(NULL), m_maxBounces(3), m_raysPerPixel(1) {}


void App::onInit() {
    message("Loading...");
    m_world = new World();

    showRenderingStats = false;
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    
    // Starting position
    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(24.3f, 0.4f, 2.5f, 68.7f, 1.2f, 0.0f));
    m_prevCFrame = defaultCamera.coordinateFrame();

    makeGUI();
    onRender();
}


void App::makeGUI() {
    GuiWindow::Ref window = GuiWindow::create("Controls", debugWindow->theme(), Rect2D(), GuiTheme::TOOL_WINDOW_STYLE);
    GuiPane* pane = window->pane();
    pane->addLabel("Use WASD keys + right mouse to move");
    pane->addButton("Render High Res.", this, &App::onRender);
    
    pane->addRadioButton("Recursive ray trace (Whitted 80)", MODE_RECURSIVE, &m_mode);
    pane->addRadioButton("Distribution ray trace (Cook et al. 84)", MODE_DISTRIBUTION, &m_mode);
    pane->addRadioButton("Path trace (Kajiya 86)", MODE_PATH, &m_mode);

    pane->addNumberBox("Rays per pixel", &m_raysPerPixel, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);
    pane->addNumberBox("Max bounces", &m_maxBounces, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);
    window->pack();

    window->setVisible(true);
    addWidget(window);
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    if (! m_prevCFrame.fuzzyEq(defaultCamera.coordinateFrame())) {
        // Update the preview image only while moving
        Mode m = m_mode;
        m_mode = MODE_RECURSIVE;
        rayTraceImage(0.18f, 1);
        m_mode = m;
        m_prevCFrame = defaultCamera.coordinateFrame();
    }

    rd->clear();

    if (m_result.notNull()) {
        rd->push2D();
        rd->setTexture(0, m_result);
        Draw::rect2D(rd->viewport(), rd);
        rd->pop2D();
    }

    PosedModel2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    delete m_world;
    m_world = NULL;
}


static G3D::Random rnd(0xF018A4D2, false);

Color3 App::rayTrace(const Ray& ray, World* world, const Color3& extinction_i, int bounce) {
    Color3 radiance = Color3::zero();

    Hit hit;
    float dist = inf();
    if (world->intersect(ray, dist, hit)) {
        const SuperBSDF::Ref& bsdf = hit.material->bsdf();

        // Shade this point (direct illumination)
        for (int L = 0; L < world->lightArray.size(); ++L) {
            const GLight& light = world->lightArray[L];

            // Shadow rays
            if (world->lineOfSight(hit.position + hit.normal * 0.0001f, light.position.xyz())) {
                Vector3 w_L = light.position.xyz() - hit.position;
                float distance2 = w_L.squaredLength();
                w_L /= sqrt(distance2);

                // Attenduated radiance
                const Color3& radiance_L = light.color / distance2;

                radiance += bsdf->evaluate(hit.normal, hit.texCoord, w_L, 
                                           radiance_L, -ray.direction()).rgb() * 
                                           max(0.0f, w_L.dot(hit.normal));
            }
        }

        // Indirect illumination
        switch (m_mode) {
        case MODE_RECURSIVE:
            // Whitted ray tracer:

            // Ambient
            radiance += bsdf->lambertian().sample(hit.texCoord).rgb() * world->ambient;

            if (bounce < m_maxBounces) {
                // Perfect reflection and refraction
                SmallArray<SuperBSDF::Impulse, 3> impulseArray;
                bsdf->getImpulses(hit.normal, hit.texCoord, -ray.direction(), impulseArray);
                
                for (int i = 0; i < impulseArray.size(); ++i) {
                    const SuperBSDF::Impulse& impulse = impulseArray[i];
                    Ray secondaryRay = Ray::fromOriginAndDirection(hit.position, impulse.w).bump(0.0001f);
                    radiance += rayTrace(secondaryRay, world, impulse.extinction, bounce + 1) * impulse.coefficient;
                }
            }
            break;

        case MODE_DISTRIBUTION:
        case MODE_PATH:

            // Distribution or path ray tracer
            if (bounce < m_maxBounces) {
                static const int numSamples = (m_mode == MODE_PATH) ? 1 : 20;
                for (int i = 0; i < numSamples; ++i) {
                    Vector3 w_o;
                    Color3 P_o;
                    float eta_o;
                    Color3 extinction_o;
                    if (bsdf->scatter(hit.normal, hit.texCoord, -ray.direction(), Color3::white(), w_o, P_o, eta_o, extinction_o, rnd)) {
                        radiance += rayTrace(Ray::fromOriginAndDirection(hit.position - w_o * 0.0001f, w_o), world, extinction_o, bounce + 1) * P_o / numSamples;
                    }
                }
            }
            break;
        }
    } else {
        // Hit the sky
        radiance = world->ambient;
    }

    return radiance;
}


void App::message(const std::string& msg) const {
    renderDevice->clear();
    renderDevice->push2D();
        debugFont->draw2D(renderDevice, msg, renderDevice->viewport().center(), 12, 
            Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    renderDevice->pop2D();

    // Force update so that we can see the message
    renderDevice->swapBuffers();
}


void App::onRender() {
    // Show message
    message("Rendering...");

    Stopwatch timer;
    Image3::Ref im = rayTraceImage(1.0f, m_raysPerPixel);
    timer.after("Trace");
    im->save("result.png");
}


Image3::Ref App::rayTraceImage(float scale, int numRays) {

    int width = window()->width() * scale;
    int height = window()->height() * scale;
    
    Image3::Ref im = Image3::createEmpty(width, height); 
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color3 sum = Color3::black();
            if (numRays == 1) {
                sum = rayTrace(defaultCamera.worldRay(x + 0.5f, y + 0.5f, im->rect2DBounds()), m_world);
            } else {
                for (int i = 0; i < numRays; ++i) {
                    sum += rayTrace(defaultCamera.worldRay(x + rnd.uniform(), y + rnd.uniform(), im->rect2DBounds()), m_world);
                }
            }
            im->set(x, y, sum / numRays);
        }
    }

    m_result = Texture::fromMemory("Result", im->getCArray(), ImageFormat::RGB32F(), im->width(), im->height(), 1, 
        GLCaps::supports(ImageFormat::RGB32F()) ? ImageFormat::RGB32F() : ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::video());

    return im;
}

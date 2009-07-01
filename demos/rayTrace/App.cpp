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

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_world(NULL) {}


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
    GuiWindow::Ref window = GuiWindow::create("Controls", debugWindow->theme());
    GuiPane* pane = window->pane();
    pane->addLabel("Use WASD keys + right mouse to move");
    pane->addButton("Render High Res.", this, &App::onRender);
    
    window->setVisible(true);
    addWidget(window);
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    if (! m_prevCFrame.fuzzyEq(defaultCamera.coordinateFrame())) {
        // Update the preview image only while moving
        rayTraceImage(0.18f);
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
static int totalRays = 0;

Color3 App::rayTrace(const Ray& ray, World* world, const Color3& extinction_i, int maxBounces) {
    Color3 radiance = Color3::zero();

    ++totalRays;

    Hit hit;
    float dist = inf();
    if (world->intersect(ray, dist, hit)) {
        const SuperBSDF::Ref& bsdf = hit.material->bsdf();

        // Shade this point (direct illumination)
        for (int L = 0; L < world->lightArray.size(); ++L) {
            const GLight& light = world->lightArray[L];

            // Shadow rays
            ++totalRays;
            if (world->lineOfSight(hit.position + hit.normal * 0.0001f, light.position.xyz())) {
                Vector3 w_L = light.position.xyz() - hit.position;
                float distance2 = w_L.squaredLength();
                w_L /= sqrt(distance2);

                // Attenduated radiance
                const Color3& radiance_L = light.color / distance2;

                radiance += bsdf->shadeDirect(hit.normal, hit.texCoord, w_L, 
                                              radiance_L, -ray.direction()).rgb();
            }
        }

        // Indirect illumination

        if (false) {
            // Distribution ray tracer
            if (maxBounces > 0) {
                static const int numSamples = 20;
                for (int i = 0; i < numSamples; ++i) {
                    Vector3 w_o;
                    Color3 P_o;
                    float eta_o;
                    Color3 extinction_o;
                    if (bsdf->scatter(hit.normal, hit.texCoord, -ray.direction(), Color3::white(), w_o, P_o, eta_o, extinction_o, rnd)) {
                        radiance += rayTrace(Ray::fromOriginAndDirection(hit.position - w_o * 0.0001f, w_o), world, extinction_o, maxBounces - 1) * P_o / numSamples;
                    }
                }
            }
        } else {
            // Whitted ray tracer:

            // Ambient
            radiance += bsdf->lambertian().sample(hit.texCoord).rgb() * world->ambient;

            if (maxBounces > 0) {
                // Perfect reflection and refraction
                Array<SuperBSDF::Impulse> impulseArray;
                bsdf->getImpulses(hit.normal, hit.texCoord, -ray.direction(), impulseArray);
                
                for (int i = 0; i < impulseArray.size(); ++i) {
                    const SuperBSDF::Impulse& impulse = impulseArray[i];
                    Ray secondaryRay = Ray::fromOriginAndDirection(hit.position, impulse.w).bump(0.0001f);
                    radiance += rayTrace(secondaryRay, world, impulse.extinction, maxBounces - 1) * impulse.coefficient;
                }
            }
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
    Image3::Ref im = rayTraceImage(1.0f);
	timer.after("Trace");
    im->save("result.png");
}


Image3::Ref App::rayTraceImage(float scale) {

    int width = window()->width() * scale;
    int height = window()->height() * scale;
    
    Image3::Ref im = Image3::createEmpty(width, height); 
    totalRays = 0;
    const int maxBounces = 3;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            im->set(x, y, rayTrace(defaultCamera.worldRay(x, y, im->rect2DBounds()), m_world, Color3::zero(), maxBounces));
        }
    }
    //debugPrintf("totalRays = %d\n", totalRays);

    m_result = Texture::fromMemory("Result", im->getCArray(), ImageFormat::RGB32F(), im->width(), im->height(), 1, 
                                   ImageFormat::RGB32F(), Texture::DIM_2D_NPOT, Texture::Settings::video());

    return im;
}

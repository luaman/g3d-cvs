/**
  @file App.cpp
 */
#include "App.h"
#include "World.h"
#include "Hit.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.width       = 512; 
    settings.window.height      = 256;

#   ifdef G3D_WIN32
        if (fileExists("data-files")) {
            chdir("data-files");
        }
#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_world(NULL) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
}


void App::onInit() {
    m_world = new World();

    showRenderingStats = false;
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    
    GCamera camera;
    camera.setPosition(Vector3(0,1,5));
    camera.lookAt(Vector3::zero());
    debugPrintf("Tracing...\n");
    
	Stopwatch timer;
    Image3::Ref im = raytrace(camera, m_world, window()->width(), window()->height());
	timer.after("Trace");
    im->save("result.png");
    
    m_result = Texture::fromMemory("Result", im->getCArray(), ImageFormat::RGB32F(), im->width(), im->height(), 1, 
                                   ImageFormat::RGB8(), Texture::DIM_2D, Texture::Settings::video());
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    rd->clear();

    rd->push2D();
    rd->setTexture(0, m_result);
    Draw::rect2D(m_result->rect2DBounds(), rd);
    rd->pop2D();

    PosedModel2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    delete m_world;
    m_world = NULL;
}


static G3D::Random rnd(0xF018A4D2, false);

static int totalRays = 0;

Color3 App::trace1(const Ray& ray, World* world, const Color3& extinction_i, int maxBounces) {
    Color3 radiance = Color3::zero();

    ++totalRays;

    static const Color3 ambientColor = Color3::white() * 0.2f;
    Hit hit;
    float dist = inf();
    if (world->intersect(ray, dist, hit)) {
        const UberBSDF::Ref& bsdf = hit.material->bsdf();


        // Shade this point
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

                radiance += bsdf->shadeDirect(hit.normal, hit.texCoord, w_L, radiance_L, -ray.direction()).rgb();
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
                        radiance += trace1(Ray::fromOriginAndDirection(hit.position - w_o * 0.0001f, w_o), world, extinction_o, maxBounces - 1) * P_o / numSamples;
                    }
                }
            }
        } else {
            // Whitted ray tracer:

            // Ambient
            radiance += bsdf->lambertian().sample(hit.texCoord).rgb() * ambientColor;

            if (maxBounces > 0) {
                Array<UberBSDF::Impulse> impulseArray;
                bsdf->getImpulses(hit.normal, hit.texCoord, -ray.direction(), impulseArray);
                
                for (int i = 0; i < impulseArray.size(); ++i) {
                    const UberBSDF::Impulse& impulse = impulseArray[i];
                    Ray secondaryRay = Ray::fromOriginAndDirection(hit.position, impulse.w);
					secondaryRay.bump(0.001f);
                    radiance += trace1(secondaryRay, world, impulse.extinction, maxBounces - 1) * impulse.coefficient;
                }
            }
        }
    }

    return radiance;
}


Image3::Ref App::raytrace(const GCamera& camera, World* world, int width, int height) {
    Image3::Ref im = Image3::createEmpty(width, height); 
    totalRays = 0;
    const int maxBounces = 2;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            im->set(x, y, trace1(camera.worldRay(x, y, im->rect2DBounds()), world, Color3::zero(), maxBounces));
        }
    }
    debugPrintf("totalRays = %d\n", totalRays);

    return im;
}

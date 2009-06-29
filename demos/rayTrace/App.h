/**
  @file App.h
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class World;

class App : public GApp {
private:
    World*              m_world;

    Texture::Ref        m_result;

public:

    /** Trace a single ray backwards */
    Color3 trace1(const Ray& ray, World* world, const Color3& extinction_i, int maxBounces);

    /** Trace a whole image */
    Image3::Ref raytrace(const GCamera& camera, World* world, int width, int height);

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);
    virtual void onCleanup();
};

#endif

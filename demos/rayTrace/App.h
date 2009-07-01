/**
  @file App.h

  This is a simple ray tracing demo showing how to use the G3D ray tracing 
  primitives.  It runs fast enough for real-time flythrough of 
  a 100k scene at low resolution. At a loss of simplicity, it could be made
  substantially faster using adaptive refinement and multithreading.
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

    /** Position during the previous frame */
    CFrame              m_prevCFrame;

    /** Called from onInit() */
    void makeGUI();

    /** Trace a single ray backwards. */
    Color3 rayTrace(const Ray& ray, World* world, const Color3& extinction_i, int maxBounces);

    /** Trace a whole image. */
    Image3::Ref rayTraceImage(float scale);

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();

    /** Callback for the render button */
    void onRender();

    virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);
    virtual void onCleanup();
};

#endif

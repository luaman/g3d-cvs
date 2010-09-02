/**
  @file App.h

  The G3D 8.0 default starter app is configured for OpenGL 3.0 and relatively recent
  GPUs.  To support older GPUs you may want to disable the framebuffer and film
  classes and use G3D::Sky to handle the skybox.
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class Body {
public:
    Vector2 position;
};


class App : public GApp {

    bool                enabled;
    Array<Body>         m_current;
    Array<Body>         m_previous;

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);
};

#endif

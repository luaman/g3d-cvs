/**
  @file App.h
  
  G3D Networking Demo
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class App : public GApp {
public:

    bool                isServer;

    void browseServers();

    App();

    virtual void onInit();

    virtual void onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);
};

#endif

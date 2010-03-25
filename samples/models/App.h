#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include "Entity.h"

class App : public GApp {
private:

    ShadowMap::Ref              shadowMap;
    Texture::Ref                logo;

    SkyRef                      sky;
    SkyParameters               skyParameters;
    LightingRef                 lighting;

    Array<Entity::Ref>          entityArray;
    Texture::Ref                texture;

    void loadScene();

public:

    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onUserInput(UserInput* ui);
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D);
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D);
};

#endif

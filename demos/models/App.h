#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>
#include "Entity.h"

class App : public GApp {
private:

    ShadowMap::Ref              shadowMap;
    Texture::Ref                logo;

    SkyRef                      sky;
    SkyParameters               skyParameters;
    LightingRef                 lighting;

    Array<EntityRef>            entityArray;
    Texture::Ref                texture;

    void loadScene();

public:

    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onUserInput(UserInput* ui);
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
};

#endif

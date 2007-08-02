#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>


typedef ReferenceCountedPointer<class Entity> EntityRef;
class Entity : public ReferenceCountedObject {
private:

    ArticulatedModelRef         artModel;

    ArticulatedModel::Pose      artPose;


    Entity() {}

public:

    /** Root frame */
    CoordinateFrame             cframe;

    static EntityRef create(
        ArticulatedModelRef model = NULL,
        const CoordinateFrame& c = CoordinateFrame()) {

        Entity* e = new Entity();

        e->artModel = model;
        e->cframe = c;
        return e;
    }

    void onSimulation(RealTime dt) {
    }
};


class App : public GApp {
private:

    ShadowMap                   shadowMap;
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

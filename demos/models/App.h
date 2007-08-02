#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>


typedef ReferenceCountedPointer<class Entity> EntityRef;
class Entity : public ReferenceCountedObject {
private:

    ArticulatedModelRef         artModel;
    ArticulatedModel::Pose      artPose;

    MD2ModelRef                 md2Model;
    MD2Model::Pose              md2Pose;
    GMaterial                   md2Material;
    
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

    static EntityRef create(
        MD2ModelRef model,
        TextureRef texture,
        const CoordinateFrame& c = CoordinateFrame()) {

        Entity* e = new Entity();

        e->md2Material.texture.append(texture);
        e->md2Model = model;
        e->cframe = c;
        return e;
    }

    void onPose(Array<PosedModelRef>& array) {
        if (artModel.notNull()) {
            artModel->pose(array, cframe, artPose);
        }

        if (md2Model.notNull()) {
            array.append(md2Model->pose(cframe, md2Pose, md2Material));
        }
    }

    void onSimulation(RealTime dt) {
        if (md2Model.notNull()) {
            MD2Model::Pose::Action action;
            action.point  = uniformRandom() > 0.9995;
            action.salute = uniformRandom() > 0.9995;
            action.wave   = uniformRandom() > 0.9995;
            md2Pose.onSimulation(dt, action);
        }
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

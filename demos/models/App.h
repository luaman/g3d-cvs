#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>

typedef ReferenceCountedPointer<class Entity> EntityRef;
class Entity : public ReferenceCountedObject {
private:

    Entity() {}

public:

    ArticulatedModelRef         model;

    ArticulatedModel::Pose      pose;

    /** Root frame */
    CoordinateFrame             cframe;

    static EntityRef create(
        ArticulatedModelRef model = NULL,
        const CoordinateFrame& c = CoordinateFrame()) {

        Entity* e = new Entity();

        e->model = model;
        e->cframe = c;
        return e;
    }
};



class App : public GApp2 {
private:

    void generateShadowMap(const GLight& light, const Array<PosedModel::Ref>& shadowCaster);

    Texture::Ref                  shadowMap;
    Matrix4                     lightMVP;

    Texture::Ref                  logo;

    SkyRef                      sky;

    SkyParameters               skyParameters;
    LightingRef                 lighting;

    Array<EntityRef>            entityArray;
    Texture::Ref                  texture;

    void loadScene();

public:

    App(const GApp2::Settings& settings = GApp2::Settings());

    virtual void onUserInput(UserInput* ui);

    virtual void onGraphics(RenderDevice* rd);

};

#endif

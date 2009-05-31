#ifndef Entity_h
#define Entity_h

#include <G3D/G3DAll.h>

class Entity : public ReferenceCountedObject {
public:

typedef ReferenceCountedPointer<class Entity> Ref;

private:

    ArticulatedModel::Ref       artModel;
    ArticulatedModel::Pose      artPose;

    MD2Model::Ref               md2Model;
    MD2Model::Pose              md2Pose;
    GMaterial                   md2Material;

    IFSModel::Ref               ifsModel;
    GMaterial                   ifsMaterial;
    bool                        ifsVertexNormals;
    
    Entity() {}

public:

    /** Root frame */
    CFrame             cframe;

    static Entity::Ref create(
        ArticulatedModel::Ref model = NULL,
        const CFrame& c = CFrame()) {

        Entity* e = new Entity();

        e->artModel = model;
        e->cframe = c;
        return e;
    }
    
    static Entity::Ref create
    (
     const MD2Model::Ref& model,
     const GMaterial& material = GMaterial(),
     const CFrame& c = CFrame()) {

        Entity* e = new Entity();

        e->md2Model = model;
        e->md2Material = material;
        e->cframe = c;
        return e;
    }

    static Entity::Ref create(
        IFSModelRef model,
        const GMaterial& material = GMaterial(),
        const CFrame& c = CFrame(),
        bool vertexNormals = true) {

        Entity* e = new Entity();

        e->ifsMaterial = material;
        e->ifsModel = model;
        e->ifsVertexNormals = vertexNormals;
        e->cframe = c;
        return e;
    }

    void onPose(Array<SurfaceRef>& array) {
        if (artModel.notNull()) {
            artModel->pose(array, cframe, artPose);
        }

        if (md2Model.notNull()) {
            array.append(md2Model->pose(cframe, md2Pose, md2Material));
        }

        if (ifsModel.notNull()) {
            array.append(ifsModel->pose(cframe, ifsMaterial, ifsVertexNormals));
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

#endif

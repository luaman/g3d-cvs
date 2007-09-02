#ifndef ENTITY_H
#define ENTITY_H

#include <G3D/G3DAll.h>

typedef ReferenceCountedPointer<class Entity> EntityRef;
class Entity : public ReferenceCountedObject {
private:

    ArticulatedModelRef         artModel;
    ArticulatedModel::Pose      artPose;

    MD2ModelRef                 md2Model;
    MD2Model::Pose              md2Pose;
    GMaterial                   md2Material;

    IFSModelRef                 ifsModel;
    GMaterial                   ifsMaterial;
    bool                        ifsVertexNormals;
    
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
        const GMaterial& material = GMaterial(),
        const CoordinateFrame& c = CoordinateFrame()) {

        Entity* e = new Entity();

        e->md2Model = model;
        e->md2Material = material;
        e->cframe = c;
        return e;
    }

    static EntityRef create(
        IFSModelRef model,
        const GMaterial& material = GMaterial(),
        const CoordinateFrame& c = CoordinateFrame(),
        bool vertexNormals = true) {

        Entity* e = new Entity();

        e->ifsMaterial = material;
        e->ifsModel = model;
        e->ifsVertexNormals = vertexNormals;
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

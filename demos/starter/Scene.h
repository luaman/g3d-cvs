#ifndef Scene_h
#define Scene_h

#include <G3D/G3DAll.h>

/** Sample object */
class Entity : public ReferenceCountedObject {
private:

    Entity();

public:

    typedef ReferenceCountedPointer<Entity> Ref;

    CFrame                  frame;
    ArticulatedModel::Ref   model;

    static Entity::Ref create(const CFrame& c, const ArticulatedModel::Ref& m);

    virtual void onPose(Array<Surface::Ref>& surfaceArray);
};


/** Sample scene graph */
class Scene : public ReferenceCountedObject {
protected:

    Lighting::Ref               m_lighting;
    Array<Entity::Ref>          m_entityArray;

public:

    typedef ReferenceCountedPointer<Scene> Ref;

    static Scene::Ref create();
    
    void onPose(Array<Surface::Ref>& surfaceArray);

    inline Lighting::Ref lighting() const {
        return m_lighting;
    }
};

#endif

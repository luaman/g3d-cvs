#ifndef Scene_h
#define Scene_h

#include <G3D/G3DAll.h>

/** Sample object. 

    G3D does not contain an Entity class in the API because it is a
    very application-specific role. This is a sample of how you 
    might begin to structure one to get you started.
*/
class Entity : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Entity> Ref;

protected:

    std::string             m_name;
    CFrame                  m_frame;
    ArticulatedModel::Ref   m_model;
    ArticulatedModel::Pose  m_pose;

    Entity();

public:

    const CFrame& frame() const {
        return m_frame;
    }

    const std::string& name() const {
        return m_name;
    }

    static Entity::Ref create(const std::string& n, const CFrame& c, const ArticulatedModel::Ref& m);

    virtual void onPose(Array<Surface::Ref>& surfaceArray);
};


/** Sample scene graph.

    Includes loading from a text file, and a GUI component for detecting
    and selecting scenes.

    G3D does not contain a Scene class in the API because it is a
    very application-specific role. This is a sample of how you 
    might begin to structure one to get you started.
*/
class Scene : public ReferenceCountedObject {
protected:
    Lighting::Ref               m_lighting;
    Texture::Ref                m_skyBox;
    Array<Entity::Ref>          m_entityArray;
	
	Scene() {}

public:

    typedef ReferenceCountedPointer<Scene> Ref;

    static Scene::Ref create(const std::string& sceneName, GCamera& camera);
    
    void onPose(Array<Surface::Ref>& surfaceArray);

    inline Lighting::Ref lighting() const {
        return m_lighting;
    }

    inline Texture::Ref skyBox() const {
        return m_skyBox;
    }

    /** Enumerate the names of all available scenes. */
    static Array<std::string> sceneNames();
};

#endif
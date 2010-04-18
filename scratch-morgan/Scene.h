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

    enum ModelType {
        ARTICULATED_MODEL,
        MD2_MODEL
    };

    ModelType                       m_modelType;

    std::string                     m_name;

    MD2Model                        m_md2Model;
    MD2Model::Pose                  m_md2Pose;

    ArticulatedModel::Ref           m_model;

    /** Current pose */
    ArticulatedModel::Pose          m_pose;

    /** Pose over time. */
    ArticulatedModel::PoseSpline    m_poseSpline;

    /** Current position */
    CFrame                          m_frame;

    /** Root position over time */
    PhysicsFrameSpline              m_frameSpline;

    Entity();

public:

    const CFrame& frame() const {
        return m_frame;
    }

    const std::string& name() const {
        return m_name;
    }

    static Entity::Ref create(const std::string& n, const ArticulatedModel::Ref& m, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::PoseSpline& poseSpline);
    static Entity::Ref create(const std::string& n, const MD2Model::Ref& m, const PhysicsFrameSpline& frameSpline);

    virtual void onSimulation(GameTime absoluteTime, GameTime deltaTime);

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
    /** Current time */
    RealTime                    m_time;
    Lighting::Ref               m_lighting;
    Texture::Ref                m_skyBox;
    Array<Entity::Ref>          m_entityArray;
	
    Scene() : m_time(0) {}

public:

    typedef ReferenceCountedPointer<Scene> Ref;

    static Scene::Ref create(const std::string& sceneName, GCamera& camera);
    
    virtual void onPose(Array<Surface::Ref>& surfaceArray);

    virtual void onSimulation(GameTime deltaTime);

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

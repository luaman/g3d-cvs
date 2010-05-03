#include "Scene.h"

using namespace G3D::units;

Entity::Entity() {}

Entity::Ref Entity::create(const std::string& n, const ArticulatedModel::Ref& m, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::PoseSpline& poseSpline) {
    Ref e = new Entity();
    e->m_name  = n;
    e->m_frameSpline = frameSpline;

    e->m_artModel = m;
    e->m_artPoseSpline = poseSpline;
    e->m_modelType = ARTICULATED_MODEL;


    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


Entity::Ref Entity::create(const std::string& n, const MD2Model::Ref& m, const PhysicsFrameSpline& frameSpline) {
    Ref e = new Entity();
    e->m_modelType = MD2_MODEL;
    e->m_name  = n;
    e->m_md2Model = m;
    e->m_frameSpline = frameSpline;

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


Entity::Ref Entity::create(const std::string& n, const MD3Model::Ref& m, const PhysicsFrameSpline& frameSpline) {
    Ref e = new Entity();
    e->m_modelType = MD3_MODEL;
    e->m_name  = n;
    e->m_md3Model = m;
    e->m_frameSpline = frameSpline;

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


void Entity::onSimulation(GameTime absoluteTime, GameTime deltaTime) {
    (void)deltaTime;
    m_frame = m_frameSpline.evaluate(float(absoluteTime));

    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artPoseSpline.get(float(absoluteTime), m_artPose);
        break;

    case MD2_MODEL:
        {
            MD2Model::Pose::Action a;
            m_md2Pose.onSimulation(deltaTime, a);
            break;
        }

    case MD3_MODEL:
        m_md3Model->simulatePose(m_md3Pose, deltaTime);
        break;
    }
}


void Entity::onPose(Array<Surface::Ref>& surfaceArray) {
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artModel->pose(surfaceArray, m_frame, m_artPose);
        break;

    case MD2_MODEL:
        m_md2Model->pose(surfaceArray, m_frame, m_md2Pose);
        break;

    case MD3_MODEL:
        m_md3Model->pose(surfaceArray, m_frame, m_md3Pose);
        break;
    }
}

////////////////////////////////////////////////////////////////////////

void Scene::onSimulation(RealTime deltaTime) {
    m_time += deltaTime;
    for (int i = 0; i < m_entityArray.size(); ++i) {
        m_entityArray[i]->onSimulation(m_time, deltaTime);
    }
}


/** Returns a table mapping scene names to filenames */
static Table<std::string, std::string>& filenameTable() {
    static Table<std::string, std::string> filenameTable;

    if (filenameTable.size() == 0) {
        // Create a table mapping scene names to filenames
        Array<std::string> filenameArray;
		FileSystem::getFiles("*.scn.any", filenameArray, true);
        for (int i = 0; i < filenameArray.size(); ++i) {
            Any a;
            a.load(filenameArray[i]);

            std::string name = a["name"].string();
            alwaysAssertM(! filenameTable.containsKey(name),
                "Duplicate scene names in " + filenameArray[i] + " and " +
                filenameTable["name"]);
                
            filenameTable.set(name, filenameArray[i]);
        }
    }

    return filenameTable;
}


Array<std::string> Scene::sceneNames() {
    return filenameTable().getKeys();
}


Scene::Ref Scene::create(const std::string& scene, GCamera& camera) {

    Scene::Ref s = new Scene();

    const std::string* f = filenameTable().getPointer(scene);
    if (f == NULL) {
        throw "No scene with name '" + scene + "' found in (" + 
            stringJoin(filenameTable().getKeys(), ", ") + ")";
    }
    const std::string& filename = *f;

    Any any;
    any.load(filename);

    // Load the lighting
    if (any.containsKey("lighting")) {
        s->m_lighting = Lighting::create(any["lighting"]);
    } else {
        s->m_lighting = Lighting::create();
    }

    // Load the models
    Any models = any["models"];
    typedef ReferenceCountedPointer<ReferenceCountedObject> ModelRef;
    Table< std::string, ModelRef > modelTable;
    for (Any::AnyTable::Iterator it = models.table().begin(); it.hasMore(); ++it) {
        ModelRef m;
        Any v = it->value;
        if (v.nameBeginsWith("ArticulatedModel")) {
            m = ArticulatedModel::create(v);
        } else if (v.nameBeginsWith("MD2Model")) {
            m = MD2Model::create(v);
        } else if (v.nameBeginsWith("MD3Model")) {
            m = MD3Model::create(v);
        } else {
            debugAssertM(false, "Unrecognized model type: " + v.name());
        }

        modelTable.set(it->key, m);        
    }

    // Instance the models
    Any entities = any["entities"];
    for (Table<std::string, Any>::Iterator it = entities.table().begin(); it.hasMore(); ++it) {
        const std::string& name = it->key;
        const Any& modelArgs = it->value;

        modelArgs.verifyType(Any::ARRAY);
        const ModelRef* model = modelTable.getPointer(modelArgs.name());
        modelArgs.verify((model != NULL), 
            "Can't instantiate undefined model named " + modelArgs.name() + ".");

        PhysicsFrameSpline frameSpline;
        ArticulatedModel::PoseSpline poseSpline;
        if (modelArgs.size() >= 1) {
            frameSpline = modelArgs[0];
            if (modelArgs.size() >= 2) {
                // Poses 
                poseSpline = modelArgs[1];
            }
        }

        ArticulatedModel::Ref artModel = model->downcast<ArticulatedModel>();
        MD2Model::Ref         md2Model = model->downcast<MD2Model>();
        MD3Model::Ref         md3Model = model->downcast<MD3Model>();

        if (artModel.notNull()) {
            s->m_entityArray.append(Entity::create(name, artModel, frameSpline, poseSpline));
        } else if (md2Model.notNull()) {
            s->m_entityArray.append(Entity::create(name, md2Model, frameSpline));
        } else if (md3Model.notNull()) {
            s->m_entityArray.append(Entity::create(name, md3Model, frameSpline));
        }
    }

    // Load the camera
    camera = any["camera"];

    if (any.containsKey("skybox")) {
        s->m_skyBox = Texture::create(any["skybox"]);
    } else {
        s->m_skyBox = s->m_lighting->environmentMap;
    }
    
    return s;
}


void Scene::onPose(Array<Surface::Ref>& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}

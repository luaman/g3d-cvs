#include "Scene.h"

using namespace G3D::units;

Entity::Entity() {}

Entity::Ref Entity::create(const CFrame& c, const ArticulatedModel::Ref& m) {
    Ref e = new Entity();
    e->frame = c;
    e->model = m;
    return e;
}


void Entity::onPose(Array<Surface::Ref>& surfaceArray) {
    model->pose(surfaceArray, frame);
}


/** Returns a table mapping scene names to filenames */
static Table<std::string, std::string>& filenameTable() {
    static Table<std::string, std::string> filenameTable;

    if (filenameTable.size() == 0) {
        // Create a table mapping scene names to filenames
        Array<std::string> filenameArray;
        getFiles("scene/*.txt", filenameArray);
        for (int i = 0; i < filenameArray.size(); ++i) {
            Any a;
            a.load(pathConcat("scene", filenameArray[i]));

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
    any.load(pathConcat("scene", filename));

    // Load the lighting
    if (any.containsKey("lighting")) {
        s->m_lighting = Lighting::create(any["lighting"]);
    } else {
        s->m_lighting = Lighting::create();
    }

    // Load the models
    Any models = any["models"];
    Table<std::string, ArticulatedModel::Ref> modelTable;
    for (Any::AnyTable::Iterator it = models.table().begin(); it.hasMore(); ++it) {
        modelTable.set(it->key, ArticulatedModel::create(it->value));        
    }

    // Instance the models
    Any entities = any["entities"];
    entities.verifyType(Any::ARRAY);
    for (int e = 0; e < entities.size(); ++e) {
        // Entities look like functions whose name is the model and whose arguments
        // are the CFrames
        const Any& entityArgs = entities[e];
        entityArgs.verifyType(Any::ARRAY);
        const ArticulatedModel::Ref* model = modelTable.getPointer(entityArgs.name());
        entityArgs.verify((model != NULL), 
            "Can't instantiate undefined model named " + entityArgs.name() + ".");

        CFrame cframe;
        if (entityArgs.size() == 1) {
            cframe = entityArgs[0];
        }            

        s->m_entityArray.append(Entity::create(cframe, *model));
    }

    // Load the camera
    camera = any["camera"];
    
    return s;
}


void Scene::onPose(Array<Surface::Ref>& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}

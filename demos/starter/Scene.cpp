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


Scene::Ref Scene::create() {
    Scene::Ref s = new Scene();
    s->m_lighting = GApp::defaultLighting();

    std::string materialPath = System::findDataFile("material");
    std::string crateFile = System::findDataFile("crate.ifs");

    // Ground plane
    {
        ArticulatedModel::Ref model = ArticulatedModel::fromFile(crateFile, Vector3(6.0f, 1.0f, 6.0f));

        Material::Settings mat;
        std::string base = pathConcat(materialPath, "asphalt/asphalt-");
        mat.setLambertian(base + "L.jpg");
        mat.setSpecular(base + "G.jpg");
        mat.setGlossyExponentShininess(20);

        BumpMap::Settings b;
        b.iterations = 0;
        mat.setBump(base + "L.jpg", b, -0.005f);
        
        model->partArray[0].triList[0]->material = Material::create(mat);

        s->m_entityArray.append(Entity::create(Vector3::unitY() * -0.5f * meters(), model));
    }
 
    // Crates
    {
        ArticulatedModel::Ref model = ArticulatedModel::fromFile(crateFile);

        Material::Settings mat;
        std::string base = pathConcat(materialPath, "metalcrate/metalcrate-");
        mat.setLambertian(base + "L.png", 0.2f);
        mat.setSpecular(base + "G.png");
        mat.setGlossyExponentShininess(20);
        BumpMap::Settings b;
        b.iterations = 1;
        mat.setBump(base + "B.png", b);
        model->partArray[0].triList[0]->material = Material::create(mat);

        s->m_entityArray.append(Entity::create(Vector3(1.0f, 0.5f, 0.0f) * meters(), model));
        s->m_entityArray.append(Entity::create(CFrame::fromXYZYPRDegrees(0.6f, 0.5f, -1.8f, 30.0f), model));
    }
    
    return s;
}
    

void Scene::onPose(Array<Surface::Ref>& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}

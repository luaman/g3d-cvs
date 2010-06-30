#include "World.h"
#include "Hit.h"

World::World() : m_mode(TRACE) {
    begin();

    lightArray.append(GLight::point(Vector3(0, 10, 0), Color3::white() * 100));
    lightArray.append(GLight::point(Vector3(22.6f, 2.9f,  6.6f), Color3::fromARGB(0xffe5bd) * 90));

    ambient = Color3::fromARGB(0x304855) * 0.9f;

    {
        ArticulatedModel::Ref teapot = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"), 0.7f);
        Material::Settings mirror;

        mirror.setSpecular(Color3::white() * 0.2f);
        mirror.setMirrorShininess();
        mirror.setLambertian(Color3::fromARGB(0xdd4034) * 0.6f);

        teapot->partArray[0].triList[0]->material = Material::create(mirror);
        insert(teapot, CFrame::fromXYZYPRDegrees(19.4f, 0.22f, 0.94f, 70));
    }

    {
        ArticulatedModel::Ref sphere = ArticulatedModel::fromFile(System::findDataFile("sphere.ifs"), 0.3f);
        // Use the outside of the object as the interface into glass from air
        Material::Specification glassAir;
        glassAir.setSpecular(Color3::white() * 0.2f);
        glassAir.setMirrorShininess();
        glassAir.setLambertian(Color3::black());
        glassAir.setEta(1.3f, 1.0f);
        glassAir.setTransmissive(Color3::fromARGB(0xb1ee9c) * 0.8f);
        sphere->partArray[0].triList[0]->material = Material::create(glassAir);

        // Create the interface into air from glass
        Material::Specification airGlass;
        airGlass.removeSpecular();
        airGlass.setLambertian(Color3::black());
        airGlass.setEta(1.0f, 1.3f);
        airGlass.setTransmissive(Color3::white());

        ArticulatedModel::Part::TriList::Ref inside = sphere->partArray[0].newTriList(Material::create(airGlass));
        inside->indexArray = sphere->partArray[0].triList[0]->indexArray;
        inside->indexArray.reverse();
        sphere->updateAll();

        insert(sphere, CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));
    }
    std::string filename = pathConcat(System::findDataFile("3ds"), "fantasy/sponza/sponza.3DS");
    insert(ArticulatedModel::fromFile(filename), Vector3(8.2f, -6, 0));
    
    end();
}


void World::begin() {
    debugAssert(m_mode == TRACE);
    m_surfaceArray.clear();
    m_triArray.clear();
    m_mode = INSERT;
}


void World::insert(const ArticulatedModel::Ref& model, const CFrame& frame) {
    Array<Surface::Ref> posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}

void World::insert(const Surface::Ref& m) {
    debugAssert(m_mode == INSERT);
    m_surfaceArray.append(m);
    Tri::getTris(m, m_triArray, CFrame());
}


void World::end() {
    for (int i = 0; i < m_triArray.size(); ++i) {
        m_triArray[i].material()->setStorage(MOVE_TO_CPU);
    }

    debugAssert(m_mode == INSERT);
    m_mode = TRACE;

    TriTree::Settings s;
    s.algorithm = TriTree::MEAN_EXTENT;
    m_triTree.setContents(m_triArray, s);
    m_triArray.clear();
}


bool World::lineOfSight(const Vector3& v0, const Vector3& v1) const {
    debugAssert(m_mode == TRACE);
    
    Vector3 d = v1 - v0;
    float len = d.length();
    Ray ray = Ray::fromOriginAndDirection(v0, d / len);
    float distance = len;
    Tri::Intersector intersector;

    return ! m_triTree.intersectRay(ray, intersector, distance);

}


bool World::intersect(const Ray& ray, float& distance, Hit& hit) const {
    debugAssert(m_mode == TRACE);

    Tri::Intersector intersector;

    m_triTree.intersectRay(ray, intersector, distance);

    // Resolve the intersection data into an actual Hit
    hit.setFromIntersector(intersector);
    return hit.exists();
}

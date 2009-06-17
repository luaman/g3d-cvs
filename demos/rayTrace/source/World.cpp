#include "World.h"
#include "Hit.h"

World::World() : m_mode(TRACE) {
    begin();

    lightArray.append(GLight::point(Vector3(0, 10, 0), Color3::white() * 100));

    {
        ArticulatedModel::Ref teapot = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"), 2);
        Material::Settings mirror;

        mirror.setSpecular(Color3::white() * 0.5f);
        mirror.setMirrorShininess();
        mirror.setLambertian(Color3::red() * 0.5f);

        teapot->partArray[0].triList[0]->material = Material::create(mirror);
        insert(teapot, Vector3(0, 0, 0));
    }

    insert(ArticulatedModel::fromFile(System::findDataFile("fantasy/sponza/sponza.3DS")), Vector3(8.2f, -6, 0));
    
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

    m_triTree.setContents(m_triArray);
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

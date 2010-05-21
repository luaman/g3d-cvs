#include "GLG3D/GEntity.h"

namespace G3D {

GEntity::GEntity() {}


GEntity::GEntity
(const std::string& n, const PhysicsFrameSpline& frameSpline, 
const ArticulatedModel::Ref& artModel, const ArticulatedModel::PoseSpline& artPoseSpline,
const MD2Model::Ref& md2Model,
const MD3Model::Ref& md3Model) : 
    m_name(n), 
    m_modelType(ARTICULATED_MODEL),
    m_frameSpline(frameSpline),
    m_artPoseSpline(artPoseSpline), 
    m_artModel(artModel),
    m_md2Model(md2Model), m_md3Model(md3Model) {

    m_name  = n;
    m_frameSpline = frameSpline;

    if (artModel.notNull()) {
        m_modelType = ARTICULATED_MODEL;
    } else if (md2Model.notNull()) {
        m_modelType = MD2_MODEL;
    } else if (md3Model.notNull()) {
        m_modelType = MD3_MODEL;
    }
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::Ref& m, const ArticulatedModel::PoseSpline& poseSpline) {
    GEntity::Ref e = new GEntity(n, frameSpline, m, poseSpline, NULL, NULL);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD2Model::Ref& m) {
    GEntity::Ref e = new GEntity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), m, NULL);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD3Model::Ref& m) {
    GEntity::Ref e = new GEntity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), NULL, m);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


void GEntity::onSimulation(GameTime absoluteTime, GameTime deltaTime) {
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


void GEntity::onPose(Array<Surface::Ref>& surfaceArray) {
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

}

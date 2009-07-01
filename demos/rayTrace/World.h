/**
  @file World.h
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#ifndef World_h
#define World_h

#include <G3D/G3DAll.h>

/** \brief The scene.*/
class World {
private:

    Array<Tri>               m_triArray;
    Array<Surface::Ref>      m_surfaceArray;
    TriTree                  m_triTree;

    enum Mode {TRACE, INSERT} m_mode;

public:

    Array<GLight>           lightArray;
    Color3                  ambient;

    World();

    /** Returns true if there is an unoccluded line of sight from v0
        to v1.  This is sometimes called the visibilty function in the
        literature.*/
    bool lineOfSight(const Vector3& v0, const Vector3& v1) const;

    void begin();
    void insert(const ArticulatedModel::Ref& model, const CFrame& frame = CFrame());
    void insert(const Surface::Ref& m);
    void end();

    /**\brief Trace the ray into the scene and return the first
       surface hit.

       \param ray In world space 

       \param distance On input, the maximum distance to trace to.  On
       output, the distance to the closest surface.

       \param hit Will be initialized by the routine.

       \return True if anything was hit
     */
    bool intersect(const Ray& ray, float& distance, class Hit& hit) const;
};

#endif

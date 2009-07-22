#ifndef Hit_h
#define Hit_h

#include <G3D/G3DAll.h>

/**
 Result of casting a ray at a set of surfaces.
 */ 
class Hit {
public:

    /** World-space position of the ray hit. */
    Vector3         position;

    /** World-space normal to the surface at the hit location. 
        May be unspecified, depending on how this ray was cast. */
    Vector3         normal;

    /** Texture coordinate.  */
    Vector2         texCoord;

    Material::Ref   material;

    /** Sets this to an infinite position */
    Hit();

    /** True if there was a hit */
    inline bool exists() const {
        return position.isFinite();
    }

    void setFromIntersector(const Tri::Intersector& intersector);
};

#endif


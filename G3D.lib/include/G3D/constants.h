/**
  @file G3D/constants.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2009-05-20
  @edited  2009-05-20
*/
#ifndef G3D_constants_h
#define G3D_constants_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {

/** These are defined to have the same value as the equivalent OpenGL
    constant. */
class PrimitiveType {
public:
    enum Value {
        POINTS         = 0x0000,
        LINES          = 0x0001,
        LINE_STRIP     = 0x0003, 
        TRIANGLES      = 0x0004, 
        TRIANGLE_STRIP = 0x0005,
        TRIANGLE_FAN   = 0x0006,
        QUADS          = 0x0007, 
        QUAD_STRIP     = 0x0008
    };

private:

    Value value;

public:

    G3D_DECLARE_ENUM_CLASS_METHODS(PrimitiveType);
};


/** Values for SuperSurface::GPUGeom::refractionHint. */
class RefractionQuality {
public:
    enum Value {
        /** No refraction; a translucent object will appear as if it had the same index of refraction
            as the surrounding medium and objects will be undistorted in the background. */
        NONE = 0, 

        /** Use a static environment map (cube or paraboloid) for computing transmissivity.*/
        STATIC_ENV = 25, 

        /** Use a dynamically rendered 2D environment map; distort the background.  This looks good for many scenes
            but avoids the cost of rendering a cube map for REFRACT_DYNAMIC_ENV. */
        DYNAMIC_FLAT = 50, 

        /** Render a dynamic environment map */
        DYNAMIC_ENV = 75, 

        /** Use the best method available, ideally true ray tracing. */
        BEST = 100
    };
private:

    Value value;

public:
    G3D_DECLARE_ENUM_CLASS_METHODS(RefractionQuality);
};


/** Values for SuperSurface::GPUGeom::mirrorHint. */
class MirrorQuality {
public:
    enum Value {
        /** Reflections are black */
        NONE = 0, 
        
        /** Use a static environment map.  This is what most games use */
        STATIC_ENV = 25, 
        
        /** Planar reflection, typically for water or glass windows.  */
        DYNAMIC_PLANAR = 50,

        /** Render a dynamic environment map. */
        DYNAMIC_ENV = 75, 
        
        /** Use the best method available, ideally true ray tracing. */
        BEST = 100
    };

private:
    Value value;

public:
    G3D_DECLARE_ENUM_CLASS_METHODS(MirrorQuality);
};

} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::PrimitiveType)
G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::RefractionQuality)
G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::MirrorQuality)

#endif


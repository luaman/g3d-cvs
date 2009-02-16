#ifndef G3D_IndexedPrimitive_h
#define G3D_IndexedPrimitive_h

#include "G3D/platform.h"
#include "G3D/MeshAlg.h"
#include "GLG3D/VAR.h"
#include "GLG3D/Material.h"

namespace G3D {

/** @brief A GPU mesh utility class that works with G3D::Material.
        
    A set of lines, points, quads, or triangles that have a
    single Material and can be rendered as a single OpenGL
    primitive using RenderDevice::sendIndices inside a
    RenderDevice::beginIndexedPrimitives() block.
    
    @sa G3D::MeshAlg, G3D::ArticulatedModel, G3D::PosedModel
*/
class IndexedPrimitive {
public:
    MeshAlg::Primitive      primitive;
    
    /** Indices into the VARs. */
    VAR                     indexVAR;
    
    VAR                     vertexVAR;
    VAR                     normalVAR;
    VAR                     tangetVAR;
    VAR                     texCoord0VAR;
    
    /** When true, this primitive should be rendered with
        two-sided lighting and texturing and not cull
        back faces. */
    bool                    twoSided;
    
    Material::Ref           material;
    
    IndexedPrimitive() : twoSided(false), primitive(MeshAlg::TRIANGLES) {}
};

} // G3D

#endif

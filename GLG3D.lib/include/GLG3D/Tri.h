/**
  @file GLG3D/Tri.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2008-08-10
  @edited  2009-06-20
*/
#ifndef GLG3D_Tri_h
#define GLG3D_Tri_h

#include "G3D/platform.h"
#include "G3D/HashTrait.h"
#include "G3D/BoundsTrait.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/AABox.h"
#include "G3D/Array.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Material.h"


namespace G3D {
class Ray;

/**
 @brief Triangle implementation optimized for ray-triangle intersection.  

 Single sided and immutable once created.
 
 The actual vertex positions have some roundoff error compared to a naive
 implementation because they are stored in a format more efficient for 
 intersection computations.

 Implementation is 132 bytes, including the vtable pointer.

 \sa G3D::Triangle, G3D::MeshShape, G3D::ArticulatedModel, G3D::Surface, G3D::MeshAlg
 */
class Tri {
private:
    // Intersector is declared below
    friend class Intersector;

    // The size of the Tri class does not appear to significantly impact
    // the performance of ray tracing under the current kd tree implementation.

    /** Vertex 0 */
    Vector3                 v0;

    /** Edge vector v1 - v0 */
    Vector3                 e1;

    /** Edge vector v2 - v0 */
    Vector3                 e2;

    /** True unit face normal: (e1 x e2).direction() */
    Vector3                 n;

    /** If the Tri is in a "smooth" surface then the vertex normals,
        otherwise the face normal. */
    Vector3                 m_normal[3];

    /** Texture coordinates. */
    Vector2                 m_texCoord[3];

    /** Per-vertex tangents for bump mapping. */
    Vector4                 m_packedTangent[3];

    Material::Ref           m_material;

    void*                   m_data;

public:

    Tri();

    virtual ~Tri();

    /** Assumes that normals are perpendicular to tangents, or that the tangents are zero */
    Tri(const Vector3& v0, const Vector3& v1, const Vector3& v2, 
        const Vector3& n0, const Vector3& n1, const Vector3& n2, 
        void* data = NULL,
        const Material::Ref& material = NULL,
        const Vector2& t0 = Vector2::zero(), const Vector2& t1 = Vector2::zero(), const Vector2& t2 = Vector2::zero(),
        const Vector4& tan0 = Vector4::zero(), const Vector4& tan1 = Vector4::zero(), const Vector4& tan2 = Vector4::zero());

    /** Backfacing version of this triangle.  Normals and tangents are
        negated and the winding order is reversed. */
    Tri otherSide() const;
    
    /** Returns a bounding box */
    inline void getBounds(AABox& box) const {
        const Vector3& v1 = v0 + e1;
        const Vector3& v2 = v0 + e2;

        box = AABox(v0.min(v1).min(v2), v0.max(v1).max(v2));
    }

    /** Surface area, computed each call. */
    float area() const;

    /** Vertex position (must be computed) */
    inline Vector3 vertex(int i) const {
        switch (i) {
        case 0:
            return v0;
        case 1:
            return v0 + e1;
        case 2:
            return v0 + e2;
        default:
            debugAssertM(false, "Illegal index");
            return v0;
        }
    }

    /** Face normal.  For degenerate triangles, this is zero.  For all other triangles
    it has unit length and is defined by counter-clockwise winding. */
    inline const Vector3& normal() const {
        return n;
    }

    /** Vertex normal */
    inline const Vector3& normal(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return m_normal[i];
    }

    inline const Vector2& texCoord(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return m_texCoord[i];
    }

    inline const Vector4& packedTangent(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return m_packedTangent[i];
    }

    /** Per-vertex unit tangent, for bump mapping. Tangents are perpendicular to 
        the corresponding vertex normals.*/
    inline Vector3 tangent(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return m_packedTangent[i].xyz();
    }

    /** Per-vertex unit tangent = normal x tangent, for bump mapping.
        (Erroneously called the "binormal" in some literature) */
    inline Vector3 tangent2(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return m_normal[i].cross(m_packedTangent[i].xyz()) * m_packedTangent[i].w;
    }

    /** Application-specific data. Can be used as a convenience 
        hook instead of subclassing Tri.*/
    inline void* data() const {
        return m_data;
    }

    /** Application-specific data; BSDF, image, etc. Can be used as a convenience 
        hook instead of subclassing Tri.*/
    inline Material::Ref material() const {
        return m_material;
    }

    /** Returns a (relatively) unique integer for this object */
    inline uint32 hashCode() const {
        return (v0.hashCode() << 20) + (e1.hashCode() << 10) + e2.hashCode();
    }

    inline bool operator==(const Tri& t) const {
        return 
            (v0 == t.v0) &&
            (e1 == t.e1) &&
            (e2 == t.e2) &&
            (m_normal[0] == t.m_normal[0]) &&
            (m_normal[1] == t.m_normal[1]) &&
            (m_normal[2] == t.m_normal[2]) &&
            (m_texCoord[0] == t.m_texCoord[0]) &&
            (m_texCoord[1] == t.m_texCoord[1]) &&
            (m_texCoord[2] == t.m_texCoord[2]) &&
            (m_data == t.m_data) &&
            (m_material == t.m_material);
    }

    /** @brief Performs intersection testing against Tri.  For use
     with a KDTree.  Avoids computing the interpolated parameters from
     barycentric coords until all intersection computations have
     completed.*/
    class Intersector {
    public:

        /** The triangle hit, NULL if no triangle hit. */
        const Tri*      tri;

        /** Barycentric coordinate of the hit that multiplies vertex 1. */
        float           u;

        /** Barycentric coordinate of the hit that multiplies vertex 2. */
        float           v;

        Intersector();

        /** @brief Computes the two-sided intersection of the ray and triangle.  
     
          Called repeatedly by KDTree::intersect.

          If an intersection is found that is closer than @a distance, updates 
          distance and stores the result in @a this.  Sample usage:
          
          <pre>
            Intersector hit;
            float distance = inf();
            for (int i = 0; i < m_array.size(); ++i) {
                hit(ray, m_array[i], distance);
            }
          </pre>      
          */
        void operator()(const Ray& ray, const Tri& tri, float& distance);

        /** Computes information about the intersection from an
            established Intersector.  The normal will have unit
            length; it is the interpolated vertex normal, not the
            face normal.  If the tangent is non-zero, it has unit
            length. It may not be precisely perpendicular to the
            normal.

            To obtain the face normal, triangle, and material, use
            the tri member.
          */
        void getResult
        (Vector3&        location,
         Vector3&        normal,
         Vector2&        texCoord,
         Vector3&        tangent1,
         Vector3&        tangent2) const;

        void getResult
        (Vector3&        location,
         Vector3&        normal,
         Vector2&        texCoord) const;
    };

    /** Extract world-space triangles from the model and append them
        onto triArray.

        \param xform After transforming to world space, transform by this xform
         (e.g., to get to a camera's object space, pass the inverse of the camera's
          object-to-world matrix)
     */
    static void getTris(const ReferenceCountedPointer<class Surface>& model, Array<Tri>& triArray, 
                        const CFrame& xform = CFrame());
};

} // namespace G3D

// Needed for InlineKDTree and KDTree
template<> struct BoundsTrait<G3D::Tri> {
    static void getBounds(const G3D::Tri& tri, G3D::AABox& out) { 
        tri.getBounds(out);
    }
};

// Needed for KDTree
template <> struct HashTrait<G3D::Tri> {
    static size_t hashCode(const G3D::Tri& tri) { 
        return tri.hashCode();
    }
};

#endif

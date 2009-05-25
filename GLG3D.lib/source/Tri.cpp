#include "Tri.h"
#include "G3D/Ray.h"
#include "GLG3D/PosedModel.h"
#include "GLG3D/GenericPosedModel.h"

namespace G3D {

Tri::Tri() : m_data(NULL) {}

Tri::~Tri() {}

Tri::Tri(
    const Vector3& v0, const Vector3& v1, const Vector3& v2, 
    const Vector3& n0, const Vector3& n1, const Vector3& n2,
    void* data,
    const Material::Ref& material,
    const Vector2& t0, const Vector2& t1, const Vector2& t2,
    const Vector3& tan0, const Vector3& tan1, const Vector3& tan2) :
    v0(v0), e1(v1 - v0), e2(v2 - v0),
    m_material(material),
    m_data(data) {

    debugAssert(material.isNull() ||
                (isValidHeapPointer(material.pointer()) &&
                 (material.pointer() > (void*)0xf)));

    m_normal[0] = n0;
    m_normal[1] = n1;
    m_normal[2] = n2;

    m_texCoord[0] = t0;
    m_texCoord[1] = t1;
    m_texCoord[2] = t2;

    m_tangent[0] = tan0;
    m_tangent[1] = tan1;
    m_tangent[2] = tan2;

    n = e1.cross(e2).direction();
}


Tri Tri::otherSide() const {
    Tri t;
    
    t.m_data = m_data;
    t.m_material = m_material;
    t.v0     = v0;
    t.e1     = e2;
    t.e2     = e1;

    // Flip winding and normal/tangent direction
    // by swapping elements 1 and 2.
    t.n = -n;

    t.m_normal[0]   = -m_normal[0];
    t.m_texCoord[0] = m_texCoord[0];
    t.m_tangent[0]  = -m_tangent[0];

    for (int i = 1; i < 3; ++i) {
        int j = 3 - i;
        t.m_normal[i]   = -m_normal[j];
        t.m_texCoord[i] = m_texCoord[j];
        t.m_tangent[i]  = -m_tangent[j];
    }
    
    return t;
}


Tri::Intersector::Intersector() : tri(NULL), u(0), v(0) {}

#ifdef _MSC_VER
// Turn on fast floating-point optimizations
#pragma float_control( push )
#pragma fp_contract( on )
#pragma fenv_access( off )
#pragma float_control( except, off )
#pragma float_control( precise, off )
#endif

void Tri::Intersector::operator()(const Ray& ray, const Tri& tri, float& distance) {
    // See RTR3 p.746 (RTR2 ch. 13.7) for the basic algorithm.
    static const float EPS = 1e-12f;

    // Test for backfaces first because this eliminates 50% of all triangles.
    if (tri.n.dot(ray.direction) >= -EPS) {
        // Backface or nearly parallel
        return;
    }

    const Vector3& v0 = tri.v0;
    const Vector3& e1 = tri.e1;
    const Vector3& e2 = tri.e2;

    const Vector3& p = ray.direction.cross(e2);
    const float a = e1.dot(p);

    debugAssert(a >= -1e-7);

    const Vector3& s = ray.origin - v0;
    const float ua = s.dot(p);

    // Note: (ua > a) == (u > 1). Delaying the division by a until
    // after all u-v tests have passed gives a 6% speedup.
    if ((ua < 0.0f) || (ua > a)) {
        // We hit the plane of the triangle, but outside the triangle
        return;
    }

    const Vector3& q = s.cross(e1);
    const float va = ray.direction.dot(q);

    if ((va < 0.0f) || ((ua + va) > a)) {
        // We hit the plane of the triangle, but outside the triangle.
        return;
    }

    if (a < EPS) {
        // This ray was parallel, but passed the backface test. We don't test until
        // down here because this case happens really infrequently.
        return;
    }

    // Divide by a
    const float f = 1.0f / a;
    const float t = e2.dot(q) * f;

    if ((t > 0.0f) && (t < distance)) {
        // Alpha masking
        if (tri.m_material.notNull() && (tri.m_material->bsdf()->lambertian().min().a < 0.5f)) {
            // This texture has an alpha channel; we have to test against it.
            const float u = ua * f;
            const float v = va * f;
            const float w = 1.0 - u - v;
            
            const Image4::Ref& image = tri.m_material->bsdf()->lambertian().image();

            Vector2 texCoord = 
                w * tri.m_texCoord[0] + 
                u * tri.m_texCoord[1] +
                v * tri.m_texCoord[2];
            
            texCoord.x *= image->width();
            texCoord.y *= image->height();

            if (image->nearest(texCoord).a < 0.5f) {
                // Alpha masked location--passed through this tri
                return;
            }

            // Hit the opaque part
            this->u = u;
            this->v = v;
        } else {
            // Opaque triangle; don't bother alpha testing

            // This is a new hit.  Save away the data about the hit
            // location, but don't bother computing barycentric w,
            // the hit location or the normal until after we've checked
            // against all triangles.
            this->u   = ua * f;
            this->v   = va * f;
        }

        distance = t;
        this->tri = &tri;
    }
}


void Tri::Intersector::getResult
(Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord) const {

    debugAssert(tri);
    float w = 1.0 - u - v;

    location = tri->v0 + 
               u * tri->e1 +
               v * tri->e2;

    texCoord = w * tri->m_texCoord[0] + 
               u * tri->m_texCoord[1] +
               v * tri->m_texCoord[2];

    normal   = w * tri->m_normal[0] + 
               u * tri->m_normal[1] +
               v * tri->m_normal[2];

    normal.unitize();
}


void Tri::Intersector::getResult
(Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord,
 Vector3&        tangent) const {

    getResult(location, normal, texCoord);

    float w = 1.0 - u - v;

    if (tri->m_tangent[0].isZero()) {
        tangent = Vector3::zero();
    } else {
        tangent  = w * tri->m_tangent[0] + 
                   u * tri->m_tangent[1] +
                   v * tri->m_tangent[2];
        tangent.unitize();
    }
}


#ifdef _MSC_VER
// Turn off fast floating-point optimizations
#pragma float_control( pop )
#endif

void Tri::getTris(const PosedModel::Ref& pmodel, Array<Tri>& triArray, const CFrame& xform) {
    const GenericPosedModel::Ref& model = pmodel.downcast<GenericPosedModel>();

    alwaysAssertM(model.notNull(), "Non-GenericPosedModel present in World.");
    const GenericPosedModel::CPUGeom& cpuGeom = model->cpuGeom();
    const GenericPosedModel::GPUGeom::Ref& gpuGeom = model->gpuGeom();
    
    bool twoSided = gpuGeom->twoSided;

    Material* material = gpuGeom->material.pointer();
    
    debugAssert(gpuGeom->primitive == PrimitiveType::TRIANGLES);
    debugAssert(cpuGeom.index != NULL);

    const Array<int>&     index     = *cpuGeom.index;

    // All data are in object space
    const Array<Vector3>& vertex    = cpuGeom.geometry->vertexArray;
    const Array<Vector3>& normal    = cpuGeom.geometry->normalArray;
    const Array<Vector2>& texCoord0 = *cpuGeom.texCoord0;
    const Array<Vector3>& tangent   = *cpuGeom.tangent;

    bool hasTexCoords = texCoord0.size() > 0;
    bool hasTangents  = tangent.size() > 0;

    // Object to world matrix.  Guaranteed to be an RT transformation,
    // so we can directly transform normals as if they were vectors.
    CFrame cframe = xform * model->coordinateFrame();

    for (int i = 0; i < index.size(); i += 3) {
        const int v0 = index[i + 0];
        const int v1 = index[i + 1];
        const int v2 = index[i + 2];

        triArray.append
            (Tri(cframe.pointToWorldSpace(vertex[v0]),    
                 cframe.pointToWorldSpace(vertex[v1]),
                 cframe.pointToWorldSpace(vertex[v2]),

                 cframe.vectorToWorldSpace(normal[v0]),  
                 cframe.vectorToWorldSpace(normal[v1]),  
                 cframe.vectorToWorldSpace(normal[v2]),

                 NULL,            
                 material,
                            
                 hasTexCoords ? texCoord0[v0] : Vector2::zero(), 
                 hasTexCoords ? texCoord0[v1] : Vector2::zero(), 
                 hasTexCoords ? texCoord0[v2] : Vector2::zero(),

                 hasTangents  ? cframe.vectorToWorldSpace(tangent[v0]) : Vector3::zero(),
                 hasTangents  ? cframe.vectorToWorldSpace(tangent[v1]) : Vector3::zero(),
                 hasTangents  ? cframe.vectorToWorldSpace(tangent[v2]) : Vector3::zero()));

        if (twoSided) {
            const Tri& t = triArray.last().otherSide();
            triArray.append(t);
        }
    }
}

} // namespace G3D

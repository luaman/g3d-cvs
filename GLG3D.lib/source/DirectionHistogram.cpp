/**
  @file DirectionHistogram.h
  @maintainer Morgan McGuire, morgan@cs.williams.edu
  @created 2009-03-25
  @edited  2009-03-25
*/

#include "GLG3D/DirectionHistogram.h"
#include "GLG3D/IFSModel.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

float DirectionHistogram::tetrahedronVolume(const Vector3& v0, const Vector3& v1, const Vector3& v2) {
    // non-unit outward facing normal
    Vector3 n = (v1 - v0).cross(v2 - v0);

    float L = n.length();
    if (L <= 0) {
        // Zero volume tetrahedron
        return 0.0f;
    }

    // Area of the base
    float A0 = 0.5f * L;

    // Height
    float h = v0.dot(n) / L;

    return h * A0 * 0.33333333f;
}


float DirectionHistogram::totalVolume() const {
    float volume = 0.0f;
    for (int i = 0; i < m_meshIndex.size(); i += 3) {
        const int i0 = m_meshIndex[i];
        const int i1 = m_meshIndex[i + 1];
        const int i2 = m_meshIndex[i + 2];

        const Vector3& v0 = m_meshVertex[i0] * m_bucket[i0];
        const Vector3& v1 = m_meshVertex[i1] * m_bucket[i1];
        const Vector3& v2 = m_meshVertex[i2] * m_bucket[i2];

        volume += tetrahedronVolume(v0, v1, v2);
    }
    return volume;
}


void DirectionHistogram::sendGeometry(RenderDevice* rd) {
    if (m_dirty) {
        if (fuzzyEq(m_totalWeight, 0.0f)) {
            // Sphere case (no data)
            m_gpuMeshVertex.update(m_meshVertex);
        } else {
            // Distort case
            Vector3* v = reinterpret_cast<Vector3*>(m_gpuMeshVertex.mapBuffer(GL_WRITE_ONLY));

            const float volume = totalVolume();

            // Normalizing factor to keep total volume at 4/3 * pi                
            const float s = 1.0f / (float)pow((volume * 3.0) / (4.0 * pi()), 1.0 / 3.0);
            //m_meshVertex.size() / m_totalWeight;

            for (int i = 0; i < m_meshVertex.size(); ++i) {
                v[i] = m_meshVertex[i] * m_bucket[i] * s;
            }
            m_gpuMeshVertex.unmapBuffer();
        }
        m_dirty = false;
    }
    rd->beginIndexedPrimitives();
    {
        rd->setVertexArray(m_gpuMeshVertex);
        rd->sendIndices(RenderDevice::TRIANGLES, m_gpuMeshIndex);
    }
    rd->endIndexedPrimitives();
}


DirectionHistogram::DirectionHistogram(float sharp) : m_sharp(sharp) {
    // Load data
    Array<Vector2>  ignore;
    std::string     name;

    IFSModel::load(System::findDataFile("sphere.ifs"), name, m_meshIndex, m_meshVertex, ignore);

    // Normalize the sphere
    for (int i = 0; i < m_meshVertex.size(); ++i) {
        m_meshVertex[i] = m_meshVertex[i].direction();
    }
    m_bucket.resize(m_meshVertex.size());
    reset();

    VARArea::Ref dataArea = VARArea::create(sizeof(Vector3) * m_meshVertex.size(),
        VARArea::WRITE_EVERY_FEW_FRAMES);
    m_gpuMeshVertex = VAR(m_meshVertex, dataArea);

    VARArea::Ref indexArea = VARArea::create(sizeof(int) * m_meshIndex.size(), 
        VARArea::WRITE_ONCE, VARArea::INDEX);
    m_gpuMeshIndex = VAR(m_meshIndex, indexArea);

    m_dirty = false;

    // sharp-th root of epsilon
    m_cutoff = (float)pow(0.00001, 1.0 / m_sharp);
}


void DirectionHistogram::reset() {
    m_totalWeight = 0.0f;
    System::memset(m_bucket.getCArray(), 0, sizeof(float) * m_bucket.size());
    m_dirty = true;
}


void DirectionHistogram::insert(const Vector3& vector, float weight) {
    const Vector3& v = vector.direction();

    // Update nearby buckets
    for (int i = 0; i < m_meshVertex.size(); ++i) {
        // Weight
        float w = m_meshVertex[i].dot(v);
        
        if (w >= m_cutoff) {
            // This weight may be significant
            w              = powf(w, m_sharp) * weight;
            m_bucket[i]   += w;
            m_totalWeight += w;
            debugAssert(w > 0);
        }
    }
    m_dirty = true;
}


void DirectionHistogram::render(
    RenderDevice* rd, 
    const Color3& solidColor, 
    const Color4& lineColor) {
    
    rd->pushState();
    {
        rd->disableLighting();

        // Hidden surfaces
        rd->setPolygonOffset(-0.75f);
        rd->setColor(solidColor);
        sendGeometry(rd);

        // Lines
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setColor(lineColor);
        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        sendGeometry(rd);
    }
    rd->popState();
}

} // G3D

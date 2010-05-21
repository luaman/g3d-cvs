/**
  @file DirectionHistogram.h
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2009-03-25
  @edited  2009-03-25
*/

#include "G3D/ThreadSet.h"
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
    for (int i = 0; i < m_meshIndex.size(); i += 4) {
        const int i0 = m_meshIndex[i];
        const int i1 = m_meshIndex[i + 1];
        const int i2 = m_meshIndex[i + 2];
        const int i3 = m_meshIndex[i + 3];

        const Vector3& v0 = m_meshVertex[i0] * m_bucket[i0] * m_invArea[i0];
        const Vector3& v1 = m_meshVertex[i1] * m_bucket[i1] * m_invArea[i1];
        const Vector3& v2 = m_meshVertex[i2] * m_bucket[i2] * m_invArea[i2];
        const Vector3& v3 = m_meshVertex[i3] * m_bucket[i3] * m_invArea[i3];

        volume += tetrahedronVolume(v0, v1, v2);
        volume += tetrahedronVolume(v0, v2, v3);
    }
    return volume;
}


void DirectionHistogram::sendGeometry(RenderDevice* rd) {
    if (m_dirty) {
        if (m_numSamples == 0) {
            // Sphere case (no data)
            m_gpuMeshVertex.update(m_meshVertex);
        } else {
            // Distort case
            Vector3* v = reinterpret_cast<Vector3*>(m_gpuMeshVertex.mapBuffer(GL_WRITE_ONLY));

            /*
            const float volume = totalVolume();
            // Normalizing factor to keep total volume at 4/3 * pi                
            const float s = 1.0f / (float)pow((volume * 3.0) / (4.0 * pi()), 1.0 / 3.0);
            */

            // Normalizing factor to keep a uniform sphere at radius ~= 1
            float s = 0;
            for (int i = 0; i < m_meshVertex.size(); ++i) {
                s += m_bucket[i] * m_invArea[i];
            }
            s = 300.0 / s;

            for (int i = 0; i < m_meshVertex.size(); ++i) {
                v[i] = m_meshVertex[i] * m_bucket[i] * s * m_invArea[i];
            }

            m_gpuMeshVertex.unmapBuffer();
        }
        m_dirty = false;
    }
    rd->beginIndexedPrimitives();
    {
        rd->setVertexArray(m_gpuMeshVertex);
        rd->sendIndices(PrimitiveType::QUADS, m_gpuMeshIndex);
    }
    rd->endIndexedPrimitives();
}


DirectionHistogram::DirectionHistogram(int numSlices, const Vector3& axis) : m_slices(numSlices) {
    alwaysAssertM(numSlices >= 4, "At least four slices required");

    // Turn on old hemisphere-only optimization
    const bool hemi = true;

    // Normalize
    const Vector3& Z = axis.direction();

    Vector3 X = (abs(Z.dot(Vector3::unitX())) <= 0.9f) ? Vector3::unitX() : Vector3::unitY();
    X = (X - Z * (Z.dot(X))).direction();
    const Vector3 Y = Z.cross(X);

    // Only generate upper hemisphere
    static const int P = m_slices;
    static const int T = hemi ? (m_slices / 2) : m_slices;
    const float thetaExtent = hemi ? G3D::halfPi() : G3D::pi();

    for (int t = 0; t < T; ++t) {
        const float theta = t * thetaExtent / (T - 1.0f);
        const float z = cos(theta);
        const float r = sin(theta);

        const bool firstRow = (t == 0);
        const bool secondRow = (t == 1);
        const bool lastRow = ! hemi && (t == T - 1);

        for (int p = 0; p < P; ++p) {
            const float phi = p * G3D::twoPi() / P;
            const float x = cos(phi) * r;
            const float y = sin(phi) * r;

            const bool unique = (! firstRow && ! lastRow) || (p == 0);
                
            // Only insert one vertex at each pole
            if (unique) {
                m_meshVertex.append(X * x + Y * y + Z * z);
            }

            const int i = m_meshVertex.size() - 1;
            // Index of the start of this row
            const int rowStart  = ((i - 1) / P) * P + 1;
            const int colOffset = i - rowStart;

            if (firstRow) {
                // (First row generates no quads)
            } else if (secondRow) {                
                // Degnererate north pole
                m_meshIndex.append(0, 0, i, rowStart + (colOffset + 1) % P);

            } else if (lastRow) {
                // Degenerate south pole
                m_meshIndex.append(i, i, i - p - 1, i - p - 2);

            } else {

                m_meshIndex.append(
                    i - P, 
                    i, 
                    rowStart + (colOffset + 1) % P, 
                    rowStart + ((colOffset + 1) % P) - P);
            }
        }
    }

    m_bucket.resize(m_meshVertex.size());
    reset();

    // We initially accumulate areas and then invert them in a follow-up pass
    m_invArea.resize(m_meshIndex.size());
    // Zero the array
    System::memset(m_invArea.getCArray(), 0, sizeof(float) * m_invArea.size());

    // Create triTree
    {
        Array<Tri> triArray;
        for (int q = 0; q < m_meshIndex.size(); q += 4) {
            int i0 = m_meshIndex[q];
            int i1 = m_meshIndex[q + 1];
            int i2 = m_meshIndex[q + 2];
            int i3 = m_meshIndex[q + 3];

            void* pointer = &m_meshIndex[q];

            // Create two tris for each quad
            // Wind backwards; these tris have to face inward

            Tri A(m_meshVertex[i0], m_meshVertex[i3], m_meshVertex[i2], m_meshVertex[i0], m_meshVertex[i3], m_meshVertex[i2], pointer);
            Tri B(m_meshVertex[i0], m_meshVertex[i2], m_meshVertex[i1], m_meshVertex[i0], m_meshVertex[i2], m_meshVertex[i1], pointer);

            triArray.append(A);
            triArray.append(B);

            // Attribute the area of the surrounding quads to each vertex.  If we don't do this, then
            // vertices near the equator will recieve only half of the correct probability.
            float area = A.area() + B.area();
            m_invArea[i0] += area;
            m_invArea[i1] += area;
            m_invArea[i2] += area;
            m_invArea[i3] += area;
        }
        m_tree.setContents(triArray);

        for (int i = 0; i < m_invArea.size(); ++i) {
            // Multiply by a small number to keep these from getting too large
            m_invArea[i] = 0.001f / m_invArea[i];
        }
    }

    VertexBuffer::Ref dataArea = VertexBuffer::create(sizeof(Vector3) * m_meshVertex.size(),
        VertexBuffer::WRITE_EVERY_FEW_FRAMES);
    m_gpuMeshVertex = VertexRange(m_meshVertex, dataArea);

    VertexBuffer::Ref indexArea = VertexBuffer::create(sizeof(int) * m_meshIndex.size(), 
        VertexBuffer::WRITE_ONCE, VertexBuffer::INDEX);
    m_gpuMeshIndex = VertexRange(m_meshIndex, indexArea);

    m_dirty = false;
}


void DirectionHistogram::reset() {
    m_numSamples = 0;
    System::memset(m_bucket.getCArray(), 0, sizeof(float) * m_bucket.size());
    m_dirty = true;
}


void DirectionHistogram::insert(const Array<Vector3>& vector, const Array<float>& weight) {
    for (int i = 0; i < vector.size(); ++i) {
        insert(vector[i], weight[i]);
    }
}


void DirectionHistogram::insert(const Vector3& vector, float weight) {
    // Find the quad hit
    float distance = inf();
    Tri::Intersector intersector;

    if (m_tree.intersectRay(Ray(Vector3::zero(), vector.direction()), intersector, distance)) {
        ++m_numSamples;

        // Hit
        const int* index = reinterpret_cast<int*>(intersector.tri->data());
        
        // Increment all vertices surrounding the quad
        for (int j = 0; j < 4; ++j) {
            int k = index[j]; 
            m_bucket[k] += weight;
        }
        m_dirty = true;
    }
}


void DirectionHistogram::render(
    RenderDevice* rd, 
    const Color3& solidColor, 
    const Color4& lineColor) {
    
    rd->pushState();
    {
        rd->disableLighting();

        // Hidden surfaces
        rd->setPolygonOffset(0.2f);
        rd->setColor(solidColor);
        sendGeometry(rd);

        rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);

        // Lines
        rd->setPolygonOffset(-0.2f);
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setColor(lineColor);
        rd->setLineWidth(1.0f);
        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        sendGeometry(rd);
    }
    rd->popState();
}

} // G3D

/**
  @file DirectionHistogram.h
  @maintainer Morgan McGuire, morgan@cs.williams.edu
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
        rd->sendIndices(PrimitiveType::QUADS, m_gpuMeshIndex);
    }
    rd->endIndexedPrimitives();
}


DirectionHistogram::DirectionHistogram(float sharp, const Vector3& hemiAxis) : m_sharp(sharp) {
    bool hemi = ! hemiAxis.isZero();

    if (hemi) {
        // Normalize
        const Vector3& Z = hemiAxis.direction();
        Vector3 X = (abs(Z.dot(Vector3::unitX())) <= 0.9f) ? Vector3::unitX() : Vector3::unitY();
        X = (X - Z * (Z.dot(X))).direction();
        const Vector3 Y = Z.cross(X);

        // Only generate upper hemisphere
        static const int P = 36 * 2;
        static const int T = 36;

        for (int t = 0; t < T; ++t) {
            const float theta = t * G3D::halfPi() / (T - 1.0f);
            const float z = cos(theta);
            const float r = sin(theta);
            for (int p = 0; p < P; ++p) {
                const float phi = p * G3D::twoPi() / P;
                const float x = cos(phi) * r;
                const float y = sin(phi) * r;

                m_meshVertex.append(X * x + Y * y + Z * z);

                if (t > 0) {
                    int i = m_meshVertex.size() - 1;
                    int rowIndex = t * P;
                    int colIndex = p;
                    m_meshIndex.append(rowIndex + colIndex - P, rowIndex + colIndex, 
                        rowIndex + ((colIndex + 1) % P), rowIndex - P + ((colIndex + 1) % P));
                }
            }
        }
    } else {
        static const int P = 36 * 2;
        static const int T = 36 * 2;

        for (int t = 0; t < T; ++t) {
            const float theta = t * G3D::pi() / (T - 1.0f);
            const float z = cos(theta);
            const float r = sin(theta);
            for (int p = 0; p < P; ++p) {
                const float phi = p * G3D::twoPi() / P;
                const float x = cos(phi) * r;
                const float y = sin(phi) * r;

                m_meshVertex.append(Vector3(x, y, z));

                if (t > 0) {
                    int i = m_meshVertex.size() - 1;
                    int rowIndex = t * P;
                    int colIndex = p;
                    m_meshIndex.append(rowIndex + colIndex - P, rowIndex + colIndex, 
                        rowIndex + ((colIndex + 1) % P), rowIndex - P + ((colIndex + 1) % P));
                }
            }
        }
    }

    m_bucket.resize(m_meshVertex.size());
    reset();

    VertexBuffer::Ref dataArea = VertexBuffer::create(sizeof(Vector3) * m_meshVertex.size(),
        VertexBuffer::WRITE_EVERY_FEW_FRAMES);
    m_gpuMeshVertex = VertexRange(m_meshVertex, dataArea);

    VertexBuffer::Ref indexArea = VertexBuffer::create(sizeof(int) * m_meshIndex.size(), 
        VertexBuffer::WRITE_ONCE, VertexBuffer::INDEX);
    m_gpuMeshIndex = VertexRange(m_meshIndex, indexArea);

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

    insert(v, weight, 0, m_meshVertex.size() - 1);

    m_dirty = true;
}


void DirectionHistogram::insert(const Vector3& vector, float weight, int startIndex, int stopIndex) {
    // Update nearby buckets
    for (int i = startIndex; i <= stopIndex; ++i) {
        // Weight
        float w = m_meshVertex[i].dot(vector);
        
        if (w >= m_cutoff) {
            // This weight may be significant
            w              = powf(w, m_sharp) * weight;
            m_bucket[i]   += w;
            m_totalWeight += w;
        }
    }
}


DirectionHistogram::WorkerThread::WorkerThread
(DirectionHistogram* h, int start, int stop,
    const Array<Vector3>& vector, const Array<float>& weight) : 
    GThread("DirectionHistogram"),
    hist(h), startIndex(start), stopIndex(stop), vector(vector), weight(weight) {
}

    
void DirectionHistogram::WorkerThread::threadMain() {
    for (int i = 0; i < vector.size(); ++i) {
        hist->insert(vector[i], weight[i], startIndex, stopIndex);
    }
}


void DirectionHistogram::insert(const Array<Vector3>& vector, const Array<float>& weight) {
    int numThreads = System::numCores();

    // Normalize all vectors first
    Array<Vector3> v(vector.size());
    Array<float> w = weight;

    for (int i = 0; i < v.size(); ++i) {
        v[i] = vector[i].direction();
    }

    if (w.size() == 0) {
        // Ensure that there are weights
        w.resize(v.size());
        for (int i = 0; i < w.size(); ++i) {
            w[i] = 1.0f;
        }
    }

    const int N = m_meshVertex.size();

    m_dirty = true;

    // Launch
    if (numThreads > vector.size() / 10) {
        // Don't bother with threads
        for (int i = 0; i < v.size(); ++i) {
            insert(v[i], w[i], 0, N - 1);
        }
        return;
    }

    ThreadSet threads;

    for (int i = 0; i < numThreads; ++i) {
        int start = i * N / numThreads;
        int stop  = (i + 1) * N / numThreads - 1;
        threads.insert(new WorkerThread(this, start, stop, v, w));
    }
    threads.start(GThread::USE_CURRENT_THREAD);
    threads.waitForCompletion();
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

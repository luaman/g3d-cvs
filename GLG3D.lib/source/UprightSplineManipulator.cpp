/**
  @file UprightSplineManipulator.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2007-06-01
  @edited  2008-10-08
*/
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/GCamera.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/Draw.h"
#include "G3D/Sphere.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "GLG3D/UprightSplineManipulator.h"

namespace G3D {

class PosedCameraSpline : public Surface {
private:

    UprightSpline*  spline;
    Color3          color;

    VertexRange     vertex;
    int             numVertices;

    AABox           boxBounds;

public:

    PosedCameraSpline(UprightSpline* s, const Color3& c) : spline(s), color(c) {
        if (spline->control.size() > 1) {
            numVertices = spline->control.size() * 11 + 1;

            int count = spline->control.size();
            if (spline->cyclic) {
                ++count;
            }

            VertexBufferRef area = VertexBuffer::create(sizeof(Vector3) * numVertices);
            Array<Vector3> v;
            v.resize(numVertices);
            
            AABox boxBounds;
            for (int i = 0; i < numVertices; ++i) {
                float s = count * i / (float)(numVertices - 1);
                v[i] = spline->evaluate(s).translation;

                if (i == 0) {
                    boxBounds = AABox(v[i]);
                } else {
                    boxBounds.merge(v[i]);
                }
            }
            
            vertex = VertexRange(v, area);
        }
    }

    void drawSplineCurve(RenderDevice* rd) const {
        rd->sendSequentialIndices(PrimitiveType::LINE_STRIP, numVertices);
    }
    
    virtual void render (RenderDevice* rd) const {
        rd->pushState();

        // Draw control points
        if (spline->control.size() > 0) {
            Draw::sphere(Sphere(spline->control[0].translation, 0.1f), rd, Color3::green(), Color4::clear());
            Draw::sphere(Sphere(spline->control.last().translation, 0.1f), rd, Color3::black(), Color4::clear());
        }
        Vector3 extent(0.07f, 0.07f, 0.07f);
        //AABox box(spline->control[i].translation - extent, spline->control[i].translation + extent);
        AABox box(-extent, extent);
        for (int i = 1; i < spline->control.size() - 1; ++i) {
            rd->setObjectToWorldMatrix(spline->control[i].toCoordinateFrame());
            Draw::box(box, rd, color, Color4::clear());
        }
        rd->popState();

        if (spline->control.size() < 4) {
            return;
        }

        rd->pushState();
            rd->setObjectToWorldMatrix(CoordinateFrame());
        
            Color4 c;
            glEnable(GL_FOG);
            glFogf(GL_FOG_START, 40.0f);
            glFogf(GL_FOG_END, 120.0);
            glFogi(GL_FOG_MODE, GL_LINEAR);

            rd->beginIndexedPrimitives();
            rd->setVertexArray(vertex);
        
            rd->disableLighting();
            glEnable(GL_LINE_SMOOTH);
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            c = Color4((Color3::white() * 2.0f + color) / 3.0f, 0);
            glFogfv(GL_FOG_COLOR, (float*)&c);

            // Core
            glFogf(GL_FOG_START, 5.0f);
            glFogf(GL_FOG_END, 60.0);
            rd->setColor(color);
            rd->setLineWidth(2);
            drawSplineCurve(rd);

            rd->setDepthWrite(false);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);

            // Core:
            rd->setDepthWrite(false);
            c = Color3::black();
            glFogfv(GL_FOG_COLOR, (float*)&c);

            GLint i = GL_FRAGMENT_DEPTH_EXT;
            glFogiv(GL_FOG_COORDINATE_SOURCE_EXT, &i);
            glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);

            // Glow:
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE);
            rd->setColor(Color3::white() * 0.5);
            rd->setLineWidth(7);
            drawSplineCurve(rd);
            
            rd->setColor(color * 0.5f);
            rd->setLineWidth(12);
            drawSplineCurve(rd);


            glDisable(GL_FOG);
            rd->endIndexedPrimitives();
        
        rd->popState();
    }


    virtual const Array< MeshAlg::Edge > & edges () const {
        static Array< MeshAlg::Edge > x;
        return x;
    }

    virtual const Array< MeshAlg::Face > & faces () const {
        static Array< MeshAlg::Face > x;
        return x;
    }

    virtual void getCoordinateFrame (CoordinateFrame &c) const {
        c = CoordinateFrame();
    }

    virtual void getObjectSpaceBoundingBox(AABox& b) const {
        b = boxBounds;
    }

    virtual void getObjectSpaceBoundingSphere(Sphere& s) const {
        boxBounds.getBounds(s);
    }

    virtual void getObjectSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize = true) const {
        (void)faceNormals;
        (void)normalize;
    }

    virtual void getWorldSpaceBoundingBox (AABox &box) const {
        getObjectSpaceBoundingBox(box);
    }

    virtual void getWorldSpaceBoundingSphere (Sphere &s) const {
        getObjectSpaceBoundingSphere(s);
    }

    virtual bool hasTexCoords () const {
        return false;
    }

    virtual bool hasTransparency () const {
        return true;
    }

    virtual std::string name () const {
        return "Camera Spline";
    }

    virtual int numBoundaryEdges () const {
        return 0;
    }

    virtual int numWeldedBoundaryEdges () const {
        return 0;
    }

    virtual const Array<Vector3>& objectSpaceFaceNormals(bool normalize = true) const {
        static Array<Vector3> x;
        (void)normalize;
        return x;
    }

    virtual const MeshAlg::Geometry& objectSpaceGeometry() const {
        static MeshAlg::Geometry x;
        return x;
    }

    virtual const Array<int>& triangleIndices() const {
        static Array<int> x;
        return x;
    }

    virtual const Array< MeshAlg::Vertex > & vertices () const {
        static Array< MeshAlg::Vertex > x;
        return x;
    }

    virtual const Array< MeshAlg::Edge > & weldedEdges () const {
        static Array< MeshAlg::Edge >  x;
        return x;
    }

    virtual const Array< MeshAlg::Face > & weldedFaces () const {
        static Array< MeshAlg::Face > x;
        return x;
    }

    virtual const Array< MeshAlg::Vertex > & weldedVertices () const {
        static Array< MeshAlg::Vertex > x;
        return x;
    }
};

UprightSplineManipulator::Ref UprightSplineManipulator::create(class GCamera* c) {
    UprightSplineManipulator* manipulator = new UprightSplineManipulator();
    manipulator->setCamera(c);
    return manipulator;
}


UprightSplineManipulator::UprightSplineManipulator() 
    : m_time(0), 
      m_mode(INACTIVE_MODE), 
      m_camera(NULL), 
      m_showPath(true), 
      m_pathColor(Color3::red()), 
      m_sampleRate(1),
      m_recordKey(' ') {
}


CoordinateFrame UprightSplineManipulator::frame() const {
    return m_currentFrame;
}


void UprightSplineManipulator::getFrame(CoordinateFrame &c) const {
    c = m_currentFrame;
}


void UprightSplineManipulator::clear() {
    m_spline.clear();
    setTime(0);
}

void UprightSplineManipulator::setMode(Mode m) {
    m_mode = m;
    if (m_mode == RECORD_KEY_MODE || m_mode == RECORD_INTERVAL_MODE) {
        debugAssertM(m_camera != NULL, "Cannot enter record mode without first setting the camera");
    }
}


UprightSplineManipulator::Mode UprightSplineManipulator::mode() const {
    return m_mode;
}


void UprightSplineManipulator::onPose
(Array< SurfaceRef > &posedArray, 
 Array< Surface2DRef > &posed2DArray) {

    (void)posed2DArray;

    if (m_showPath && (m_spline.control.size() > 0)) {
        posedArray.append(new PosedCameraSpline(&m_spline, m_pathColor));
    }
}


bool UprightSplineManipulator::onEvent (const GEvent &event) {
    if ((m_mode == RECORD_KEY_MODE) && 
        (event.type == (uint8)GEventType::KEY_DOWN) &&
        (event.key.keysym.sym == m_recordKey) &&
        (m_camera != NULL)) {

        // Capture data point
        m_spline.append(m_camera->coordinateFrame());
        
        // Consume the event
        return true;
    }

    return false;
}


void UprightSplineManipulator::onSimulation (RealTime rdt, SimTime sdt, SimTime idt) {
    (void)rdt;
    (void)idt;
    if (m_mode != INACTIVE_MODE) {
        setTime(m_time + sdt);
    }
}


void UprightSplineManipulator::onUserInput(UserInput* ui) {
    (void)ui;
}


void UprightSplineManipulator::setTime(double t) {
    m_time = t;

    switch (m_mode) {
    case PLAY_MODE:
        if (m_spline.control.size() >= 4) {
            m_currentFrame = m_spline.evaluate((float)t * (float)m_sampleRate).toCoordinateFrame();
        } else {
            // Not enough points for a spline
            m_currentFrame = CoordinateFrame();
        }
        break;
        
    case RECORD_INTERVAL_MODE:
        if (m_camera != NULL) {
            // We have a camera
            if (m_time * m_sampleRate > m_spline.control.size()) {
                // Enough time has elapsed to capture a new data point
                m_spline.append(m_camera->coordinateFrame());
            }
        }
        break;

    case RECORD_KEY_MODE:
        break;

    case INACTIVE_MODE:
        break;
    }
}

}

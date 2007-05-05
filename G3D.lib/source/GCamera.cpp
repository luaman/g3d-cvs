/**
  @file GCamera.cpp

  @author Morgan McGuire, matrix@graphics3d.com
 
  @created 2001-04-15
  @edited  2006-01-11
*/

#include "G3D/platform.h"
#include "G3D/GCamera.h"
#include "G3D/Rect2D.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Ray.h"

namespace G3D {


GCamera::GCamera() {
    nearPlane   = 0.1f;
    farPlane    = (float)inf();
    setFieldOfView((float)toRadians(55.0f));
}


GCamera::~GCamera() {
}


CoordinateFrame GCamera::getCoordinateFrame() const {
    return cframe;
}


void GCamera::getCoordinateFrame(CoordinateFrame& c) const {
    c = cframe;
}


void GCamera::setCoordinateFrame(const CoordinateFrame& c) {
    cframe = c;
}


void GCamera::setFieldOfView(float angle) {
    debugAssert((angle < pi()) && (angle > 0));
    
    m_fieldOfView = angle;
    
    // Solve for the corresponding image plane depth, as if the extent
    // of the film was 1x1.
    m_imagePlaneDepth = 1.0f / (2.0f * tanf(angle / 2.0f));
}
 

void GCamera::setImagePlaneDepth(
    float                                   depth,
    const class Rect2D&                     viewport) {
	
    debugAssert(depth > 0);
    setFieldOfView(2.0f * atanf(viewport.height() / (2.0f * depth)));
}


float GCamera::imagePlaneDepth(
    const class Rect2D&                     viewport) const {

    // The image plane depth has been pre-computed for 
    // a 1x1 image.  Now that the image is width x height, 
    // we need to scale appropriately. 

    return m_imagePlaneDepth * viewport.height();
}


float GCamera::viewportWidth(const Rect2D& viewport) const {
    return viewportHeight(viewport) * viewport.width() / viewport.height();
}


float GCamera::viewportHeight(const Rect2D& viewport) const {
    (void)viewport;
    return nearPlane / m_imagePlaneDepth;
}


Ray GCamera::worldRay(
    float                                  x,
    float                                  y,
    const Rect2D&                          viewport) const {

    int screenWidth  = iFloor(viewport.width());
    int screenHeight = iFloor(viewport.height());

    Ray out;
    // Set the origin to 0
    out.origin = Vector3::zero();

    float cx = screenWidth  / 2.0f;
    float cy = screenHeight / 2.0f;

    out.direction =
        Vector3( (x - cx),
                -(y - cy),
                -imagePlaneDepth(viewport));

    out = cframe.toWorldSpace(out);

    // Normalize the direction (we didn't do it before)
    out.direction = out.direction.direction();

    return out;
}


Vector3 GCamera::project(
    const Vector3&                      point,
    const Rect2D&                       viewport) const {

    int screenWidth  = (int)viewport.width();
    int screenHeight = (int)viewport.height();

    Vector3 out = cframe.pointToObjectSpace(point);
    float w = -out.z;

    if (w <= 0) {
        return Vector3::inf();
    }
    debugAssert(w > 0);

    // Find where it hits an image plane of these dimensions
    float zImagePlane = imagePlaneDepth(viewport);

    // Recover the distance
    float rhw = zImagePlane / w;

    // Add the image center, flip the y axis
    out.x = screenWidth / 2.0f + (rhw * out.x);
    out.y = screenHeight / 2.0f - (rhw * out.y);
    
    if (farPlane >= inf()) {
        out.z = (-out.z - 2.0f * nearPlane) * rhw;
    } else {
        out.z = rhw * (-out.z * (farPlane + nearPlane) - (2.0f * farPlane * nearPlane)) / (farPlane - nearPlane);
    }

    return out;
}


float GCamera::worldToScreenSpaceArea(float area, float z, const Rect2D& viewport) const {

    if (z >= 0) {
        return (float)inf();
    }

    float zImagePlane = imagePlaneDepth(viewport);

    return area * (float)square(zImagePlane / z);
}


Vector3 GCamera::unproject(const Vector3& v, const Rect2D& viewport) const {
    float n = -nearPlane;
    float f = -farPlane;

    float z;

    if (f >= inf()) {
        // Infinite far plane
        z = 1.0f / (((-1.0f / n) * v.z) + 1.0f / n);
    } else {
        z = 1.0f / ((((1.0f / f) - (1.0f / n)) * v.z) + 1.0f / n);
    }

    Ray ray = worldRay(v.x, v.y, viewport);

    // Find out where the ray reaches the specified depth.
    Vector3 out = ray.origin + ray.direction * z  / ray.direction.z;

    return cframe.pointToWorldSpace(out);
}


void GCamera::getClipPlanes(
    const Rect2D&       viewport,
    Array<Plane>&       clip) const {

    Frustum fr;
    getFrustum(viewport, fr);
    clip.resize(fr.faceArray.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int f = 0; f < clip.size(); ++f) {
        clip[f] = fr.faceArray[f].plane;
    }
}
 


GCamera::Frustum GCamera::frustum(const Rect2D& viewport) const {
    Frustum f;
    getFrustum(viewport, f);
    return f;
}


void GCamera::getFrustum(const Rect2D& viewport, Frustum& fr) const {

    // The volume is the convex hull of the vertices definining the view
    // frustum and the light source point at infinity.

    const float x               = viewportWidth(viewport) / 2;
    const float y               = viewportHeight(viewport) / 2;
    const float z               = nearPlaneZ();
    const float w               = z / farPlaneZ();
    const float fovx            = x * fieldOfView() / y;

    // Near face (ccw from UR)
    fr.vertexPos.append(
        Vector4( x,  y, z, 1),
        Vector4(-x,  y, z, 1),
        Vector4(-x, -y, z, 1),
        Vector4( x, -y, z, 1));

    // Far face (ccw from UR, from origin)
    fr.vertexPos.append(
        Vector4( x,  y, z, w),
        Vector4(-x,  y, z, w),
        Vector4(-x, -y, z, w),
        Vector4( x, -y, z, w));

    Frustum::Face face;

    // Near plane (wind backwards so normal faces into frustum)
    // Recall that nearPlane, farPlane are positive numbers, so
    // we need to negate them to produce actual z values.
    face.plane = Plane(Vector3(0,0,-1), Vector3(0,0,-nearPlane));
    face.vertexIndex[0] = 3;
    face.vertexIndex[1] = 2;
    face.vertexIndex[2] = 1;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Right plane
    face.plane = Plane(Vector3(-cosf(fovx/2), 0, -sinf(fovx/2)), Vector3::zero());
    face.vertexIndex[0] = 0;
    face.vertexIndex[1] = 4;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 3;
    fr.faceArray.append(face);

    // Left plane
    face.plane = Plane(Vector3(-fr.faceArray.last().plane.normal().x, 0, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 5;
    face.vertexIndex[1] = 1;
    face.vertexIndex[2] = 2;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Top plane
    face.plane = Plane(Vector3(0, -cosf(fieldOfView()/2.0f), -sinf(fieldOfView()/2.0f)), Vector3::zero());
    face.vertexIndex[0] = 1;
    face.vertexIndex[1] = 5;
    face.vertexIndex[2] = 4;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Bottom plane
    face.plane = Plane(Vector3(0, -fr.faceArray.last().plane.normal().y, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 2;
    face.vertexIndex[1] = 3;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Far plane
    if (farPlane < inf()) {
    	face.plane = Plane(Vector3(0, 0, 1), Vector3(0, 0, -farPlane));
        face.vertexIndex[0] = 4;
        face.vertexIndex[1] = 5;
        face.vertexIndex[2] = 6;
        face.vertexIndex[3] = 7;
        fr.faceArray.append(face);
    }

    // Transform vertices to world space
    for (int v = 0; v < fr.vertexPos.size(); ++v) {
        fr.vertexPos[v] = cframe.toWorldSpace(fr.vertexPos[v]);
    }

    // Transform planes to world space
    for (int p = 0; p < fr.faceArray.size(); ++p) {
        // Since there is no scale factor, we don't have to 
        // worry about the inverse transpose of the normal.
        Vector3 normal;
        float d;
        
        fr.faceArray[p].plane.getEquation(normal, d);
        
        Vector3 newNormal = cframe.rotation * normal;
        
        if (isFinite(d)) {
            d = (newNormal * -d + cframe.translation).dot(newNormal);
            fr.faceArray[p].plane = Plane(newNormal, newNormal * d);
        } else {
            // When d is infinite, we can't multiply 0's by it without
            // generating NaNs.
            fr.faceArray[p].plane = Plane::fromEquation(newNormal.x, newNormal.y, newNormal.z, d);
        }
    }
}


void GCamera::get3DViewportCorners(
    const Rect2D& viewport,
    Vector3& outUR,
    Vector3& outUL,
    Vector3& outLL,
    Vector3& outLR) const {

    // Must be kept in sync with getFrustum()
    const float w               = viewportWidth(viewport) / 2.0f;
    const float h               = viewportHeight(viewport) / 2.0f;
    const float z               = nearPlaneZ();

    // Compute the points
    outUR = Vector3( w,  h, z);
    outUL = Vector3(-w,  h, z);
    outLL = Vector3(-w, -h, z);
    outLR = Vector3( w, -h, z);

    // Take to world space
    outUR = cframe.pointToWorldSpace(outUR);
    outUL = cframe.pointToWorldSpace(outUL);
    outLR = cframe.pointToWorldSpace(outLR);
    outLL = cframe.pointToWorldSpace(outLL);
}


void GCamera::setPosition(const Vector3& t) { 
    cframe.translation = t;
}


void GCamera::lookAt(const Vector3& position, const Vector3& up) { 
    cframe.lookAt(position, up);
}


void GCamera::serialize(BinaryOutput& bo) const {
    bo.writeFloat64(m_fieldOfView);
    bo.writeFloat64(m_imagePlaneDepth);
    debugAssert(nearPlane > 0.0f);
    bo.writeFloat64(nearPlane);
    debugAssert(farPlane > 0.0f);
    bo.writeFloat64(farPlane);
    cframe.serialize(bo);
}


void GCamera::deserialize(BinaryInput& bi) {
    m_fieldOfView     = (float)bi.readFloat64();
    m_imagePlaneDepth = (float)bi.readFloat64();
    nearPlane       = (float)bi.readFloat64();
    debugAssert(nearPlane > 0.0f);
    farPlane        = (float)bi.readFloat64();
    debugAssert(farPlane > 0.0f);
    cframe.deserialize(bi);
}

} // namespace

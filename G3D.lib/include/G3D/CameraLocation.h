/**
 @file CameraLocation.h

 @author Morgan McGuire, morgan@cs.williams.edu
 */

#ifndef G3D_CAMERALOCATION_H
#define G3D_CAMERALOCATION_H

#include "G3D/platform.h"
#include "G3D/Spline.h"
#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"

namespace G3D {

/**
 Camera position expressed in Euler angles, suitable for spline creation.
 Unlike a G3D::Quat, CameraLocation always keeps the camera from rolling.
 */
class CameraLocation {
public:

    Vector3         translation;

    /** -pi/2 < pitch < pi/2 in radians about the X-axis */
    float           pitch;

    /** In radians about the Y-axis */
    float           yaw;

    inline CameraLocation(const Vector3& t = Vector3::zero(), float p = 0, float y = 0) 
        : translation(t), pitch(p), yaw(y) {}

    CameraLocation(const CoordinateFrame& cframe);
    
    CoordinateFrame toCoordinateFrame() const;

    /** Supports implicit cast to CoordinateFrame */
    inline operator CoordinateFrame() const {
        return toCoordinateFrame();
    }

    /** Required for use with spline */
    CameraLocation operator+(const CameraLocation& other) const;

    /** Required for use with spline */
    CameraLocation operator*(const float k) const;

    /**
       Unwraps the yaw values in the elements of the array such that
       they still represent the same angles but strictly increase/decrease
       without wrapping about zero.  For use with Spline<CameraLocation>
     */
    static void unwrapYaw(CameraLocation* a, int N);

    void serialize(class BinaryOutput& b) const;
    void deserialize(class BinaryInput& b);
};

/** Shortest-path linear velocity spline for camera positions.  Always keeps the camera from rolling.
*/
class CameraSpline : public Spline<CameraLocation> {
protected:

    virtual void ensureShortestPath(CameraLocation* A, int N) const {
        CameraLocation::unwrapYaw(A, N);
    }

public:
    
    void serialize(class BinaryOutput& b) const;
    void deserialize(class BinaryInput& b);

};

}

#endif

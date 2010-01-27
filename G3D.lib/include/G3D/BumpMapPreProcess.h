/**
  \file BumpMapPreProcess.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2010-01-28
  \edited  2010-01-28

  Copyright 2000-2010, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_BumpMapPreProcess_h
#define G3D_BumpMapPreProcess_h

#include "G3D/platform.h"

namespace G3D {
class Any;

/** 
Not in the BumpMap class to avoid a circular dependency between Texture and BumpMap.
G3D::GImage::computeNormalMap().
*/
class BumpMapPreProcess {
public:

    /** If true, the elevations are box filtered after computing normals
     and before uploading, which produces better results for parallax offset mapping
     Defaults to false. */
    bool            lowPassFilter;

    /** Height of the maximum ("white") value, in pixels, for the purpose of computing normals.
       A value of 255 means that a 255 x 255 bump image with a full black-to-white gradient
         will produce a 45-degree ramp (this also results in "cubic" voxels).
         A negative value means to set zExtentPixels to -zExtentPixels * max(width, height).
         The default is -0.05.
     */
    float           zExtentPixels;

    /** After computing normals, scale the height by |N.z|, a trick that reduces texture swim in steep areas for parallax offset 
      mapping. Defaults to false.*/
    bool            scaleZByNz;

    BumpMapPreProcess() : lowPassFilter(false), zExtentPixels(-0.05f), scaleZByNz(false) {}

    BumpMapPreProcess(const Any& any);

    operator Any() const;
};

}

#endif

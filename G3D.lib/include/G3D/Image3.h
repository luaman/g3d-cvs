/**
  @file Image3.h

  @maintainer Morgan McGuire, matrix@graphics3d.com

  @created 2007-01-31
  @edited  2007-01-31
*/


#ifndef G3D_IMAGE3_H
#define G3D_IMAGE3_H

#include "G3D/platform.h"
#include "G3D/Map2D.h"
#include "G3D/Color3.h"
#include "G3D/GImage.h"

namespace G3D {

typedef ReferenceCountedPointer<class Image3> Image3Ref;

/**
 RGB image with 32-bit floating point storage for each channel.

 See also G3D::Image3uint8, G3D::GImage.
 */
class Image3 : public Map2D<Color3, Color3> {
public:

    typedef Image3      Type;
    typedef Image3Ref   Ref;

protected:

    Image3(int w, int h, WrapMode wrap);

    void copyGImage(const class GImage& im);

public:

    /** Creates an all-zero width x height image. */
    Ref createEmpty(int width, int height, WrapMode wrap = WRAP_ERROR);

    /** Creates a 0 x 0 image. */
    Ref createEmpty(WrapMode wrap = WRAP_ERROR);

    Ref fromFile(const std::string& filename, WrapMode wrap = WRAP_ERROR, GImage::Format fmt = GImage::AUTODETECT);

    Ref fromImage3uint8(const ReferenceCountedPointer<class Image3uint8>& im);

    Ref fromGImage(const class GImage& im);

    /** Loads from any of the file formats supported by G3D::GImage.  If there is an alpha channel on the input,
        it is stripped. */
    void load(const std::string& filename, GImage::Format fmt = GImage::AUTODETECT);

    /** Saves in any of the formats supported by G3D::GImage. */
    void save(const std::string& filename, GImage::Format fmt = GImage::AUTODETECT);
};

} // G3D

#endif

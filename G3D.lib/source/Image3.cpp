/**
  @file Image3.cpp

  @maintainer Morgan McGuire, matrix@graphics3d.com

  @created 2007-01-31
  @edited  2007-01-31
*/


#include "G3D/Image3.h"
#include "G3D/Image3uint8.h"
#include "G3D/GImage.h"

namespace G3D {

Image3::Image3(int w, int h, WrapMode wrap) : Map2D<Color3, Color3>(w, h, wrap) {
    setAll(ZERO);
}


Image3::Ref Image3::fromImage3uint8(const ReferenceCountedPointer<Image3uint8>& im) {
    Ref out = createEmpty(static_cast<WrapMode>(im->wrapMode()));
    out->resize(im->width(), im->height());

    int N = im->width() * im->height();
    const Color3uint8* src = reinterpret_cast<Color3uint8*>(im->getCArray());
    for (int i = 0; i < N; ++i) {
        out->data[i] = Color3(src[i]);
    }

    return out;
}


Image3::Ref Image3::createEmpty(int width, int height, WrapMode wrap) {
    return new Type(width, height, WRAP_ERROR);
}


Image3::Ref Image3::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap);
}


Image3::Ref Image3::fromFile(const std::string& filename, WrapMode wrap, GImage::Format fmt) {
    Ref out = createEmpty(wrap);
    out->load(filename);
    return out;
}


void Image3::load(const std::string& filename, GImage::Format fmt) {
    copyGImage(GImage(filename, fmt));
}


void Image3::copyGImage(const GImage& im) {
    resize(im.width, im.height);

    int N = im.width * im.height;
    if (im.channels == 3) {
        // Convert int8 -> float
        const Color3uint8* src = im.pixel3();
        for (int i = 0; i < N; ++i) {
            data[i] = Color3(src[i]);
        }
    } else {
        // Strip alpha and convert
        const Color4uint8* src = im.pixel4();
        for (int i = 0; i < N; ++i) {
            data[i] = Color3(src[i].rgb());
        }
    }
}

/** Saves in any of the formats supported by G3D::GImage. */
void Image3::save(const std::string& filename, GImage::Format fmt) {
    GImage im(width(), height(), 3);

    int N = im.width * im.height;
    Color3uint8* dst = im.pixel3();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color3uint8(data[i]);
    }
    
    im.save(filename, fmt);
}

} // G3D

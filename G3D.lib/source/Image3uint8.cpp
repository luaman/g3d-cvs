/**
  @file Image3uint8.cpp

  @maintainer Morgan McGuire, matrix@graphics3d.com

  @created 2007-01-31
  @edited  2007-01-31
*/

#include "G3D/Image3uint8.h"
#include "G3D/Image3.h"
#include "G3D/GImage.h"

namespace G3D {

Image3uint8::Image3uint8(int w, int h, WrapMode wrap) : Map2D<Color3uint8>(w, h, wrap) {
    setAll(ZERO);
}


Image3uint8::Ref Image3uint8::fromImage3(const ReferenceCountedPointer<Image3>& im) {
    Ref out = createEmpty(static_cast<WrapMode>(im->wrapMode()));
    out->resize(im->width(), im->height());

    int N = im->width() * im->height();
    const Color3* src = reinterpret_cast<Color3*>(im->getCArray());
    for (int i = 0; i < N; ++i) {
        out->data[i] = Color3uint8(src[i]);
    }

    return out;
}


Image3uint8::Ref Image3uint8::createEmpty(int width, int height, WrapMode wrap) {
    return new Type(width, height, WRAP_ERROR);
}


Image3uint8::Ref Image3uint8::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap);
}


Image3uint8::Ref Image3uint8::fromFile(const std::string& filename, WrapMode wrap, GImage::Format fmt) {
    Ref out = createEmpty(wrap);
    out->load(filename);
    return out;
}


void Image3uint8::load(const std::string& filename, GImage::Format fmt) {
    copyGImage(GImage(filename, fmt));
}


void Image3uint8::copyGImage(const GImage& im) {
    resize(im.width, im.height);

    if (im.channels == 3) {
        // Copy data
        System::memcpy(getCArray(), im.byte(), im.width * im.height * 3);
    } else {
        // Copy 3/4 bytes
        GImage::RGBAtoRGB((const uint8*)im.byte(), (uint8*)getCArray(), im.width * im.height);
    }
}

/** Saves in any of the formats supported by G3D::GImage. */
void Image3uint8::save(const std::string& filename, GImage::Format fmt) {
    GImage im(width(), height(), 3);
    System::memcpy(im.byte(), getCArray(), width() * height() * 3);
    im.save(filename, fmt);
}

} // G3D

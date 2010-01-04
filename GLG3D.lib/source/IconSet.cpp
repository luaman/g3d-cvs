/**
  @file IconSet.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2010-01-04
  @edited  2010-01-04
*/

#include "GLG3D/IconSet.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/fileutils.h"

namespace G3D {
    
IconSet::Ref IconSet::fromFile(const std::string& filename) {
    BinaryInput b(filename, G3D_LITTLE_ENDIAN);
    
    const std::string& header = b.readString();
    alwaysAssertM(header == "ICON", "Corrupt icon file");
    
    const float version = b.readFloat32();
    alwaysAssertM(version == 1.0f, "Unsupported icon file version");

    IconSet::Ref s = new IconSet();

    s->m_icon.resize(b.readInt32());
    for (int i = 0; i < s->m_icon.size(); ++i) {
        Entry& e = s->m_icon[i];
        e.filename = b.readString32();
        float x, y, w, h;
        x = b.readFloat32();
        y = b.readFloat32();
        w = b.readFloat32();
        h = b.readFloat32();
        e.rect = Rect2D::xywh(x, y, w, h);
        s->m_index.set(e.filename, i);
    }

    GImage image;
    image.decode(b, GImage::PNG);
    Texture::Settings settings;
    settings.wrapMode = WrapMode::CLAMP;
    s->m_texture = Texture::fromGImage(filename, image, ImageFormat::AUTO(), Texture::DIM_2D_NPOT, settings);

    return s;
}


void IconSet::findImages(const std::string& baseDir, const std::string& sourceDir, Array<Source>& sourceArray) {
    Array<std::string> filenameArray;
    getFiles(pathConcat(pathConcat(baseDir, sourceDir), "*"), filenameArray);
    for (int i = 0; i < filenameArray.size(); ++i) {
        if (GImage::supportedFormat(filenameExt(filenameArray[i]))) {
            std::string f = pathConcat(sourceDir, filenameArray[i]);
            GImage im(pathConcat(baseDir, f));
            Source& s = sourceArray.next();
            s.filename = f;
            s.width    = im.width();
            s.height   = im.height();
            s.channels = im.channels();
        }
    }

    Array<std::string> dirArray;
    getDirs(pathConcat(pathConcat(baseDir, sourceDir), "*"), dirArray);
    for (int i = 0; i < dirArray.size(); ++i) {
        if (dirArray[i] != ".svn" && dirArray[i] != "CVS") {
            findImages(baseDir, pathConcat(sourceDir, dirArray[i]), sourceArray);
        }
    }
}


void IconSet::makeIconSet(const std::string& baseDir, const std::string& outFile) {
    // Find all images
    Array<Source> sourceArray;
    findImages(baseDir, "", sourceArray);
   
    // See if we can fit everything in one row
    int maxWidth = 0;
    int minHeight = 0;
    int maxChannels = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        maxWidth += sourceArray[i].width;
        minHeight = max(minHeight, sourceArray[i].height);
        maxChannels = max(maxChannels, sourceArray[i].channels);
    }

    int width = min(maxWidth, 1024);

    // Round to the nearest multiple of 4 pixels; PNG and textures
    // like this sizing.
    width = (width / 4) * 4;
    
    // Given our width, walk through on a pretend allocation to see
    // how much space we need. 

    // Current row width
    int w = 0;

    // Current row height
    int h = 0;

    // Full image height
    int height = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        // Walk until we hit the end of the row
        const Source& s = sourceArray[i];
        if (s.width + w > width) {
            // Start the next row
            height += h;
            w = 0;
            h = 0;
        }

        // Add this icon
        w += s.width;
        h = max(h, s.height);
    }

    alwaysAssertM(height < 1024, "Height must be less than 1024");

    BinaryOutput b(outFile, G3D_LITTLE_ENDIAN);

    // Write the header
    b.writeString("ICON");
    b.writeFloat32(1.0f);
    b.writeInt32(sourceArray.size());

    GImage packed(width, height, maxChannels);
    w = 0; h = 0; 
    int y = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        // Walk until we hit the end of the row
        const Source& s = sourceArray[i];
        if (s.width + w > width) {
            // Start the next row
            y += h;
            w = 0;
            h = 0;
        }

        b.writeString32(s.filename);
        b.writeFloat32(w / float(width));
        b.writeFloat32(y / float(height));
        b.writeFloat32(s.width / float(width));
        b.writeFloat32(s.height / float(height));

        GImage src;
        if (s.channels == maxChannels) {
            src.load(pathConcat(baseDir, s.filename));
        } else {
            // Need to expand the number of channels
            GImage tmp(pathConcat(baseDir, s.filename));
            src.resize(tmp.width(), tmp.height(), maxChannels);
            if (maxChannels == 4) {
                if (tmp.channels() == 1) {
                    GImage::LtoRGBA(tmp.byte(), src.byte(), tmp.width() * tmp.height());
                } else if (tmp.channels() == 3) {
                    GImage::RGBtoRGBA(tmp.byte(), src.byte(), tmp.width() * tmp.height());
                }
            } else {
                debugAssert(maxChannels == 3 && tmp.channels() == 1);
                GImage::LtoRGB(tmp.byte(), src.byte(), tmp.width() * tmp.height());
            }
        }
        
        // Paste into the packed image
        GImage::pasteSubImage(packed, src, w, y, 0, 0, src.width(), src.height());

        w += s.width;
        h = max(h, s.height);
    }

    packed.encode(GImage::PNG, b);

    b.commit();
}


Icon IconSet::get(int index) const {
    return Icon(m_texture, m_icon[index].rect);
}

}

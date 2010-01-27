/**
  \file Icon.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2010-01-04
  \edited  2010-01-04
*/
#ifndef G3D_Icon_h
#define G3D_Icon_h

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"

namespace G3D {

class Icon {
private:

    Texture::Ref    m_texture;
    Rect2D          m_sourceRect;

public:

    Icon() {}

    /** Create a new icon.  
        \sa G3D::IconSet */
    Icon(const Texture::Ref& t, const Rect2D& r = Rect2D::xywh(0,0,1,1)) : m_texture(t), m_sourceRect(r) {}

    const Texture::Ref& texture() const {
        return m_texture;
    }

    /** Position within texture() of this icon, in texture coordinates
        (not pixels).*/
    const Rect2D& sourceRect() const {
        return m_sourceRect;
    }

    int width() const {
        return iRound(m_texture->width() * m_sourceRect.width());
    }

    int height() const {
        return iRound(m_texture->height() * m_sourceRect.height());
    }
};

}

#endif

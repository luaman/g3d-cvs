/**
  @file BackgroundWidget.cpp
 
  @maintainer Morgan McGuire, morgan3d@users.sf.net
 
  @created 2007-08-16
  @edited  2007-08-16

  Copyright 2000-2007, Morgan McGuire.
  All rights reserved.
 */

#include "GLG3D/BackgroundWidget.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"

namespace G3D {

void BackgroundWidget::Posed::render (RenderDevice *rd) const {
    rd->setTexture(0, texture);
    Draw::fastRect2D(rd->viewport(), rd);
    rd->setTexture(0, NULL);
}


BackgroundWidget::BackgroundWidget() : m_posed(new Posed()) {
}

BackgroundWidgetRef BackgroundWidget::fromTexture(TextureRef t) {
    BackgroundWidgetRef b = new BackgroundWidget();
    b->setTexture(t);
    return b;
}

BackgroundWidgetRef BackgroundWidget::fromFile(const std::string& textureFilename) {
    BackgroundWidgetRef b = new BackgroundWidget();

    b->setTexture(textureFilename);

    return b;
}


void BackgroundWidget::setTexture(const std::string& textureFilename) {
    Texture::Dimension dim;

    if (GLCaps::supports_GL_ARB_texture_non_power_of_two()) {
        dim = Texture::DIM_2D_NPOT;
    } else {
        dim = Texture::DIM_2D;
    }

    setTexture(Texture::fromFile(textureFilename, ImageFormat::AUTO(), dim, Texture::Settings::video()));
}

}

/**
   @file IconSetViewer.cpp
 
 Viewer for .icn files

 \author Morgan McGuire
 
 \created 2010-01-04
 \edited  2010-01-04
*/
#include "IconSetViewer.h"


IconSetViewer::IconSetViewer(const GFont::Ref& captionFont) : m_font(captionFont) {
}


void IconSetViewer::onInit(const std::string& filename) {
    m_iconSet = IconSet::fromFile(filename);
}


void IconSetViewer::onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting) {
    app->colorClear = Color3::white();

    rd->push2D();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    const int N = m_iconSet->size();
    const Rect2D& viewport = rd->viewport();


    // Current x position (set negative to force drawing from an offset)
    int x = -3000;//-1775;
    int y = 2;
    // Highest x position of the current column
    int x1 = x;
    const int fontSize = 8;
    std::string currentPath;
    for (int i = 0; i < N; ++i) {
        const Icon& icon = m_iconSet->get(i);
        const std::string& filename = m_iconSet->filename(i);
        const int h = max(fontSize, icon.height());

        if (y + h > viewport.height()) {
            // New column
            y = 2;
            x = x1 + 12;
        }

        const std::string& path = filenamePath(filename);
        if (path != currentPath) {
            // Print the path
            Vector2 p(x, y + 10);
            p += m_font->draw2D(rd, path, p, fontSize + 2, Color3::blue(), Color4::clear());
            x1 = max(x1, iCeil(p.x));
            y = iCeil(p.y) + 1;
            currentPath = path;
        }

        rd->setTexture(0, icon.texture());
        Draw::rect2D(Rect2D::xywh(x, y, icon.width(), icon.height()), rd, Color3::white(), icon.sourceRect());

        Vector2 p(x + max(icon.width(), 32), y);
        p += m_font->draw2D(rd, filenameBaseExt(filename), p, fontSize, Color3::black(), Color4::clear());
        x1 = max(x1, iCeil(p.x));
        y = max(y + icon.height() + 2, iCeil(p.y));
    }

    rd->pop2D();
}

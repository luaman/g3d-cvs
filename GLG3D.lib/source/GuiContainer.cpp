/**
 @file GuiContainer.cpp
 
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2008-07-08
 @edited  2008-07-08
 */
#include "G3D/platform.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiText.h"
#include "GLG3D/GuiWindow.h"

namespace G3D {

GuiContainer::GuiContainer(class GuiWindow* gui, const class GuiText& text) :
    GuiControl(gui, text) {}


GuiContainer::GuiContainer(class GuiContainer* parent, const class GuiText& text) :
    GuiControl(parent, text) {}

    
void GuiContainer::setRect(const Rect2D& rect) {
    m_rect = rect;    
    m_clientRect = m_rect;
}


void GuiContainer::increaseBounds(const Vector2& extent) {
    if ((m_clientRect.width() < extent.x) || (m_clientRect.height() < extent.y)) {
        // Create the new client rect
        Vector2 newExtent = extent.max(m_clientRect.wh());

        // Pad for the container's own size
        debugAssertM(m_rect.height() >= m_clientRect.height(), "Corrupt GuiContainer bounds");
        newExtent += m_rect.wh() - m_clientRect.wh();

        // The new window has the old position and the new width
        setRect(Rect2D::xywh(m_rect.x0y0(), newExtent));

        if (m_parent != NULL) {
            m_parent->increaseBounds(m_rect.x1y1());
        } else {
            m_gui->increaseBounds(m_rect.x1y1());
        }
    }
}

}

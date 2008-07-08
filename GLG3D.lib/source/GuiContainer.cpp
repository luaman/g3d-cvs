/**
 @file GuiContainer.cpp
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2008-07-08
 @edited  2008-07-08
 */
#include "G3D/platform.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiCaption.h"

namespace G3D {

GuiContainer::GuiContainer(class GuiWindow* gui, const class GuiCaption& text) :
    GuiControl(gui, text) {}


GuiContainer::GuiContainer(class GuiContainer* parent, const class GuiCaption& text) :
    GuiControl(parent, text) {}


    
void GuiContainer::setRect(const Rect2D& rect) {
    m_rect = rect;    
    m_clientRect = m_rect;
}

}

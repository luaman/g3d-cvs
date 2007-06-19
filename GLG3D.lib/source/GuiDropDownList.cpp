/**
 @file GuiDropDownList.cpp
 
 @maintainer Morgan McGuire, morgan@graphics3d.com

 @created 2007-06-02
 @edited  2007-06-10
 */
#include "G3D/platform.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiDropDownList::GuiDropDownList
(GuiWindow*                  gui, 
 GuiPane*                    parent, 
 const GuiCaption&           caption, 
 const Pointer<int>&         indexValue, 
 Array<std::string>*         listValue) : GuiControl(gui, parent, caption), 
                                          m_indexValue(indexValue), 
                                          m_listValue(*listValue),
                                          m_selecting(false) {
}

void GuiDropDownList::render(RenderDevice* rd, const GuiSkinRef& skin) const {
    if (m_visible) {
        skin->renderDropDownList(m_rect, m_enabled, focused() || mouseOver(), m_selecting, m_listValue[*m_indexValue], m_caption);
    }
}


bool GuiDropDownList::onEvent(const GEvent& event) {
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && m_visible) {
        // TODO: handle selection TODO: GuiWindow must support an
        // active "menu" that can float over everything and steal all
        // events
        return true;
    } else {
        return false;
    }
}


}

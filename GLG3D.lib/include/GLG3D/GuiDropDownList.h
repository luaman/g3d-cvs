/**
 @file GLG3D/GuiDropDownList.h

 @created 2007-06-15
 @edited  2008-07-20

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2008, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GUIDROPDOWNLIST_H
#define G3D_GUIDROPDOWNLIST_H

#include "G3D/Pointer.h"
#include "G3D/Array.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiMenu.h"

namespace G3D {

class GuiPane;

/** List box for viewing strings.  Fires
    a G3D::GuiEvent of type G3D::GEventType::GUI_ACTION on the containing window when
    the user selects a new value.
*/
class GuiDropDownList : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:

    /** Pop-up list menu; call menu() to create this. */
    GuiMenuRef                      m_menu;
    
    GuiMenuRef menu();

    /** The index of the currently selected item. */
    Pointer<int>                    m_indexValue;

    Array<std::string>*             m_stringListValue;
    Array<GuiCaption>*              m_captionListValue;

    /** True when the menu is open */
    bool                            m_selecting;

    /** Which of the two list values to use */
    bool                            m_useStringList;

    /** Called by GuiPane */
    GuiDropDownList
       (GuiContainer*               parent, 
        const GuiCaption&           caption, 
        const Pointer<int>&         indexValue, 
        Array<std::string>*         listValue);

    GuiDropDownList
       (GuiContainer*               parent, 
        const GuiCaption&           caption, 
        const Pointer<int>&         indexValue, 
        Array<GuiCaption>*          listValue);

    /** Called by GuiPane */
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    virtual bool onEvent(const GEvent& event);
   
    /** Makes the menu appear */
    void showMenu();

public:

    virtual void setRect(const Rect2D&);
    
};

} // G3D

#endif

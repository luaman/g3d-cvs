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
    the user selects a new value, GEventType::GUI_CANCEL when the user opens the dropdown and then clicks off or presses ESC.
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

    Array<GuiCaption>               m_listValue;

    /** True when the menu is open */
    bool                            m_selecting;

    GuiDropDownList
       (GuiContainer*               parent, 
        const GuiCaption&           caption, 
        const Pointer<int>&         indexValue, 
        const Array<GuiCaption>&    listValue);

    /** Called by GuiPane */
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

    virtual bool onEvent(const GEvent& event);
   
    /** Makes the menu appear */
    void showMenu();

public:

    void setList(const Array<GuiCaption>& c);

    void setList(const Array<std::string>& c);

    /** Remove all values from the list */
    void clear();

    void append(const GuiCaption& c);

    inline const GuiCaption& get(int i) const {
        return m_listValue[i];
    }

    inline void set(int i, const GuiCaption& v) {
        m_listValue[i] = v;
    }

    inline void resize(int n) {
        m_listValue.resize(n);
        *m_indexValue = iClamp(*m_indexValue, 0, m_listValue.size() - 1);
    }

    virtual void setRect(const Rect2D&);

    /** Returns the currently selected value */
    const GuiCaption& selectedValue() const;
        

};

} // G3D

#endif

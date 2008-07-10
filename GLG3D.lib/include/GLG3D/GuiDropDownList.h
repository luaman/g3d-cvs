/**
 @file GLG3D/GuiDropDownList.h

 @created 2007-06-15
 @edited  2008-07-20

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIDROPDOWNLIST_H
#define G3D_GUIDROPDOWNLIST_H

#include "G3D/Pointer.h"
#include "G3D/Array.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"

namespace G3D {

class GuiPane;

typedef ReferenceCountedPointer<class GuiMenu> GuiMenuRef;

/**A special "popup" window that hides itself when it loses focus.
   Used by GuiDropDownList for the popup and can be used to build context menus. */
class GuiMenu : public GuiWindow {
protected:

    Array<std::string>*             m_stringListValue;
    Array<GuiCaption>*              m_captionListValue;
    /** The created labels */
    Array<GuiControl*>              m_labelArray;
    Pointer<int>                    m_indexValue;

    /** Which of the two list values to use */
    bool                            m_useStringList;

    /** Window to select when the menu is closed */
    GuiWindow*                      m_superior;

    /** Mouse is over this option */
    int                             m_highlightIndex;

    GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<GuiCaption>* listPtr, const Pointer<int>& indexValue);
    GuiMenu(const GuiThemeRef& skin, const Rect2D& rect, Array<std::string>* listPtr, const Pointer<int>& indexValue);

    /** Returns -1 if none */
    int labelIndexUnderMouse(Vector2 click) const;

public:

    static GuiMenuRef create(const GuiThemeRef& skin, Array<GuiCaption>* listPtr, const Pointer<int>& indexValue);
    static GuiMenuRef create(const GuiThemeRef& skin, Array<std::string>* listPtr, const Pointer<int>& indexValue);

    virtual bool onEvent(const GEvent& event);
    virtual void render(RenderDevice* rd);

    void hide();

    /** @param superior The window from which the menu is being created. */
    void show(WidgetManager* manager, GuiWindow* superior, const Vector2& position);
};


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

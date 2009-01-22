/**
 @file GLG3D/GuiMenu.h

 @created 2008-07-14
 @edited  2008-07-14

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2008, Morgan McGuire, morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GUIMENU_H
#define G3D_GUIMENU_H

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

    GuiControl*                     m_eventSource;

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

    /** Fires an action event */
    void fireEvent(GEventType type);
public:

    static GuiMenuRef create(const GuiThemeRef& skin, Array<GuiCaption>* listPtr, const Pointer<int>& indexValue);
    static GuiMenuRef create(const GuiThemeRef& skin, Array<std::string>* listPtr, const Pointer<int>& indexValue);

    virtual bool onEvent(const GEvent& event);
    virtual void render(RenderDevice* rd) const;

    void hide();

    /** @param superior The window from which the menu is being created. */
    virtual void show(WidgetManager* manager, GuiWindow* superior, GuiControl* eventSource, const Vector2& position, bool modal = false);
};

}
#endif

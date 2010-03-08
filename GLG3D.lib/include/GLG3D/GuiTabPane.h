/**
 @file GLG3D/GuiTabPane.h

 @created 2010-03-01
 @edited  2010-03-01

 Copyright 2000-2010, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
*/
#ifndef GLG3D_GuiTabPane_h
#define GLG3D_GuiTabPane_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Pointer.h"
#include "GLG3D/GuiContainer.h"

namespace G3D {

class GuiPane;

class GuiTabPane : public GuiContainer {
protected:

    /** Used if no index pointer is provided */
    int                 m_internalIndex;
    GuiPane*            m_tabButtonPane;
    GuiPane*            m_viewPane;

    /** Parallel array to m_contentPaneArray */
    Array<int>          m_contentIDArray;
    Array<GuiPane*>     m_contentPaneArray;
    Pointer<int>        m_indexPtr;

    /** Events are only delivered to a control when the control that
        control has the key focus (which is transferred during a mouse
        down) */
    virtual bool onEvent(const GEvent& event) { (void)event; return false; }
public:

    /** For use by GuiPane.  Call GuiPane::addTabPane to create */
    GuiTabPane(GuiContainer* parent, const Pointer<int>& index = NULL);

    /** \param id If -1 (default), set to the integer corresponding to the number of panes already in existence. Useful 
        for cases where you want the index to correspond to an enum.*/
    GuiPane* addTab(const GuiText& label, int id = -1);

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;
    virtual void setRect(const Rect2D& rect);
    virtual void pack();

};

}

#endif

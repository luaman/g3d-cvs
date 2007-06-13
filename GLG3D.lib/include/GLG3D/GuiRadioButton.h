/**
 @file GLG3D/GuiRadioButton.h

 @created 2006-05-01
 @edited  2007-06-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIRADIOBUTTON_H
#define G3D_GUIRADIOBUTTON_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;

/**
   Radio button or exclusive set of toggle butons.
 */
class GuiRadioButton : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
public:

    enum Style {RADIO_STYLE, BUTTON_STYLE};

protected:
    
    Pointer<int>     m_value;
    int              m_myID;
    Style            m_style;

    /** 
        @param myID The ID of this button 
        
        @param groupSelection Pointer to the current selection.  This button is selected
        when myID == *groupSelection
     */
    GuiRadioButton(GuiWindow* gui, GuiPane* parent, const GuiCaption& text, int myID, int* groupSelection, Style style = RADIO_STYLE);

    template<class T>
    GuiRadioButton(GuiWindow* gui,
                   GuiPane* parent,
                   const GuiCaption& text,
                   int myID,
                   T* object,
                   int (T::*get)() const,
                   void (T::*set)(bool),
                   Style style = RADIO_STYLE) 
        : GuiControl(gui, parent, text), m_value(object, get, set), 
          m_myID(myID), m_style(style) {}


    bool selected() const;
    
    void setSelected();

    virtual void render(RenderDevice* rd, const GuiSkinRef& skin) const;

    virtual bool onEvent(const GEvent& event);

public:
};

} // G3D

#endif

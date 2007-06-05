/**
 @file GLG3D/GuiWindow.h

 @created 2006-05-01
 @edited  2007-06-01

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIWINDOW_H
#define G3D_GUIWINDOW_H

#include <string>
#include "G3D/Pointer.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiSkin.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"

namespace G3D {

class GuiPane;

/**
   Retained-mode graphical user interface window. 

   %G3D Guis (Graphical User Interfaces) are "skinnable", meaning that the appearance is controled by data files.
   %G3D provides already made skins in the data/gui directory of the installation.  See GuiSkin for information
   on how to draw your own.

   The Gui API connects existing variables and methods directly to controls.  Except for GuiButton, you don't have
   to write event handlers like in other APIs. Just pass a pointer to the variable that you want to receive the
   value of the control when the control is created.  An example of creating a dialog is shown below:

   <pre>
        GuiSkinRef skin = GuiSkin::fromFile(dataDir + "gui/osx.skn");
        GuiWindow::Ref window = GuiWindow::create
            ("Person", Rect2D::xywh(300, 200, 150, 200),
             skin, GuiWindow::FRAME_STYLE, GuiWindow::HIDE_ON_CLOSE);

        GuiPane* pane = window->pane();
        pane->addCheckBox("Likes cats", &player.likesCats);
        pane->addCheckBox("Is my friend", &player, &Person::getIsMyFriend, &Person::setIsMyFriend);
        pane->addRadioButton("Male", Person::MALE, &player.gender);
        pane->addRadioButton("Female", Person::FEMALE, &player.gender);
        player.height = 1.5;
        pane->addSlider("Height", &player.height, 1.0f, 2.2f);
        GuiButton* invite = pane->addButton("Invite");

        addWidget(window);
   </pre>

   Note that in the example, one check-box is connected to a field of "player" and another to methods to get and set
   a value.  To process the button click, extend the GApp2 (or another GModule's) GApp2::onEvent method as follows:

   <pre>
   bool App::onEvent(const GEvent& e) {
       if (e.type == GEventType::GUI_ACTION) {
           if (e.gui.control == invite) {
               ... handle the invite action here ...
               return true;
           }
       }
       return false;
   }
   </pre>
 */
class GuiWindow : public Widget {

    friend class GuiControl;
    friend class GuiButton;
    friend class GuiRadioButton;
    friend class _GuiSliderBase;
    friend class GuiPane;

private:
    enum {CONTROL_WIDTH = 180};
public:
    typedef ReferenceCountedPointer<GuiWindow> Ref;

    /** Controls the appearance of the window's borders and background.
        FRAME_STYLE      - regular border and title
        TOOL_FRAME_STYLE - small title, thin border
        NO_FRAME_STYLE   - do not render any background at all
     */
    enum Style {FRAME_STYLE, TOOL_FRAME_STYLE, NO_FRAME_STYLE};

    /**
      Controls the behavior when the close button is pressed (if there
      is one).  

      NO_CLOSE - Do not show the close button
      IGNORE_CLOSE - Fire G3D::GEvent::GUI_CLOSE event but take no further action
      HIDE_ON_CLOSE - Set the window visibility to false and fire G3D::GEvent::GUI_CLOSE
      REMOVE_ON_CLOSE - Remove this GuiWindow from its containing WidgetManager and fire 
         G3D::GEvent::GUI_CLOSE with a NULL window argument (since the window may be garbage collected before the event is received).
     */
    enum CloseAction {NO_CLOSE, IGNORE_CLOSE, HIDE_ON_CLOSE, REMOVE_ON_CLOSE};
    
private:

    /** Used for rendering the UI */
    class Posed : public PosedModel2D {
    public:
        GuiWindow* gui;
        Posed(GuiWindow* gui);
        virtual Rect2D bounds () const;
        virtual float depth () const;
        virtual void render (RenderDevice *rd) const;
    };

    /** Window label */
    GuiText             m_text;

    /** Window border bounds. Actual rendering may be outside these bounds. */
    Rect2D              m_rect;

    /** Client rect bounds, absolute on the GWindow. */
    Rect2D              m_clientRect;
    
    /** Is this window visible? */
    bool                m_visible;

    Style               m_style;

    class ControlButton { 
    public:
        bool           down;
        bool           mouseOver;
        ControlButton() : down(false), mouseOver(false) {};
    };

    CloseAction         m_closeAction;
    ControlButton       m_closeButton;

    PosedModel2DRef     posed;

    GuiSkinRef          skin;

    /** True when the window is being dragged */
    bool                inDrag;

    /** Position at which the drag started */
    Vector2             dragStart;
    Rect2D              dragOriginalRect;

    GuiControl*         mouseOverGuiControl;
    GuiControl*         keyFocusGuiControl;
    
    bool                m_focused;
    bool                m_mouseVisible;

    GuiPane*            m_rootPane;

    GuiWindow(const GuiText& text, const Rect2D& rect, GuiSkinRef skin, Style style, CloseAction closeAction);

    void render(RenderDevice* rd);

    /** Take the specified close action */
    void close();

public:

    /** Is this window in focus on the WidgetManager? */
    inline bool focused() const {
        return m_focused;
    }

    /** Window bounds, including shadow and glow, absolute on the GWindow. */
    const Rect2D& rect() const {
        return m_rect;
    }

    /**
     Set the border bounds relative to the GWindow. 
     The window may render outside the bounds because of drop shadows
     and glows.
      */
    void setRect(const Rect2D& r);

    /** Client rect bounds, absolute on the GWindow. */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }

    bool visible() const {
        return m_visible;
    }

    /** Hide this entire window.  The window cannot have
        focus if it is not visible. 

        Removing the GuiWindow from the WidgetManager is more efficient
        than making it invisible.
    */
    void setVisible(bool v) { 
        m_visible = v;
    }

    ~GuiWindow();

    GuiPane* pane() {
        return m_rootPane;
    }

    static Ref create(const GuiText& windowTitle, const Rect2D& rect, const GuiSkinRef& skin, 
                      Style style = FRAME_STYLE, CloseAction = NO_CLOSE);

    void getPosedModel(Array<PosedModelRef>& posedArray, Array<PosedModel2DRef>& posed2DArray);

    virtual bool onEvent(const GEvent& event);

    virtual void onLogic() {}

    virtual void onNetwork() {}

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {}

    virtual void onUserInput(UserInput *ui);

};

}

#endif
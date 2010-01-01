/**
 @file GLG3D/GuiWindow.h

 @created 2006-05-01
 @edited  2009-09-21

 G3D Library http://g3d.sf.net
 Copyright 2000-2010, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUIWINDOW_H
#define G3D_GUIWINDOW_H

#include <string>
#include "G3D/Pointer.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class GuiPane;
class GuiContainer;

class GuiDrawer {
private:
    bool m_open;
public:
    enum Side {TOP_SIDE, LEFT_SIDE, RIGHT_SIDE, BOTTOM_SIDE};
    
    /** Returns true if this drawer has been pulled out */
    bool open() const {
        return m_open;
    }

    void setOpen(bool b) {
        // TODO
        (void)b;
    }
};
    
typedef ReferenceCountedPointer<class GuiWindow> GuiWindowRef;
/**
   Retained-mode graphical user interface window. 

   %G3D Guis (Graphical User Interfaces) are "skinnable", meaning that the appearance is controled by data files.
   %G3D provides already made skins in the data/gui directory of the installation.  See GuiTheme for information
   on how to draw your own.

   The Gui API connects existing variables and methods directly to controls.  Except for GuiButton, you don't have
   to write event handlers like in other APIs. Just pass a pointer to the variable that you want to receive the
   value of the control when the control is created.  An example of creating a dialog is shown below:

   <pre>
        GuiWindow::Ref window = GuiWindow::create("Person");

        GuiPane* pane = window->pane();
        pane->addCheckBox("Likes cats", &player.likesCats);
        pane->addCheckBox("Is my friend", &player, &Person::getIsMyFriend, &Person::setIsMyFriend);
        pane->addRadioButton("Male", Person::MALE, &player.gender);
        pane->addRadioButton("Female", Person::FEMALE, &player.gender);
        pane->addNumberBox<int>("Age", &player.age, "yrs", true, 1, 100);
        player.height = 1.5;
        pane->addSlider("Height", &player.height, 1.0f, 2.2f);
        GuiButton* invite = pane->addButton("Invite");

        addWidget(window);
   </pre>

   Note that in the example, one check-box is connected to a field of "player" and another to methods to get and set
   a value.  To process the button click, extend the GApp (or another Widget's) GApp::onEvent method as follows:

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

   It is not necessary to subclass GuiWindow to create a user
   interface.  Just instantiate GuiWindow and add controls to its
   pane.  If you do choose to subclass GuiWindow, be sure to call
   the superclass methods for those that you override.
 */
class GuiWindow : public Widget {

    friend class GuiControl;
    friend class GuiButton;
    friend class GuiRadioButton;
    friend class _GuiSliderBase;
    friend class GuiContainer;
    friend class GuiPane;
    friend class GuiTextBox;
    friend class GuiDropDownList;

private:
    enum {CONTROL_WIDTH = 180};
public:
    typedef ReferenceCountedPointer<GuiWindow> Ref;

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
    
protected:

    class ControlButton { 
    public:
        bool           down;
        bool           mouseOver;
        ControlButton() : down(false), mouseOver(false) {};
    };

    
    /** State for managing modal dialogs */
    class Modal {
    public:
        UserInput*          userInput;
        WidgetManager::Ref  manager;
        OSWindow*            osWindow;
        RenderDevice*       renderDevice;
        
        /** Image of the screen under the modal dialog.*/
        TextureRef          image;

        /** Size of the screen */
        Rect2D              viewport;
        
        /** The dialog that is running */
        GuiWindow*          dialog;

        Modal(OSWindow* osWindow);
        /** Run an event loop until the window closes */
        void run(GuiWindow::Ref dialog);
        /** Callback for OSWindow loop body */
        static void loopBody(void* me);
        /** Called from loop Body */
        void oneFrame();
        void processEventQueue();
        ~Modal();
    };

    Modal*              modal;

    /** Window label */
    GuiText          m_text;

    /** Window border bounds. Actual rendering may be outside these bounds. */
    Rect2D              m_rect;

    /** Client rect bounds, absolute on the OSWindow. */
    Rect2D              m_clientRect;
    
    /** Is this window visible? */
    bool                m_visible;

    GuiTheme::WindowStyle   m_style;

    CloseAction         m_closeAction;
    ControlButton       m_closeButton;

    GuiThemeRef         m_skin;

    /** True when the window is being dragged */
    bool                inDrag;

    /** Position at which the drag started */
    Vector2             dragStart;
    Rect2D              dragOriginalRect;

    GuiControl*         mouseOverGuiControl;
    GuiControl*         keyFocusGuiControl;
    
    bool                m_enabled;
    bool                m_focused;
    bool                m_mouseVisible;
    
    _internal::Morph    m_morph;

    Array<GuiDrawer*>   m_drawerArray;
    GuiPane*            m_rootPane;

protected:

    GuiWindow(const GuiText& text, GuiTheme::Ref skin, const Rect2D& rect, GuiTheme::WindowStyle style, CloseAction closeAction);

    virtual void render(RenderDevice* rd) const;

    /** Take the specified close action */
    void close();

    /**
      Called when tab is pressed.
     */
    void focusOnNextControl();

    void setFocusControl(GuiControl* c);

    /** Called by GuiPane::increaseBounds() */
    void increaseBounds(const Vector2& extent);

public:

    /** 
        Blocks until the dialog is closed (visible = false).  Do not call between
        RenderDevice::beginFrame and RenderDevice::endFrame.
     */
    void showModal(OSWindow* osWindow);

    void showModal(GuiWindow::Ref parent);

    /** Is this window in focus on the WidgetManager? */
    inline bool focused() const {
        return m_focused;
    }

    /** Window bounds, including shadow and glow, absolute on the OSWindow. */
    const Rect2D& rect() const {
        return m_rect;
    }

    /** Interior bounds of the window, absolute on the OSWindow */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }

    GuiThemeRef theme() const {
        return m_skin;
    }

    /**
     Set the border bounds relative to the OSWindow. 
     The window may render outside the bounds because of drop shadows
     and glows.
      */
    virtual void setRect(const Rect2D& r);

    /** Move to the center of the screen */
    void moveToCenter();

    void moveTo(const Vector2& position);

    /**
       Causes the window to change shape and/or position to meet the
       specified location.  The window will not respond to drag events
       while it is morphing.
     */
    void morphTo(const Rect2D& r);

    /** Returns true while a morph is in progress. */
    bool morphing() const {
        return m_morph.active;
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
        if (! v && m_manager != NULL) {
            m_manager->defocusWidget(this);
        }
    }

    WidgetManager* manager() const {
        return m_manager;
    }

    void setEnabled(bool e) {
        m_enabled = e;
    }

    bool enabled() const {
        return m_enabled;
    }

    ~GuiWindow();

    GuiPane* pane() {
        return m_rootPane;
    }

    const GuiPane* pane() const {
        return m_rootPane;
    }

    /** As controls are added, the window will automatically grow to contain them as needed */
    static Ref create(const GuiText& windowTitle, const GuiTheme::Ref& skin = NULL, 
                      const Rect2D& rect = Rect2D::xywh(100, 100, 100, 50), 
                      GuiTheme::WindowStyle style = GuiTheme::NORMAL_WINDOW_STYLE, 
                      CloseAction = NO_CLOSE);

    /**
       Drawers are like windows that slide out of the side of another
       GuiWindow.  Drawers are initially sized based on the side of
       the window that they slide out of, but they can be explicitly
       sized.  Multiple drawers can be attached to the same side,
       however it is up to the caller to ensure that they do not overlap.

       @param side Side that the drawer sticks out of
     */
    virtual GuiDrawer* addDrawer(const GuiText& caption = "", 
                                 GuiDrawer::Side side = GuiDrawer::RIGHT_SIDE) { 
                                     (void)caption;(void)side;
                                     return NULL; }

    virtual void onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray);

    virtual bool onEvent(const GEvent& event);

    virtual void onAI() {}

    virtual void onNetwork() {}

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
        (void)rdt;(void)sdt;(void)idt;
    }

    virtual void onUserInput(UserInput* ui);

    /** 
        Resize the pane so that all of its controls are visible and so that there is
        no wasted space, then resize the window around the pane.

        @sa G3D::GuiPane::pack
     */
    void pack();

    virtual void setCaption(const GuiText& text);

    const GuiText& caption() const {
        return m_text;
    }

    virtual Rect2D bounds () const;

    virtual float depth () const;

};

}

#endif

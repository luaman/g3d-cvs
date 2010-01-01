/**
 @file GLG3D/GuiTextBox.h

 @created 2007-06-11
 @edited  2007-06-13

 G3D Library http://g3d.sf.net
 Copyright 2001-2007, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUITEXTBOX_H
#define G3D_GUITEXTBOX_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Text box for entering strings.  

    <b>Events:</b>
    <ol> 
      <li> GEventType::GUI_ACTION when enter is pressed or the box loses focus
      <li> GEventType::GUI_CHANGE as text is entered (in IMMEDIATE_UPDATE mode)
      <li> GEventType::GUI_CANCEL when ESC is pressed
    </ol>
*/
class GuiTextBox : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
public:
    /** IMMEDIATE_UPDATE - Update the string and fire a GUI_ACTION every time the text is changed
        DELAYED_UPDATE - Wait until the box loses focus to fire an event and update the string. */
    enum Update {IMMEDIATE_UPDATE, DELAYED_UPDATE};

protected:

    /** The string that this box is associated with. */
    Pointer<std::string> m_value;

    /** The value currently being set by the user. If in
        IMMEDIATE_UPDATE mode, this is continually synchronized with
        m_value.*/
    std::string          m_userValue;

    /** Character position in m_editContents of the cursor */
    int                  m_cursorPos;

    /** True if currently being edited, that is, if the user has
     changed the string more recently than the program has changed
     it.*/
    bool                 m_editing;

    /** Original value before the user started editing.  This is used
        to detect changes in m_value while the user is editing. */
    std::string          m_oldValue;

    Update               m_update;

    /** String to be used as the cursor character */
    GuiText              m_cursor;

    /** Key that is currently auto-repeating. */
    GKeySym              m_repeatKeysym;

    /** Time at which setRepeatKeysym was called. */
    RealTime             m_keyDownTime;

    /** Time at which the key will repeat (if down). */
    RealTime             m_keyRepeatTime;

    /** Called by GuiPane */
    GuiTextBox(GuiContainer* parent, const GuiText& caption, 
               const Pointer<std::string>& value, Update update);

    virtual bool onEvent(const GEvent& event);

    /** Called from onEvent when a key is pressed. */
    void setRepeatKeysym(GKeySym key);

    /** Called from onEvent when the repeat key is released. */
    void unsetRepeatKeysym();

    /** Called from render and onEvent to enact the action triggered by the repeat key. */
    void processRepeatKeysym();

    /** Called to change the value to the typed value.*/
    virtual void commit();

public:

    virtual void setRect(const Rect2D&);    

    /** Called by GuiPane */
    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const;

};

} // G3D

#endif

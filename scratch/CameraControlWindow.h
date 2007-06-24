#ifndef G3D_CAMERACONTROLWINDOW_H
#define G3D_CAMERACONTROLWINDOW_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "UprightSplineManipulator.h"

namespace G3D {

/**
  TODO: 
  debug window bottom padding too large
  debug setFocused(true) on the textbox

  distinguish change, action, and cancel events on text box

  add button for displaying when minimized
  disable old tab switch
  add new camera control window to project
  change GApp2 to create cameracontrolwindow
 */
//
// If you are looking for an example of how to create a straightforward
// GUI in G3D do not look at this class.  CameraControlWindow uses a number
// of unusual tricks to provide a fancy compact interface that you do not
// need in a normal program.  The GUI code in this class is more complex
// than what you would have to write for a less dynamic UI.
class CameraControlWindow : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class CameraControlWindow> Ref;

protected:
    static const Vector2        smallSize;
    static const Vector2        bigSize;

    std::string cameraLocation() const;
    void setCameraLocation(const std::string& s);

    /** Array of all .trk files in the current directory */
    Array<std::string>          trackFileArray;

    /** Index into trackFileArray */
    int                         trackFileIndex;

    //enum Source {NO_SOURCE, MANUAL_SOURCE, SPLINE_SOURCE};

    /** Label for trackList.  Separate in order to allow condensed spacing */
    GuiLabel*                   trackLabel;
    GuiDropDownList*            trackList;

    GuiTextBox*                 cameraLocationTextBox;

    GuiRadioButton*             playButton;
    GuiRadioButton*             stopButton;
    GuiRadioButton*             recordButton;

    /** The manipulator from which the camera is copying its frame */
    Pointer<Manipulator::Ref>   cameraManipulator;

    FirstPersonManipulatorRef   manualManipulator;
    UprightSplineManipulatorRef trackManipulator;

    GuiCheckBox*                visibleCheckBox;
    GuiCheckBox*                cyclicCheckBox;

    /** Button to expand and contract additional manual controls. */
    GuiButton*                  drawerButton;

    /** The button must be in its own pane so that it can float over
        the expanded pane. */
    GuiPane*                    drawerButtonPane;
    GuiCaption                  drawerExpandCaption;
    GuiCaption                  drawerCollapseCaption;

    GuiButton*                  saveButton;

    GuiLabel*                   helpLabel;

    GuiCaption                  manualHelpCaption;
    GuiCaption                  autoHelpCaption;
    GuiCaption                  recordHelpCaption;

    /** If true, the window is big enough to show all controls */
    bool                        m_expanded;

    /** True when the user has chosen to override program control of
        the camera. */
    bool                        manualOperation;

    CameraControlWindow(
        const FirstPersonManipulatorRef&    manualManipulator, 
        const UprightSplineManipulatorRef&  trackManipulator, 
        const Pointer<Manipulator::Ref>&    cameraManipulator,
        const GuiSkinRef&                   skin);

    /** Sets the controller for the cameraManipulator. */
    //void setSource(Source s);

    /** Control source that the Gui thinks should be in use */
    //Source desiredSource() const;

    void sync();

    /** Updates the trackFileArray from the list of track files */
    void updateTrackFiles();

public:

    /**
     @param cameraManipulator The manipulator that should drive the camera.  This will be assigned to
     as the program runs.
     */
    static Ref create(
        const FirstPersonManipulatorRef&   manualManipulator,
        const UprightSplineManipulatorRef& trackManipulator,
        const Pointer<Manipulator::Ref>&   cameraManipulator,
        const GuiSkinRef&                  skin);

    virtual bool onEvent(const GEvent& event);
    virtual void onUserInput(UserInput*);
    virtual void setRect(const Rect2D& r);
};

}

#endif

#ifndef G3D_CAMERACONTROLWINDOW_H
#define G3D_CAMERACONTROLWINDOW_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "UprightSplineManipulator.h"

namespace G3D {

class CameraControlWindow : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class CameraControlWindow> Ref;

protected:
    static const Vector2        smallSize;
    static const Vector2        bigSize;

    /** Array of all .trk files in the current directory */
    Array<std::string>          trackFileArray;

    /** Index into trackFileArray */
    int                         trackFileIndex;

    enum Source {NO_SOURCE, MANUAL_SOURCE, SPLINE_SOURCE};

    enum Controller {PROGRAM_CONTROLLER, MANUAL_CONTROLLER, TRACK_CONTROLLER};
    Controller                  controller;

    GuiDropDownList*            trackList;

    GuiRadioButton*             programButton;
    GuiRadioButton*             manualButton;
    GuiRadioButton*             followTrackButton;

    GuiRadioButton*             playButton;
    GuiRadioButton*             stopButton;
    GuiRadioButton*             recordButton;

    Pointer<Manipulator::Ref>   cameraManipulator;

    FirstPersonManipulatorRef   manualManipulator;
    UprightSplineManipulatorRef trackManipulator;

    GuiButton*                  drawerButton;

    CameraControlWindow(
        const FirstPersonManipulatorRef&    manualManipulator, 
        const UprightSplineManipulatorRef&  trackManipulator, 
        const Pointer<Manipulator::Ref>&    cameraManipulator,
        const GuiSkinRef&                   skin);

    /** Sets the controller for the cameraManipulator. */
    void setSource(Source s);

    /** Control source that the Gui thinks should be in use */
    Source desiredSource() const;

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
};

}

#endif

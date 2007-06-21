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

    enum Controller {PROGRAM_CONTROLLER, MANUAL_CONTROLLER, TRACK_CONTROLLER};
    Controller                  controller;

    enum Mode {STOP_MODE, PLAY_MODE, RECORD_MODE};
    Mode                        mode;

    GuiDropDownList*            trackList;

    GuiRadioButton*             programButton;
    GuiRadioButton*             manualButton;
    GuiRadioButton*             followTrackButton;

    GuiRadioButton*             playButton;
    GuiRadioButton*             stopButton;
    GuiRadioButton*             recordButton;

    FirstPersonManipulatorRef   manualManipulator;
    UprightSplineManipulatorRef trackManipulator;

    CameraControlWindow(
        FirstPersonManipulatorRef& manualManipulator, 
        UprightSplineManipulatorRef& trackManipulator, 
        const GuiSkinRef& skin);

    void sync();

    /** Updates the trackFileArray from the list of track files */
    void updateTrackFiles();

public:

    static Ref create(
        FirstPersonManipulatorRef& manualManipulator,
        UprightSplineManipulatorRef& trackManipulator, 
        const GuiSkinRef& skin) {

        return new CameraControlWindow(manualManipulator, trackManipulator, skin);
    }

    virtual bool onEvent(const GEvent& event);
    virtual void onUserInput(UserInput*);
};

}

#endif

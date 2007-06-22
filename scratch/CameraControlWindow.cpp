#include "CameraControlWindow.h"

namespace G3D {

const Vector2 CameraControlWindow::smallSize(120, 84);
const Vector2 CameraControlWindow::bigSize(120, 165);


CameraControlWindow::CameraControlWindow(
    FirstPersonManipulatorRef&      manualManipulator, 
    UprightSplineManipulatorRef&    trackManipulator, 
    const GuiSkinRef&               skin) : 

    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(700, 140, 0, 0),
              GuiWindow::TOOL_FRAME_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    controller(PROGRAM_CONTROLLER),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator)
    {

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    programButton     = pane->addRadioButton("Program",      PROGRAM_CONTROLLER, &controller);
    manualButton      = pane->addRadioButton("Manual",       MANUAL_CONTROLLER,  &controller);
    followTrackButton = pane->addRadioButton("Follow Track", TRACK_CONTROLLER,   &controller);

    trackList = pane->addDropDownList("", &trackFileIndex, &trackFileArray);
    trackList->setRect(trackList->rect() - Vector2(70, 0));

    recordButton = pane->addRadioButton
        (GuiCaption::Symbol::record(), 
         UprightSplineManipulator::RECORD_KEY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::BUTTON_STYLE);

    Rect2D baseRect = Rect2D::xywh(recordButton->rect().x0() + 20, recordButton->rect().y0(), 30, 30);
    recordButton->setRect(baseRect + Vector2(baseRect.width() * 0, 0));

    playButton = pane->addRadioButton
        (GuiCaption::Symbol::play(), 
         UprightSplineManipulator::PLAY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::BUTTON_STYLE);

    playButton->setRect(baseRect + Vector2(baseRect.width() * 1, 0));

    stopButton = pane->addRadioButton
        (GuiCaption::Symbol::stop(), 
         UprightSplineManipulator::INACTIVE_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::BUTTON_STYLE);

    stopButton->setRect(baseRect + Vector2(baseRect.width() * 2, 0));

    GuiControl* last = pane->addCheckBox("Visible", trackManipulator.pointer(), &UprightSplineManipulator::showPath, &UprightSplineManipulator::setShowPath);

    last->setRect(last->rect() + Vector2(20, 0));

    pack();
    setRect(Rect2D::xywh(rect().x0y0(), smallSize));
    sync();
}


void CameraControlWindow::updateTrackFiles() {
    trackFileArray.fastClear();
    getFiles("*.trk", trackFileArray);
    for (int i = 0; i < trackFileArray.size(); ++i) {
        trackFileArray[i] = trackFileArray[i].substr(0, trackFileArray[i].length() - 4);
    }
    trackFileIndex = iMin(trackFileArray.size() - 1, trackFileIndex);
}


void CameraControlWindow::onUserInput(UserInput* ui) {
    GuiWindow::onUserInput(ui);

    // Detect if the user explicitly grabbed or released control of
    // the camera without using the gui.
    bool guiActive = desireManualActive();
    bool userActive = manualManipulator->active();
    if (guiActive != userActive) {
        if (userActive) {
            // The user took control of the camera explicitly
            controller = MANUAL_CONTROLLER;
        } else {
            // The user released control of the camera explicitly...
            if (controller == TRACK_CONTROLLER) {
                // ...while recording or playing back
                trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
            } else {
                // ...while in manual mode
                controller = PROGRAM_CONTROLLER;
            }
        }
        sync();
    }
}


void CameraControlWindow::setManualActive(bool e) {
    manualManipulator->setActive(e);
}


bool CameraControlWindow::onEvent(const GEvent& event) {
    /*
    switch(event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        debugPrintf("Mouse Button: %d\n", event.button.button);
        break;
    case GEventType::MOUSE_MOTION:
        debugPrintf("Mouse Motion, state: %d\n", event.motion.state);
        break;
    case GEventType::KEY_DOWN:
        debugPrintf("Key: %d\n", event.key.keysym.sym);
        break;
    default:;
    }*/


    // Recording
    //bool recording = trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE;

    // Allow parent to process the event
    bool c = GuiWindow::onEvent(event);
    if (c) {
        return true;
    }
    
    if (event.type == GEventType::GUI_ACTION) {
        sync();

        if (event.gui.control == recordButton) {
            // Start recording
            trackManipulator->setMode(UprightSplineManipulator::RECORD_KEY_MODE);
        }

        if (event.gui.control == stopButton) {
            // Start recording
            trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
        }
    }

    return false;
}

bool CameraControlWindow::desireManualActive() const {
    return (controller == MANUAL_CONTROLLER) ||
        ((controller == TRACK_CONTROLLER) && 
         (trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE));
}

void CameraControlWindow::sync() {
    setManualActive(desireManualActive());
    
    Vector2 currentSize = rect().wh();
    Vector2 newSize = currentSize;

    if (controller == TRACK_CONTROLLER) {
        newSize = bigSize;

        bool hasTracks = trackFileArray.size() > 0;
        trackList->setEnabled(hasTracks);

        playButton->setEnabled(trackManipulator->splineSize() > 1);

    } else {
        newSize = smallSize;
        trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
    }

    if (newSize != currentSize) {
        morphTo(Rect2D::xywh(rect().x0y0(), newSize));
    }
}

}

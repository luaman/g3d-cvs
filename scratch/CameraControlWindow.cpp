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
              Rect2D::xywh(600,200,0,0),
              GuiWindow::TOOL_FRAME_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    controller(PROGRAM_CONTROLLER),
    mode(STOP_MODE),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator)
    {

    updateTrackFiles();

    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));

    GuiPane* pane = GuiWindow::pane();

    programButton     = pane->addRadioButton("Program",      PROGRAM_CONTROLLER, &controller);
    manualButton      = pane->addRadioButton("Manual",       MANUAL_CONTROLLER,  &controller);
    followTrackButton = pane->addRadioButton("Follow Track", TRACK_CONTROLLER,   &controller);

    trackList = pane->addDropDownList("", &trackFileIndex, &trackFileArray);
    trackList->setRect(trackList->rect() - Vector2(70, 0));

    const std::string STOP = "<";
    const std::string PLAY = "4";
    const std::string RECORD = "=";

    recordButton = pane->addRadioButton(GuiCaption(RECORD, iconFont, 16, Color3::red() * 0.5f), RECORD_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
    Rect2D baseRect = Rect2D::xywh(recordButton->rect().x0() + 20, recordButton->rect().y0(), 30, 30);
    recordButton->setRect(baseRect + Vector2(baseRect.width() * 0, 0));

    playButton = pane->addRadioButton(GuiCaption(PLAY, iconFont, 16), PLAY_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
    playButton->setRect(baseRect + Vector2(baseRect.width() * 1, 0));

    stopButton = pane->addRadioButton(GuiCaption(STOP, iconFont, 16), STOP_MODE, &mode, GuiRadioButton::BUTTON_STYLE);
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

    // Because the control options don't map directly to the built-in camera controllers
    // we must explicitly sync some of the variables.

    if (manualManipulator->active()) {
        if (controller != MANUAL_CONTROLLER) {
            // The user took control of the camera explicitly
            controller = MANUAL_CONTROLLER;
            sync();
        }
    } else {
        if (controller == MANUAL_CONTROLLER) {
            // The user released control of the camera explicitly
            controller = PROGRAM_CONTROLLER;
            sync();
        }
    }
}


bool CameraControlWindow::onEvent(const GEvent& event) {

    // Recording
    bool recording = trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE;
    if (recording) {
        // See if the user pressed the capture frame key
    }

    // Allow parent to process the event
    bool c = GuiWindow::onEvent(event);
    if (c) {
        return true;
    }
    
    if (event.type == GEventType::GUI_ACTION) {
        sync();
    }

    return false;
}


void CameraControlWindow::sync() {
    manualManipulator->setActive(controller == MANUAL_CONTROLLER);
    
    Vector2 currentSize = rect().wh();
    Vector2 newSize = currentSize;

    if (controller == TRACK_CONTROLLER) {
        newSize = bigSize;

        bool hasTracks = trackFileArray.size() > 0;

        playButton->setEnabled(hasTracks);
        trackList->setEnabled(hasTracks);

    } else {
        newSize = smallSize;
    }

    if (newSize != currentSize) {
        morphTo(Rect2D::xywh(rect().x0y0(), newSize));
    }
}

}

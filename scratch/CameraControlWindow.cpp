#include "CameraControlWindow.h"

namespace G3D {

const Vector2 CameraControlWindow::smallSize(120, 184);
const Vector2 CameraControlWindow::bigSize(120, 265);

CameraControlWindow::Ref CameraControlWindow::create(
    const FirstPersonManipulatorRef&   manualManipulator,
    const UprightSplineManipulatorRef& trackManipulator,
    const Pointer<Manipulator::Ref>&   cameraManipulator,
    const GuiSkinRef&                  skin) {

    return new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, skin);
}


CameraControlWindow::CameraControlWindow(
    const FirstPersonManipulatorRef&      manualManipulator, 
    const UprightSplineManipulatorRef&    trackManipulator, 
    const Pointer<Manipulator::Ref>&      cameraManipulator,
    const GuiSkinRef&                     skin) : 

    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(700, 140, 0, 0),
              GuiWindow::NORMAL_FRAME_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    cameraManipulator(cameraManipulator),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator)
    {

    if (manualManipulator->active()) {
        controller = MANUAL_CONTROLLER;
    } else {
        controller = PROGRAM_CONTROLLER;
    }

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));

    {
    static bool b = false;

    GuiCheckBox* activeBox = pane->addCheckBox("Active", &b);

    // TODO: put the drawer button on an invisible pane so that it can float over the trayPane
    drawerButton = pane->addButton(GuiCaption("6", iconFont), GuiButton::TOOL_STYLE);
    drawerButton->setRect(Rect2D::xywh(70, activeBox->rect().y1() - 4, 12, 12));
    }

    // Put all remaining controls in a pane that is offset so that its border lines up with the edge
    GuiPane* trayPane = pane->addPane("", 0, GuiPane::SIMPLE_FRAME_STYLE);
    trayPane->setPosition(Vector2(trayPane->rect().x0() - trayPane->clientRect().x0(), trayPane->rect().y0()));

    pane = trayPane;

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
    bool guiActive = (desiredSource() == MANUAL_SOURCE);
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

    if ((controller == TRACK_CONTROLLER) && 
        (trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE)) {
        // Keep the FPS controller in sync with the spline controller
        manualManipulator->setFrame(trackManipulator->frame());
    }
}


void CameraControlWindow::setSource(Source s) {
    switch (s) {
    case NO_SOURCE:
        manualManipulator->setActive(false);
        *cameraManipulator = Manipulator::Ref(NULL);
        break;

    case MANUAL_SOURCE:
        manualManipulator->setActive(true);
        *cameraManipulator = manualManipulator;
        break;

    case SPLINE_SOURCE:
        manualManipulator->setActive(false);
        *cameraManipulator = trackManipulator;
        break;
    }
}


bool CameraControlWindow::onEvent(const GEvent& event) {
    // Allow parent to process the event
    bool c = GuiWindow::onEvent(event);
    if (c) {
        return true;
    }
    
    if (event.type == GEventType::GUI_ACTION) {
        if (event.gui.control == playButton) {
            // Restart at the beginning of the path
            trackManipulator->setTime(0);
        }
        sync();
    }

    return false;
}


CameraControlWindow::Source CameraControlWindow::desiredSource() const {
    switch (controller) {
    case PROGRAM_CONTROLLER:
        return NO_SOURCE;

    case MANUAL_CONTROLLER:
        return MANUAL_SOURCE;

    case TRACK_CONTROLLER:
        switch (trackManipulator->mode()) {
        case UprightSplineManipulator::RECORD_KEY_MODE:
        case UprightSplineManipulator::RECORD_INTERVAL_MODE:
            return MANUAL_SOURCE;

        case UprightSplineManipulator::PLAY_MODE:
            return SPLINE_SOURCE;

        case UprightSplineManipulator::INACTIVE_MODE:
            return NO_SOURCE;
        }
    }

    return NO_SOURCE;
}


void CameraControlWindow::sync() {
    setSource(desiredSource());
    
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

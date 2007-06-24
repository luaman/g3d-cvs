#include "CameraControlWindow.h"

namespace G3D {

const Vector2 CameraControlWindow::smallSize(242, 48);
const Vector2 CameraControlWindow::bigSize(242, 134);

CameraControlWindow::Ref CameraControlWindow::create(
    const FirstPersonManipulatorRef&   manualManipulator,
    const UprightSplineManipulatorRef& trackManipulator,
    const Pointer<Manipulator::Ref>&   cameraManipulator,
    const GuiSkinRef&                  skin) {

    return new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, skin);
}


std::string CameraControlWindow::cameraLocation() const {
    CoordinateFrame cframe;
    trackManipulator->camera()->getCoordinateFrame(cframe);
    UprightFrame uframe(cframe);
    
    return format("(% 5.1f, % 5.1f, % 5.1f), % 3.1f, % 3.1f", 
                  uframe.translation.x, uframe.translation.y, uframe.translation.z, 
                  uframe.yaw, uframe.pitch);
}


void CameraControlWindow::setCameraLocation(const std::string& s) {
    TextInput t(TextInput::FROM_STRING, s);
    try {
        
        UprightFrame uframe;
        uframe.translation.deserialize(t);
        t.readSymbol(",");
        uframe.yaw = t.readNumber();
        t.readSymbol(",");
        uframe.pitch = t.readNumber();
        
        CoordinateFrame cframe = uframe;

        trackManipulator->camera()->setCoordinateFrame(cframe);
        manualManipulator->setFrame(cframe);

    } catch (const TextInput::TokenException& e) {
        // Ignore the incorrectly formatted value
        (void)e;
    }
}


CameraControlWindow::CameraControlWindow(
    const FirstPersonManipulatorRef&      manualManipulator, 
    const UprightSplineManipulatorRef&    trackManipulator, 
    const Pointer<Manipulator::Ref>&      cameraManipulator,
    const GuiSkinRef&                     skin) : 
    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(5, 100, 200, 0),
              GuiWindow::TOOL_FRAME_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    cameraManipulator(cameraManipulator),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator),
    drawerButton(NULL),
    m_expanded(false)
    {

    manualOperation = manualManipulator->active();

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    // The default G3D textbox label leaves too much space between
    // the box and the label, so we override it.
    pane->addLabel("xyz")->setPosition(5, 2);
    pane->addLabel(GuiCaption("qf", greekFont, 12))->setPosition(24, 2);
    cameraLocationTextBox = pane->addTextBox("", Pointer<std::string>(this, &CameraControlWindow::cameraLocation, &CameraControlWindow::setCameraLocation));
    cameraLocationTextBox->setRect(Rect2D::xywh(-50, 2, 290, 24));
    
    GuiPane* manualPane = pane->addPane();
    manualPane->moveBy(-8, 0);

    manualPane->addCheckBox("Manual Control (F2)", &manualOperation)->moveBy(-2, 0);

    trackLabel = manualPane->addLabel("Path");
    trackLabel->moveBy(0, -3);
    trackList = manualPane->addDropDownList("", &trackFileIndex, &trackFileArray);
    trackList->setRect(Rect2D::xywh(trackList->rect().x0y0() - Vector2(54, 26), Vector2(215, trackList->rect().height())));

    visibleCheckBox = manualPane->addCheckBox("Visible", trackManipulator.pointer(), &UprightSplineManipulator::showPath, &UprightSplineManipulator::setShowPath);
    visibleCheckBox->moveRightOf(trackList);
    visibleCheckBox->moveBy(10, 0);
    
    Vector2 buttonSize = Vector2(20, 20);
    recordButton = manualPane->addRadioButton
        (GuiCaption::Symbol::record(), 
         UprightSplineManipulator::RECORD_KEY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    recordButton->moveBy(38, 1);
    recordButton->setSize(buttonSize);
    
    playButton = manualPane->addRadioButton
        (GuiCaption::Symbol::play(), 
         UprightSplineManipulator::PLAY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    playButton->setSize(buttonSize);
    playButton->moveRightOf(recordButton);

    stopButton = manualPane->addRadioButton
        (GuiCaption::Symbol::stop(), 
         UprightSplineManipulator::INACTIVE_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    stopButton->setSize(buttonSize);
    stopButton->moveRightOf(playButton);

    manualPane->pack();
    pack();
    
    // Make the pane width match the window width
    manualPane->setPosition(0, manualPane->rect().y0());
    manualPane->setSize(clientRect().width(), manualPane->rect().height());

    // Have to create the drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    drawerCollapseCaption = GuiCaption("5", iconFont);
    drawerExpandCaption = GuiCaption("6", iconFont);
    drawerButton = pane->addButton(drawerExpandCaption, GuiButton::TOOL_STYLE);
    drawerButton->setRect(Rect2D::xywh(0, 0, 12, 12));

    setRect(Rect2D::xywh(rect().x0y0(), smallSize));
    sync();
}


void CameraControlWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (drawerButton) {
        float s = 12;
        const Rect2D& r = clientRect();
        drawerButton->setPosition((r.width() - s) / 2.0f, r.height() - s);
    }
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

    if (manualOperation && (trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE)) {
        // Keep the FPS controller in sync with the spline controller
        CoordinateFrame cframe;
        trackManipulator->getFrame(cframe);
        manualManipulator->setFrame(cframe);
        trackManipulator->camera()->setCoordinateFrame(cframe);
    }
}

bool CameraControlWindow::onEvent(const GEvent& event) {
    if (! visible()) {
        return false;
    }

    // Allow super class to process the event
    if (GuiWindow::onEvent(event)) {
        return true;
    }
    
    // Accelerator key for toggling camera control
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F2)) {
        manualOperation = ! manualOperation;
        sync();
        return true;
    }

    // Special buttons
    if (event.type == GEventType::GUI_ACTION) {
        if (event.gui.control == drawerButton) {
            m_expanded = ! m_expanded;
            morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? bigSize : smallSize));
            drawerButton->setCaption(m_expanded ? drawerCollapseCaption : drawerExpandCaption);
        } else if (event.gui.control == playButton) {
            // Take over manual operation
            manualOperation = true;
            // Restart at the beginning of the path
            trackManipulator->setTime(0);
        } else if ((event.gui.control == recordButton) || (event.gui.control == cameraLocationTextBox)) {
            // Take over manual operation
            manualOperation = true;
        }
        sync();
    }

    return false;
}


void CameraControlWindow::sync() {
    //setSource(desiredSource());

    if (m_expanded) {
        bool hasTracks = trackFileArray.size() > 0;
        trackList->setEnabled(hasTracks);
        trackLabel->setEnabled(hasTracks);

        bool hasSpline = trackManipulator->splineSize() > 1;
        visibleCheckBox->setEnabled(hasSpline);
        playButton->setEnabled(hasSpline);
    }

    if (manualOperation) {
        // User has control
        bool playing = trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE;
        manualManipulator->setActive(! playing);
        if (playing) {
            *cameraManipulator = trackManipulator;
        } else {
            *cameraManipulator = manualManipulator;
        }
    } else {
        // Program has control
        manualManipulator->setActive(false);
        *cameraManipulator = Manipulator::Ref(NULL);
        trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
    }
}

}

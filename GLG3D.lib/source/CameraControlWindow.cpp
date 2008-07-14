/**
  @file CameraControlWindow.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2007-06-01
  @edited  2008-07-14
*/
#include "G3D/platform.h"
#include "G3D/GCamera.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

const Vector2 CameraControlWindow::smallSize(286, 48);
const Vector2 CameraControlWindow::bigSize(286, 157);

static const std::string noSpline = "< None >";
static const std::string untitled = "< Unsaved >";


CameraControlWindow::Ref CameraControlWindow::create(
    const FirstPersonManipulatorRef&   manualManipulator,
    const UprightSplineManipulatorRef& trackManipulator,
    const Pointer<Manipulator::Ref>&   cameraManipulator,
    const GuiThemeRef&                  skin) {

    return new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, skin);
}


std::string CameraControlWindow::cameraLocation() const {
    CoordinateFrame cframe;
    trackManipulator->camera()->getCoordinateFrame(cframe);
    UprightFrame uframe(cframe);
    
    // \xba is the character 186, which is the degree symbol
    return format("(% 5.1f, % 5.1f, % 5.1f), % 5.0f\xba, % 5.0f\xba", 
                  uframe.translation.x, uframe.translation.y, uframe.translation.z, 
                  toDegrees(uframe.yaw), toDegrees(uframe.pitch));
}


void CameraControlWindow::setCameraLocation(const std::string& s) {
    TextInput t(TextInput::FROM_STRING, s);
    try {
        
        UprightFrame uframe;
        uframe.translation.deserialize(t);
        t.readSymbol(",");
        uframe.yaw = t.readNumber();
        std::string DEGREE = "\xba";
        if (t.peek().string() == DEGREE) {
            t.readSymbol();
        }
        t.readSymbol(",");
        uframe.pitch = t.readNumber();
        if (t.peek().string() == DEGREE) {
            t.readSymbol();
        }
        
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
    const GuiThemeRef&                     skin) : 
    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(5, 54, 200, 0),
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    cameraManipulator(cameraManipulator),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator),
    drawerButton(NULL),
    drawerButtonPane(NULL),
    m_expanded(false)
    {

    manualOperation = manualManipulator->active();

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    // The default G3D textbox label doesn't support multiple fonts
    pane->addLabel(GuiCaption("qf", greekFont, 12))->setPosition(20, 2);
    cameraLocationTextBox = 
        pane->addTextBox("xyz", Pointer<std::string>(this, &CameraControlWindow::cameraLocation, 
                                                     &CameraControlWindow::setCameraLocation));
    cameraLocationTextBox->setRect(Rect2D::xywh(0, 2, 246, 24));
    cameraLocationTextBox->setCaptionSize(36);

    const char* DOWN = "6";
    const char* CHECK = "\x98";
    float h = cameraLocationTextBox->rect().height() - 4;

    GuiButton* dropDownButton = pane->addButton(GuiCaption(DOWN, iconFont, 18), GuiTheme::TOOL_BUTTON_STYLE);

    dropDownButton->setSize(h-1, h);
    dropDownButton->moveRightOf(cameraLocationTextBox);
    dropDownButton->moveBy(-2, 2);

    // Change to black "r" (x) for remove
    GuiButton* bookMarkButton = 
        pane->addButton(GuiCaption(CHECK, iconFont, 16, Color3::blue() * 0.8f), GuiTheme::TOOL_BUTTON_STYLE);
    bookMarkButton->setSize(h-1, h);

    
    GuiPane* manualPane = pane->addPane();
    manualPane->moveBy(-8, 0);

    manualPane->addCheckBox("Manual Control (F2)", &manualOperation)->moveBy(-2, 2);

    trackList = manualPane->addDropDownList("Path", &trackFileIndex, &trackFileArray);
    trackList->setRect(Rect2D::xywh(Vector2(0, trackList->rect().y1() - 25), Vector2(180, trackList->rect().height())));
    trackList->setCaptionSize(34);

    visibleCheckBox = manualPane->addCheckBox("Visible", Pointer<bool>(trackManipulator, &UprightSplineManipulator::showPath, &UprightSplineManipulator::setShowPath));
    visibleCheckBox->moveRightOf(trackList);
    visibleCheckBox->moveBy(6, 0);
    
    Vector2 buttonSize = Vector2(20, 20);
    recordButton = manualPane->addRadioButton
        (GuiCaption::Symbol::record(), 
         UprightSplineManipulator::RECORD_KEY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    recordButton->moveBy(32, 2);
    recordButton->setSize(buttonSize);
    
    playButton = manualPane->addRadioButton
        (GuiCaption::Symbol::play(), 
         UprightSplineManipulator::PLAY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    playButton->setSize(buttonSize);

    stopButton = manualPane->addRadioButton
        (GuiCaption::Symbol::stop(), 
         UprightSplineManipulator::INACTIVE_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiRadioButton::TOOL_STYLE);
    stopButton->setSize(buttonSize);

    saveButton = manualPane->addButton("Save...");
    saveButton->moveRightOf(stopButton);
    saveButton->setSize(saveButton->rect().wh() - Vector2(20, 1));
    saveButton->moveBy(20, -3);
    saveButton->setEnabled(false);

    cyclicCheckBox = manualPane->addCheckBox("Cyclic", Pointer<bool>(trackManipulator, &UprightSplineManipulator::cyclic, &UprightSplineManipulator::setCyclic));
    cyclicCheckBox->setPosition(visibleCheckBox->rect().x0(), saveButton->rect().y0() + 1);

#   ifdef G3D_OSX
        manualHelpCaption = GuiCaption("W,A,S,D and shift+left mouse to move.", NULL, 10);
#   else
        manualHelpCaption = GuiCaption("W,A,S,D and right mouse to move.", NULL, 10);
#   endif

    autoHelpCaption = "";
    playHelpCaption = "";

    recordHelpCaption = GuiCaption("Spacebar to place a control point.", NULL, 10);

    helpLabel = manualPane->addLabel(manualHelpCaption);
    helpLabel->moveBy(0, 2);

    manualPane->pack();
    pack();
    // Set the width here so that the client rect is correct below
    setRect(Rect2D::xywh(rect().x0y0(), bigSize));

    // Make the pane width match the window width
    manualPane->setPosition(0, manualPane->rect().y0());
    manualPane->setSize(clientRect().width(), manualPane->rect().height());

    // Have to create the drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    drawerCollapseCaption = GuiCaption("5", iconFont);
    drawerExpandCaption = GuiCaption("6", iconFont);
    drawerButtonPane = pane->addPane("", GuiTheme::NO_PANE_STYLE);
    drawerButton = drawerButtonPane->addButton(drawerExpandCaption, GuiTheme::TOOL_BUTTON_STYLE);
    drawerButton->setRect(Rect2D::xywh(0, 0, 12, 12));
    drawerButtonPane->setSize(12, 12);
    
    // Resize the pane to include the drawer button so that it is not clipped
    pane->setSize(clientRect().wh());

    setRect(Rect2D::xywh(rect().x0y0(), smallSize));
    sync();
}


void CameraControlWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (drawerButtonPane) {
        float s = 12;
        const Rect2D& r = clientRect();
        drawerButtonPane->setPosition((r.width() - s) / 2.0f, r.height() - s);
    }
}


void CameraControlWindow::updateTrackFiles() {
    trackFileArray.fastClear();
    trackFileArray.append(noSpline);
    getFiles("*.trk", trackFileArray);
    for (int i = 1; i < trackFileArray.size(); ++i) {
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

    // Allow super class to process the event
    if (GuiWindow::onEvent(event)) {
        return true;
    }
    
    // Accelerator key for toggling camera control.  Active even when the window is hidden.
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F2)) {
        manualOperation = ! manualOperation;
        sync();
        return true;
    }

    if (! visible()) {
        return false;
    }

    // Special buttons
    if (event.type == GEventType::GUI_ACTION) {
        GuiControl* control = event.gui.control;

        if (control == drawerButton) {

            // Change the window size
            m_expanded = ! m_expanded;
            morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? bigSize : smallSize));
            drawerButton->setCaption(m_expanded ? drawerCollapseCaption : drawerExpandCaption);

        } else if (control == trackList) {
            
            if (trackFileArray[trackFileIndex] != untitled) {
                // Load the new spline
                loadSpline(trackFileArray[trackFileIndex] + ".trk");

                // When we load, we lose our temporarily recorded spline,
                // so remove that display from the menu.
                if (trackFileArray.last() == untitled) {
                    trackFileArray.remove(trackFileArray.size() - 1);
                }
            }

        } else if (control == playButton) {

            // Take over manual operation
            manualOperation = true;
            // Restart at the beginning of the path
            trackManipulator->setTime(0);

        } else if ((control == recordButton) || (control == cameraLocationTextBox)) {

            // Take over manual operation and reset the recording
            manualOperation = true;
            trackManipulator->clear();
            trackManipulator->setTime(0);

            // Select the untitled path
            if ((trackFileArray.size() == 0) || (trackFileArray.last() != untitled)) {
                trackFileArray.append(untitled);
            }
            trackFileIndex = trackFileArray.size() - 1;

            saveButton->setEnabled(true);

        } else if (control == saveButton) {

            // Save
            std::string saveName;

            if (FileDialog::create(this)->getFilename(saveName)) {
                saveName = filenameBaseExt(trimWhitespace(saveName));

                if (saveName != "") {
                    saveName = saveName.substr(0, saveName.length() - filenameExt(saveName).length());

                    BinaryOutput b(saveName + ".trk", G3D_LITTLE_ENDIAN);
                    trackManipulator->spline().serialize(b);
                    b.commit();

                    updateTrackFiles();

                    // Select the one we just saved
                    trackFileIndex = iMax(0, trackFileArray.findIndex(saveName));
                    
                    saveButton->setEnabled(false);
                }
            }
        }
        sync();

    } else if (trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE) {
        // Check if the user has added a point yet
        sync();
    }

    return false;
}


void CameraControlWindow::loadSpline(const std::string& filename) {
    saveButton->setEnabled(false);
    trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);

    if (filename == noSpline) {
        trackManipulator->clear();
        return;
    }

    if (! fileExists(filename)) {
        trackManipulator->clear();
        return;
    }

    UprightSpline spline;

    BinaryInput b(filename, G3D_LITTLE_ENDIAN);
    spline.deserialize(b);

    trackManipulator->setSpline(spline);
    manualOperation = true;
}


void CameraControlWindow::sync() {

    if (m_expanded) {
        bool hasTracks = trackFileArray.size() > 0;
        trackList->setEnabled(hasTracks);

        bool hasSpline = trackManipulator->splineSize() > 0;
        visibleCheckBox->setEnabled(hasSpline);
        cyclicCheckBox->setEnabled(hasSpline);
        playButton->setEnabled(hasSpline);

        if (manualOperation) {
            switch (trackManipulator->mode()) {
            case UprightSplineManipulator::RECORD_KEY_MODE:
            case UprightSplineManipulator::RECORD_INTERVAL_MODE:
                helpLabel->setCaption(recordHelpCaption);
                break;

            case UprightSplineManipulator::PLAY_MODE:
                helpLabel->setCaption(playHelpCaption);
                break;

            case UprightSplineManipulator::INACTIVE_MODE:
                helpLabel->setCaption(manualHelpCaption);
            }
        } else {
            helpLabel->setCaption(autoHelpCaption);
        }
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

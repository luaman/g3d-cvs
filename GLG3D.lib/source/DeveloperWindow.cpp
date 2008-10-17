/**
  @file DeveloperWindow.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2007-06-01
  @edited  2008-02-28
*/
#include "G3D/platform.h"
#include "GLG3D/DeveloperWindow.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

DeveloperWindow::Ref DeveloperWindow::create(
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiThemeRef&                 skin,
     GConsoleRef                        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText) {

    return new DeveloperWindow(manualManipulator, trackManipulator, cameraManipulator, skin, console, debugVisible, showStats, showText);
    
}

DeveloperWindow::DeveloperWindow(
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiThemeRef&                 skin,
     GConsoleRef                        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText) : 
    GuiWindow("Developer (F11)", skin, Rect2D::xywh(600, 80, 0, 0), GuiTheme::TOOL_WINDOW_STYLE, HIDE_ON_CLOSE), consoleWindow(console) {

    cameraControlWindow = CameraControlWindow::create(manualManipulator, trackManipulator, cameraManipulator, skin);

    GuiPane* root = pane();
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    
    const float iconSize = 32;
    Vector2 buttonSize(32, 26);

    GuiCaption cameraIcon(std::string() + char(185), iconFont, iconSize);
    GuiCaption movieIcon(std::string() + char(183), iconFont, iconSize * 0.9);
    GuiCaption consoleIcon(std::string() + char(190), iconFont, iconSize * 0.9);
    GuiCaption statsIcon(std::string() + char(143), iconFont, iconSize);
    GuiCaption debugIcon("@", iconFont, iconSize * 0.8);
    GuiCaption printIcon(std::string() + char(157), iconFont, iconSize * 0.8); //105 = information

    Pointer<bool> ptr = Pointer<bool>((GuiWindow::Ref)cameraControlWindow, &GuiWindow::visible, &GuiWindow::setVisible);
    GuiControl* cameraButton = root->addCheckBox(cameraIcon, ptr, GuiTheme::TOOL_CHECK_BOX_STYLE);
    cameraButton->setSize(buttonSize);
    cameraButton->setPosition(0, 0);

    // Reserved for future use
    static bool temp = false;
    GuiControl* movieButton = root->addCheckBox(movieIcon, &temp, GuiTheme::TOOL_CHECK_BOX_STYLE);
    movieButton->setSize(buttonSize);
    movieButton->setEnabled(false);

    GuiControl* consoleButton = root->addCheckBox(consoleIcon, Pointer<bool>(consoleWindow, &GConsole::active, &GConsole::setActive), GuiTheme::TOOL_CHECK_BOX_STYLE);
    consoleButton->setSize(buttonSize);

    GuiControl* debugButton = root->addCheckBox(debugIcon, debugVisible, GuiTheme::TOOL_CHECK_BOX_STYLE);
    debugButton->setSize(buttonSize);

    GuiControl* statsButton = root->addCheckBox(statsIcon, showStats, GuiTheme::TOOL_CHECK_BOX_STYLE);
    statsButton->setSize(buttonSize);

    GuiControl* printButton = root->addCheckBox(printIcon, showText, GuiTheme::TOOL_CHECK_BOX_STYLE);
    printButton->setSize(buttonSize);

    cameraControlWindow->setVisible(true);
    pack();
    setRect(Rect2D::xywh(0, 0, 194, 38));
}


void DeveloperWindow::setManager(WidgetManager* manager) {
    if (m_manager) {
        // Remove from old
        m_manager->remove(cameraControlWindow);
    }

    if (manager) {
        // Add to new
        manager->add(cameraControlWindow);
    }

    GuiWindow::setManager(manager);

    // Move to the lower left
    Vector2 osWindowSize = manager->window()->dimensions().wh();
    setRect(Rect2D::xywh(osWindowSize - rect().wh(), rect().wh()));
}


bool DeveloperWindow::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F11)) {
        // Toggle visibility
        setVisible(! visible());
        return true;
    }

    return false;
}

}

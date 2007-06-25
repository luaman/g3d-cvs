#include "DeveloperWindow.h"
#include "CameraControlWindow.h"

DeveloperWindow::Ref DeveloperWindow::create(
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiSkinRef&                  skin,
     GConsoleRef                        console,
     bool*                              showStats,
     bool*                              showText) {

    return new DeveloperWindow(manualManipulator, trackManipulator, cameraManipulator, skin, console, showStats, showText);
    
}

DeveloperWindow::DeveloperWindow(
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiSkinRef&                  skin,
     GConsoleRef                        console,
     bool*                              showStats,
     bool*                              showText) : GuiWindow("Developer (F12)", skin, Rect2D::xywh(600,80,0,0), TOOL_FRAME_STYLE, HIDE_ON_CLOSE), consoleWindow(console) {

    cameraControlWindow = CameraControlWindow::create(manualManipulator, trackManipulator, cameraManipulator, skin);

    GuiPane* root = pane();
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    
    const float iconSize = 32;
    Vector2 buttonSize(32, 26);

    GuiCaption cameraIcon(std::string() + char(185), iconFont, iconSize);
    GuiCaption consoleIcon(std::string() + char(190), iconFont, iconSize * 0.9);
    GuiCaption statsIcon(std::string() + char(143), iconFont, iconSize);
    GuiCaption printIcon(std::string() + char(157), iconFont, iconSize * 0.8); //105 = information

    GuiControl* cameraButton = root->addCheckBox(cameraIcon, Pointer<bool>(cameraControlWindow, &GuiWindow::visible, &GuiWindow::setVisible), GuiCheckBox::TOOL_STYLE);
    cameraButton->setSize(buttonSize);
    cameraButton->setPosition(0, 0);

    GuiControl* consoleButton = root->addCheckBox(consoleIcon, Pointer<bool>(consoleWindow, &GConsole::active, &GConsole::setActive), GuiCheckBox::TOOL_STYLE);
    consoleButton->setSize(buttonSize);
    consoleButton->moveRightOf(cameraButton);

    GuiControl* statsButton = root->addCheckBox(statsIcon, showStats, GuiCheckBox::TOOL_STYLE);
    statsButton->setSize(buttonSize);
    statsButton->moveRightOf(consoleButton);

    GuiControl* printButton = root->addCheckBox(printIcon, showText, GuiCheckBox::TOOL_STYLE);
    printButton->setSize(buttonSize);
    printButton->moveRightOf(statsButton);

    cameraControlWindow->setVisible(false);
    pack();
    setRect(Rect2D::xywh(0, 0, 130, 40));
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
    setRect(Rect2D::xywh(manager->window()->dimensions().x1y1() - rect().wh(), rect().wh()));
}


bool DeveloperWindow::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F12)) {
        // Toggle visibility
        setVisible(! visible());
    }
    
    return false;
}

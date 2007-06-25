#ifndef G3D_DEVELOPERWINDOW_H
#define G3D_DEVELOPERWINDOW_H

#include <G3D/G3DAll.h>
#include "UprightSplineManipulator.h"

class DeveloperWindow : public GuiWindow {
protected:

    DeveloperWindow
    (const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiSkinRef&                  skin,
     GConsoleRef                        console,
     bool*                              showStats,
     bool*                              showText);

public:

    typedef ReferenceCountedPointer<class DeveloperWindow> Ref;

    GuiWindow::Ref                cameraControlWindow;
    GConsoleRef                   consoleWindow;

    static Ref create
    (const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const GuiSkinRef&                  skin,
     GConsoleRef                        console,
     bool*                              showStats,
     bool*                              showText);
    
    virtual void setManager(WidgetManager* manager);

    virtual bool onEvent(const GEvent& event);
};

#endif

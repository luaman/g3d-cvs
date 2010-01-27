/**
  @file DeveloperWindow.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2007-06-10
  @edited  2009-03-30
*/
#ifndef G3D_DeveloperWindow_h
#define G3D_DeveloperWindow_h

#include "G3D/platform.h"
#include "G3D/Pointer.h"
#include "GLG3D/Widget.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/VideoRecordDialog.h"
#include "GLG3D/Film.h"

namespace G3D {
/**
 Developer controls instantiated by GApp for debugging.
 @sa G3D::GApp, G3D::CameraControlWindow, G3D::GConsole
 */
class DeveloperWindow : public GuiWindow {
protected:

    DeveloperWindow
    (class GApp*                        app,
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const Film::Ref&                   film,
     const GuiThemeRef&                 theme,
     GConsoleRef                        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText);

public:

    typedef ReferenceCountedPointer<class DeveloperWindow> Ref;

    VideoRecordDialog::Ref        videoRecordDialog;
    CameraControlWindow::Ref      cameraControlWindow;
    GConsoleRef                   consoleWindow;

    static Ref create
    (GApp*                              app,
     const FirstPersonManipulatorRef&   manualManipulator,
     const UprightSplineManipulatorRef& trackManipulator,
     const Pointer<Manipulator::Ref>&   cameraManipulator,
     const Film::Ref&                   film,
     const GuiThemeRef&                 theme,
     GConsoleRef                        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText);
    
    virtual void setManager(WidgetManager* manager);

    virtual bool onEvent(const GEvent& event);
};

}

#endif

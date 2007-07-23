/**
  @file main.cpp

  This is a sample set of project files to get you started.  It uses the G3D::GApp infrastructure
  to make writing interactive 3D projects easy.  G3D does not restrict you to using GApp; see the
  other examples.

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width = 800; 
    settings.window.height = 600;

    return App(settings).run();
}

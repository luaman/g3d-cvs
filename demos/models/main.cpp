/**
  @file models/main.cpp

  @author Morgan McGuire, morgan@cs.williams.edu
 */

#include <G3D/G3DAll.h>
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;

#   ifdef G3D_WIN32
        if (! fileExists("G3D-logo-tiny-alpha.tga", false)) {
            // Running under visual studio from the wrong directory
            chdir("../demos/models/data-files");
        }
#   endif

    alwaysAssertM(fileExists("G3D-logo-tiny-alpha.tga", false), 
        "Cannot find runtime data files.");

    // Uncomment the next line to see what articulated model will look like
    // on a legacy OpenGL 1.x graphics card.
    // ArticulatedModel::setProfile(ArticulatedModel::FIXED_FUNCTION);

    return App(settings).run();
}
